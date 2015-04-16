/*
    libmaus
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#if ! defined(LIBMAUS_UTIL_SIMPLEHASHSETINSDEL_HPP)
#define LIBMAUS_UTIL_SIMPLEHASHSETINSDEL_HPP

#include <libmaus/util/SimpleHashMapNumberCast.hpp>
#include <libmaus/util/SimpleHashMapHashCompute.hpp>
#include <libmaus/util/SimpleHashMapKeyPrint.hpp>
#include <libmaus/util/SimpleHashMapConstants.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/hashing/hash.hpp>
#include <libmaus/math/primes16.hpp>

namespace libmaus
{
	namespace util
	{
		template<
			typename _key_type, 
			typename _constants_type = SimpleHashMapConstants<_key_type>,
			typename _key_print_type = SimpleHashMapKeyPrint<_key_type>, 
			typename _hash_compute_type = SimpleHashMapHashCompute<_key_type>, 
			typename _number_cast_type = SimpleHashMapNumberCast<_key_type>
		>
		struct SimpleHashSetInsDel : 
			public _constants_type, 
			public _key_print_type, 
			public _hash_compute_type,
			public _number_cast_type
		{
			typedef _key_type key_type;
			typedef _constants_type constants_type;
			typedef _key_print_type key_print_type;
			typedef _hash_compute_type hash_compute_type;
			typedef _number_cast_type number_cast_type;

			typedef SimpleHashSetInsDel<key_type,constants_type,key_print_type,hash_compute_type,number_cast_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			protected:
			unsigned int slog;
			uint64_t hashsize;
			uint64_t hashmask;
			uint64_t fill;
			uint64_t deleted;
			
			// hash array
			::libmaus::autoarray::AutoArray<key_type> H;
			// shrink array
			::libmaus::autoarray::AutoArray<key_type> R;

			inline uint64_t hash(key_type const & v) const
			{
				return hash_compute_type::hash(v) & hashmask;
			}
			
			inline uint64_t displace(uint64_t const p, key_type const & k) const
			{
				return (p + primes16[number_cast_type::cast(k)&0xFFFFu]) & hashmask;
			}

			void extendInternal()
			{
				if ( hashsize == H.size() )
				{
					uint64_t o = 0;
					for ( uint64_t i = 0; i < hashsize; ++i )
						if ( constants_type::isInUse(H[i]) )
						{
							R[o++] = H[i];
						}
						
					slog++;
					hashsize <<= 1;
					hashmask = hashsize-1;
					fill = 0;
					deleted = 0;
					
					H = ::libmaus::autoarray::AutoArray<key_type>(hashsize,false);
					for ( uint64_t i = 0; i < hashsize; ++i )
						H[i] = constants_type::unused();
					
					for ( uint64_t i = 0; i < o; ++i )
						insert(R[i]);
						
					R = ::libmaus::autoarray::AutoArray<key_type>(hashsize,false);					
				}
				else
				{
					assert ( hashsize < H.size() );
					assert ( 2*hashsize <= H.size() );
					
					uint64_t o = 0;
					for ( uint64_t i = 0; i < hashsize; ++i )
					{
						if ( constants_type::isInUse(H[i]) )
							R[o++] = H[i];
						H[i] = constants_type::unused();
					}
					for ( uint64_t i = hashsize; i < 2*hashsize; ++i )
						H[i] = constants_type::unused();

					hashsize <<= 1;
					slog += 1;
					hashmask = hashsize-1;
					fill = 0;
					deleted = 0;

					for ( uint64_t i = 0; i < o; ++i )
						insert(R[i]);
				}
			}
			
			void shrink()
			{
				assert ( fill <= hashsize/2 );

				if ( slog )
				{
					uint64_t o = 0;
					for ( uint64_t i = 0; i < hashsize; ++i )
					{
						if ( constants_type::isInUse(H[i]) )
							R[o++] = H[i];
						H[i] = constants_type::unused();
					}
						
					hashsize >>= 1;
					slog -= 1;
					hashmask = hashsize-1;
					fill = 0;
					deleted = 0;

					for ( uint64_t i = 0; i < o; ++i )
						insert(R[i]);						
				}
			}

			public:			
			SimpleHashSetInsDel(unsigned int const rslog)
			: slog(rslog), hashsize(1ull << slog), hashmask(hashsize-1), fill(0), deleted(0), H(hashsize,false), R(hashsize,false)
			{
				std::fill(H.begin(),H.end(),constants_type::unused());
			}
			virtual ~SimpleHashSetInsDel() {}

			key_type const * begin() const { return H.begin(); }
			key_type const * end() const { return begin()+hashsize; }
			key_type * begin() { return H.begin(); }
			key_type * end() { return begin()+hashsize; }
			
			uint64_t getTableSize() const
			{
				return H.size();
			}
						
			uint64_t size() const
			{
				return fill;
			}
		
			double loadFactor() const
			{
				return static_cast<double>(fill+deleted) / hashsize;
			}
			
			void insertExtend(key_type const & v, double const loadthres)
			{
				if ( loadFactor() >= loadthres || (fill == hashsize) )
					extendInternal();
				
				insert(v);
			}

			// returns true if value v is contained
			bool contains(key_type const & v) const
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;

				do
				{
					// correct value stored
					if ( H[p] == v )
					{
						return true;
					}
					// position in use?
					else if ( H[p] == constants_type::unused() )
					{
						return false;
					}
					else
					{
						p = displace(p,v);
					}
				} while ( p != p0 );
				
				return false;
			}

			// 
			uint64_t getIndex(key_type const & v) const
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;

				do
				{
					// correct value stored
					if ( H[p] == v )
					{
						return p;
					}
					// position in use?
					else if ( H[p] == constants_type::unused() )
					{
						// break loop and fall through to exception below
						p = p0;
					}
					else
					{
						p = displace(p,v);
					}
				} while ( p != p0 );
				
				libmaus::exception::LibMausException lme;
				lme.getStream() << "SimpleHashSetInsDel::getIndex called for non-existing key ";
				key_print_type::printKey(lme.getStream(),v);
				lme.getStream() << std::endl;
				lme.finish();
				throw lme;
			}

			// 
			uint64_t getIndexUnchecked(key_type const & v) const
			{
				uint64_t p = hash(v);

				while ( true )
				{
					if ( H[p] == v )
					{
						return p;
					}
					else
					{
						p = displace(p,v);
					}
				}				
			}

			// 
			int64_t getIndexChecked(key_type const & v) const
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;

				do
				{
					// correct value stored
					if ( H[p] == v )
					{
						return p;
					}
					// position in use?
					else if ( H[p] == constants_type::unused() )
					{
						// break loop and fall through to exception below
						p = p0;
					}
					else
					{
						p = displace(p,v);
					}
				} while ( p != p0 );
				
				return -1;
			}

			// insert key value pair
			void insert(key_type const & v)
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;
				
				do
				{
					// position in use?
					if ( constants_type::isInUse(H[p]) )
					{
						// key already present, replace value
						if ( H[p] == v )
						{
							return;
						}
						// in use but by other value (collision)
						else
						{
							p = displace(p,v);
						}
					}
					// position is not currently in use
					else
					{
						// replacing deleted index
						if ( H[p] == constants_type::deleted() )
						{
							assert ( deleted );
							deleted--;
						}
						else
						{
							assert ( H[p] == constants_type::unused() );
						}
					
						H[p] = v;
						fill += 1;
						return;
					}
				} while ( p != p0 );
				
				::libmaus::exception::LibMausException se;
				se.getStream() << "SimpleHashSetInsDel::insert(): unable to insert, table is full." << std::endl;
				se.finish();
				throw se;
			}

			// 
			void erase(key_type const & v)
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;

				do
				{
					// correct value found
					if ( H[p] == v )
					{
						H[p] = constants_type::deleted();
						fill -= 1;
						deleted += 1;

						if ( deleted >= (hashsize/2) && slog )
							shrink();
							
						return;
					}
					// position in use?
					else if ( H[p] == constants_type::unused() )
					{
						// break loop and fall through to exception below
						p = p0;
					}
					else
					{
						p = displace(p,v);
					}
				} while ( p != p0 );
				
				libmaus::exception::LibMausException lme;
				lme.getStream() << "SimpleHashSetInsDel::erase called for non-existing key ";
				key_print_type::printKey(lme.getStream(),v);
				lme.getStream() << std::endl;
				lme.finish();
				throw lme;
			}

			void eraseIndex(uint64_t const i)
			{
				H[i] = constants_type::deleted();
				deleted++;
				fill--;
				
				if ( deleted >= (hashsize/2) && slog )
					shrink();
			}

		};
	}
}
#endif
