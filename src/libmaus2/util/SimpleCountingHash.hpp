/*
    libmaus2
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

#if ! defined(SIMPLECOUNTINGHASH_HPP)
#define SIMPLECOUNTINGHASH_HPP

#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/hashing/hash.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <libmaus2/math/primes16.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename _key_type>
		struct SimpleCountingHashBase
		{
			typedef _key_type key_type;
		
			static _key_type unused()
			{
				return std::numeric_limits<key_type>::max();
			}
		};

		template<typename _key_type>
		struct SimpleCountingHashBaseType
		{
			static uint64_t hash(uint64_t const v)
			{
				return libmaus2::hashing::EvaHash::hash642(&v,1);
			}
		};
	
		template<
			typename _key_type, 
			typename _count_type, 
			typename _base_type = SimpleCountingHashBase<_key_type>,
			typename _hash_type = SimpleCountingHashBaseType<_key_type>
		>
		struct SimpleCountingHash : public _base_type
		{
			typedef _key_type key_type;
			typedef _count_type count_type;
			typedef _base_type base_type;
			typedef _hash_type hash_type;

			typedef SimpleCountingHash<key_type,count_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			protected:
			#if ! defined(LIBMAUS2_HAVE_SYNC_OPS)
			::libmaus2::parallel::OMPLock hlock;
			::libmaus2::parallel::OMPLock clock;
			#endif

			unsigned int slog;
			uint64_t hashsize;
			uint64_t hashmask;
			uint64_t fill;
			
			// value array
			::libmaus2::autoarray::AutoArray<key_type> H;
			// count array
			::libmaus2::autoarray::AutoArray<count_type> C;
			
			public:			
			key_type const * begin() const { return H.begin(); }
			key_type const * end() const { return H.end(); }
			key_type * begin() { return H.begin(); }
			key_type * end() { return H.end(); }
			count_type const * cntbegin() const { return C.begin(); }
			count_type const * cntend() const { return C.end(); }
			count_type * cntbegin() { return C.begin(); }
			count_type * cntend() { return C.end(); }
			
			void clear()
			{
				for ( key_type * p = begin(); p != end(); ++p )
					*p = base_type::unused();
				for ( count_type * p = cntbegin(); p != cntend(); ++p )
					*p = count_type();
				fill = 0;
			}
			
			uint64_t byteSize() const
			{
				return 
					sizeof(slog)+
					sizeof(hashsize)+
					sizeof(hashmask)+
					sizeof(fill)+
					H.byteSize()+
					C.byteSize();
			}

			void assign(this_type & from)
			{
				slog = from.slog;
				hashsize = from.hashsize;
				hashmask = from.hashmask;
				fill = from.fill;
				H = from.H;
				C = from.C;
			}
			
			uint64_t getTableSize() const
			{
				return H.size();
			}


			static void copy(
				this_type const & from,
				this_type & to,
				uint64_t const blockid,
				uint64_t const numblocks
				)
			{
				uint64_t const blocksize = (from.H.size()+numblocks-1) / numblocks;
				uint64_t const idlow = blockid*blocksize;
				uint64_t const idhigh = std::min(idlow+blocksize,from.H.size());
				
				for ( uint64_t i = idlow; i < idhigh; ++i )
					if ( from.H[i] != base_type::unused() )
						to.insert(from.H[i],from.C[i]);
			}

			unique_ptr_type extend() const
			{
				unique_ptr_type O(new this_type(slog+1));
				for ( uint64_t i = 0; i < H.size(); ++i )
					if ( H[i] != base_type::unused() )
						O->insert ( H[i], C[i] );
				return UNIQUE_PTR_MOVE(O);
			}

			unique_ptr_type extendEmpty() const
			{
				unique_ptr_type O(new this_type(slog+1));
				return UNIQUE_PTR_MOVE(O);
			}

			void extendInternal()
			{
				unique_ptr_type O(new this_type(slog+1));
				for ( uint64_t i = 0; i < H.size(); ++i )
					if ( H[i] != base_type::unused() )
						O->insert ( H[i], C[i] );
				
				slog = O->slog;
				hashsize = O->hashsize;
				hashmask = O->hashmask;
				fill = O->fill;
				H = O->H;
				C = O->C;
			}

			SimpleCountingHash(unsigned int const rslog)
			: slog(rslog), hashsize(1ull << slog), hashmask(hashsize-1), fill(0), H(hashsize,false), C(hashsize)
			{
				std::fill(H.begin(),H.end(),base_type::unused());
			}
			
			uint64_t hash(key_type const v) const
			{
				return hash_type::hash(v) & hashmask;
			}
			
			inline uint64_t displace(uint64_t const p, key_type const & v) const
			{
				return (p + primes16[v&0xFFFFu]) & hashmask;
			}
			
			uint64_t size() const
			{
				return fill;
			}
		
			// saturating increment	
			uint64_t increment(uint64_t const p, uint64_t const inc = 1)
			{
				uint64_t c;
				
				#if defined(LIBMAUS2_HAVE_SYNC_OPS)
				bool ok = false;
				c = 0;
				
				while ( ! ok )
				{
					c = C[p];
					
					if ( c == std::numeric_limits<count_type>::max() )
						ok = true;
					else
					{
						// TODO: check for overflow if increment is not 1
						ok = __sync_bool_compare_and_swap (C.get()+p, c, c+inc);
						if ( ok )
							c = c+inc;
					}
				}
				#else
				clock.lock();
				
				if ( C[p] == std::numeric_limits<count_type>::max() )
					c = C[p];
				else
				{
					C[p] += inc;
					c = C[p];
				}

				clock.unlock();
				#endif
				
				return c;
			}
			
			double loadFactor() const
			{
				return static_cast<double>(fill) / H.size();
			}

			count_type insertExtend(key_type const v, uint64_t const inc = 1, double const loadthres = 0.8)
			{
				if ( loadFactor() >= loadthres || fill == H.size() )
					extendInternal();
				
				return insert(v,inc);
			}

			// insert value and return count after insertion			
			count_type insert(key_type const v, uint64_t const inc = 1)
			{
				if ( v == base_type::unused() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "SimpleCountingHash::insert(): cannot insert key " << v << std::endl;
					lme.finish();
					throw lme;				
				}
			
				uint64_t const p0 = hash(v);
				uint64_t p = p0;
				
				// uint64_t loopcnt = 0;

				do
				{
					// position in use?
					if ( H[p] != base_type::unused() )
					{
						// value already present
						if ( H[p] == v )
						{
							return increment(p,inc);
						}
						// in use but by other value (collision)
						else
						{
							p = displace(p,v);
						}
					}
					// position is not currently in use, try to get it
					else
					{
						#if defined(LIBMAUS2_HAVE_SYNC_OPS)
						bool const ok = __sync_bool_compare_and_swap (H.get()+p, base_type::unused(), v);
						#else
						hlock.lock();
						bool const ok = (H[p] == base_type::unused());
						if ( ok )
							H[p] = v;
						hlock.unlock();
						#endif
						
						assert ( H[p] != base_type::unused() );
						
						// got it
						if ( H[p] == v )
						{
							// if this inserted the value, then increment fill
							if ( ok )
							{
								#if defined(LIBMAUS2_HAVE_SYNC_OPS)
								__sync_fetch_and_add(&fill,1);
								#else
								clock.lock();
								fill++;
								clock.unlock();
								#endif
							}

							return increment(p,inc);
						}
						// someone else snapped position p before we got it
						else
						{			
							p = displace(p,v);
						}
					}
				
					/*	
					if ( ++loopcnt % 1024 == 0 )
						std::cerr << "loopcnt " << loopcnt << " v=" << v << std::endl;
					*/
				} while ( p != p0 );
				
				::libmaus2::exception::LibMausException se;
				se.getStream() << "SimpleCountingHash::insert(): unable to insert, table is full." << std::endl;
				se.finish();
				throw se;
			}
			
			// returns true if value v is contained
			bool contains(key_type const v) const
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;

				do
				{
					// position in use?
					if ( H[p] == base_type::unused() )
					{
						return false;
					}
					// correct value stored
					else if ( H[p] == v )
					{
						return true;
					}
					else
					{
						p = displace(p,v);
					}
				} while ( p != p0 );
				
				return false;
			}
			
			// get count for value v
			count_type getCount(key_type const v) const
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;

				do
				{
					// position in use?
					if ( H[p] == base_type::unused() )
					{
						return 0;
					}
					// correct value stored
					else if ( H[p] == v )
					{
						return C[p];
					}
					else
					{
						p = displace(p,v);
					}
				} while ( p != p0 );
				
				return 0;
			}
		};

		template<typename _key_type, typename _count_type>
		struct ExtendingSimpleCountingHash : public SimpleCountingHash<_key_type,_count_type>
		{
			typedef _key_type key_type;
			typedef _count_type count_type;
			
			typedef SimpleCountingHash<key_type,count_type> base_type;
			typedef ExtendingSimpleCountingHash<key_type,count_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typename ::libmaus2::util::SimpleCountingHash<key_type,count_type>::unique_ptr_type tmpMap;
		
			ExtendingSimpleCountingHash(unsigned int const rslog) : base_type(rslog) {}

			void checkExtend(
				double const critload = 0.8
				)
			{
				if ( base_type::loadFactor() >= critload )
				{
					#if ! defined(_OPENMP)

					base_type::extendInternal();
					
					#else

					uint64_t const threadid = omp_get_thread_num();
					
					#pragma omp barrier
					
					if ( threadid == 0 )
						// produce empty hash map
						tmpMap = UNIQUE_PTR_MOVE(base_type::extendEmpty());
					
					#pragma omp barrier
					
					// fill empty hash map
					::libmaus2::parallel::OMPLock cntlock;
					int64_t cnt = 0;
					
					while ( cnt < omp_get_num_threads() )
					{
						int64_t id;
						
						cntlock.lock();
						if ( cnt < omp_get_num_threads() )
							id = cnt++;
						else
							id = omp_get_num_threads();
						cntlock.unlock();
						
						if ( id < omp_get_num_threads() )
							base_type::copy(*this,*tmpMap,id,omp_get_num_threads());
					}
						
					#pragma omp barrier
					
					if ( threadid == 0 )
					{
						#if 0
						// copy new hash map
						base_type::slog = tmpMap->slog;
						base_type::hashsize = tmpMap->hashsize;
						base_type::hashmask = tmpMap->hashmask;
						base_type::fill = tmpMap->fill;
						base_type::H = tmpMap->H;
						base_type::C = tmpMap->C;
						#else
						base_type::assign(*tmpMap);
						#endif
						// std::cerr << "New hash map size " << base_type::getTableSize() << std::endl;
					}
					
					#pragma omp barrier
					
					#endif		
				}
			}

		};

	}
}
#endif
