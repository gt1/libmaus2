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
#if ! defined(LIBMAUS_UTIL_SIMPLEHASHMAP_HPP)
#define LIBMAUS_UTIL_SIMPLEHASHMAP_HPP

#include <libmaus/util/SimpleHashMapHashCompute.hpp>
#include <libmaus/util/SimpleHashMapKeyPrint.hpp>
#include <libmaus/util/SimpleHashMapConstants.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/hashing/hash.hpp>
#include <libmaus/parallel/OMPLock.hpp>
#include <libmaus/math/primes16.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/parallel/SynchronousCounter.hpp>

namespace libmaus
{
	namespace util
	{
		template<typename _key_type, typename _value_type>
		struct SimpleHashMap : public SimpleHashMapConstants<_key_type>, public SimpleHashMapKeyPrint<_key_type>, public SimpleHashMapHashCompute<_key_type>
		{
			typedef _key_type key_type;
			typedef _value_type value_type;

			typedef SimpleHashMapConstants<key_type> base_type;
			typedef std::pair<key_type,value_type> pair_type;
			typedef SimpleHashMap<key_type,value_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			protected:
			unsigned int slog;
			uint64_t hashsize;
			uint64_t hashmask;
			uint64_t fill;
			
			// hash array
			::libmaus::autoarray::AutoArray<pair_type> H;

			#if ! defined(LIBMAUS_HAVE_SYNC_OPS)
			::libmaus::parallel::OMPLock hlock;
			::libmaus::parallel::OMPLock clock;
			#endif
			
			::libmaus::parallel::OMPLock elock;
			
			public:
			unique_ptr_type uclone() const
			{
				unique_ptr_type O(new this_type(slog));
				std::copy(H.begin(),H.end(),O->H.begin());
				return UNIQUE_PTR_MOVE(O);
			}

			shared_ptr_type sclone() const
			{
				shared_ptr_type O(new this_type(slog));
				std::copy(H.begin(),H.end(),O->H.begin());
				return O;
			}

			void serialise(std::ostream & out) const
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(out,slog);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,hashsize);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,hashmask);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,fill);
				H.serialize(out);
			}
			
			SimpleHashMap(std::istream & in)
			:
				slog(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				hashsize(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				hashmask(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				fill(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				H(in)
			{
			
			}
			
			virtual ~SimpleHashMap() {}
			
			void clear()
			{
				for ( pair_type * ita = begin(); ita != end(); ++ita )
					ita->first = base_type::unused();
				fill = 0;
			}
			
			void clear(_key_type * keys, uint64_t const n)
			{
				for ( uint64_t i = 0; i < n; ++i )
					keys[i] = getIndex(keys[i]);
				for ( uint64_t i = 0; i < n; ++i )
					H[keys[i]].first = base_type::unused();
				fill = 0;
			}
			
			pair_type const * begin() const { return H.begin(); }
			pair_type const * end() const { return H.end(); }
			pair_type * begin() { return H.begin(); }
			pair_type * end() { return H.end(); }
			
			uint64_t getTableSize() const
			{
				return H.size();
			}
			
			unique_ptr_type extend() const
			{
				unique_ptr_type O(new this_type(slog+1));
				for ( uint64_t i = 0; i < H.size(); ++i )
					if ( H[i].first != base_type::unused() )
						O->insert ( H[i].first, H[i].second );
				return UNIQUE_PTR_MOVE(O);
			}

			unique_ptr_type extendEmpty() const
			{
				unique_ptr_type O(new this_type(slog+1));
				return UNIQUE_PTR_MOVE(O);
			}

			void assign(this_type & from)
			{
				slog = from.slog;
				hashsize = from.hashsize;
				hashmask = from.hashmask;
				fill = from.fill;
				H = from.H;
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
					if ( from.H[i].first != base_type::unused() )
						to.insert(from.H[i].first,from.H[i].second);
			}

			void extendInternal()
			{
				unique_ptr_type O(new this_type(slog+1));
				for ( uint64_t i = 0; i < H.size(); ++i )
					if ( H[i].first != base_type::unused() )
						O->insert ( H[i].first, H[i].second );
				
				slog = O->slog;
				hashsize = O->hashsize;
				hashmask = O->hashmask;
				fill = O->fill;
				H = O->H;
			}

			void extendInternalNonSync()
			{
				unique_ptr_type O(new this_type(slog+1));
				for ( uint64_t i = 0; i < H.size(); ++i )
					if ( H[i].first != base_type::unused() )
						O->insertNonSync ( H[i].first, H[i].second );
				
				slog = O->slog;
				hashsize = O->hashsize;
				hashmask = O->hashmask;
				fill = O->fill;
				H = O->H;
			}

			SimpleHashMap(unsigned int const rslog)
			: slog(rslog), hashsize(1ull << slog), hashmask(hashsize-1), fill(0), H(hashsize,false), elock()
			{
				std::fill(H.begin(),H.end(),pair_type(base_type::unused(),value_type()));
			}
			
			inline uint64_t hash(uint64_t const v) const
			{
				return SimpleHashMapHashCompute<_key_type>::hash(v) & hashmask;
			}
			
			inline uint64_t displace(uint64_t const p, uint64_t const v) const
			{
				return (p + primes16[v&0xFFFFu]) & hashmask;
			}
			
			uint64_t size() const
			{
				return fill;
			}
		
			double loadFactor() const
			{
				return static_cast<double>(fill) / H.size();
			}
			
			void insertExtend(key_type const v, value_type const w, double const loadthres)
			{
				if ( loadFactor() >= loadthres || (fill == H.size()) )
					extendInternal();
				
				insert(v,w);
			}

			void insertNonSyncExtend(key_type const v, value_type const w, double const loadthres)
			{
				if ( loadFactor() >= loadthres || (fill == H.size()) )
					extendInternalNonSync();
				
				insertNonSync(v,w);
			}

			// insert key value pair
			void insert(key_type const v, value_type const w)
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;
				
				// uint64_t loopcnt = 0;

				do
				{
					// position in use?
					if ( H[p].first != base_type::unused() )
					{
						// value already present
						if ( H[p].first == v )
						{
							H[p].second = w;
							return;
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
						#if defined(LIBMAUS_HAVE_SYNC_OPS)
						bool const ok = __sync_bool_compare_and_swap ( &(H[p].first), base_type::unused(), v);
						#else
						hlock.lock();
						bool const ok = (H[p].first == base_type::unused());
						if ( ok )
							H[p].first = v;
						hlock.unlock();
						#endif
						
						assert ( H[p].first != base_type::unused() );
						
						// got it
						if ( H[p].first == v )
						{
							// if this inserted the value, then increment fill
							if ( ok )
							{
								#if defined(LIBMAUS_HAVE_SYNC_OPS)
								__sync_fetch_and_add(&fill,1);
								#else
								clock.lock();
								fill++;
								clock.unlock();
								#endif
							}

							H[p].second = w;
							return;
						}
						// someone else snapped position p before we got it
						else
						{			
							p = displace(p,v);
						}
					}
				} while ( p != p0 );
				
				::libmaus::exception::LibMausException se;
				se.getStream() << "SimpleHashMap::insert(): unable to insert, table is full." << std::endl;
				se.finish();
				throw se;
			}

			// insert key value pair
			void insertNonSync(key_type const v, value_type const w)
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;
				
				// uint64_t loopcnt = 0;

				do
				{
					// position in use?
					if ( H[p].first != base_type::unused() )
					{
						// value already present
						if ( H[p].first == v )
						{
							H[p].second = w;
							return;
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
						bool const ok = (H[p].first == base_type::unused());
						if ( ok )
							H[p].first = v;
						
						assert ( H[p].first != base_type::unused() );
						
						// got it
						if ( H[p].first == v )
						{
							// if this inserted the value, then increment fill
							if ( ok )
								fill++;

							H[p].second = w;
							return;
						}
						// someone else snapped position p before we got it
						else
						{			
							p = displace(p,v);
						}
					}
				} while ( p != p0 );
				
				::libmaus::exception::LibMausException se;
				se.getStream() << "SimpleHashMap::insert(): unable to insert, table is full." << std::endl;
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
					if ( H[p].first == base_type::unused() )
					{
						return false;
					}
					// correct value stored
					else if ( H[p].first == v )
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

			// 
			uint64_t getIndex(key_type const v) const
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;

				do
				{
					// position in use?
					if ( H[p].first == base_type::unused() )
					{
						// break loop and fall through to exception below
						p = p0;
					}
					// correct value stored
					else if ( H[p].first == v )
					{
						return p;
					}
					else
					{
						p = displace(p,v);
					}
				} while ( p != p0 );
				
				libmaus::exception::LibMausException lme;
				lme.getStream() << "SimpleHashMap::getIndex called for non-existing key ";
				SimpleHashMapKeyPrint<_key_type>::printKey(lme.getStream(),v);
				lme.getStream() << std::endl;
				lme.finish();
				throw lme;
			}

			// returns true if value v is contained
			bool contains(key_type const v, value_type & r) const
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;

				do
				{
					// position in use?
					if ( H[p].first == base_type::unused() )
					{
						return false;
					}
					// correct value stored
					else if ( H[p].first == v )
					{
						r = H[p].second;
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
			value_type get(key_type const v) const
			{
				uint64_t const p0 = hash(v);
				uint64_t p = p0;

				do
				{
					// position in use?
					// correct value stored
					if ( H[p].first == v )
					{
						return H[p].second;
					}
					else if ( H[p].first == base_type::unused() )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "SimpleHashMap::get() called for key ";
						SimpleHashMapKeyPrint<_key_type>::printKey(se.getStream(), v);
						se.getStream() << " which is not contained." << std::endl;
						se.finish();
						throw se;
					}
					else
					{
						p = displace(p,v);
					}
				} while ( p != p0 );
				
				::libmaus::exception::LibMausException se;
				se.getStream() << "SimpleHashMap::get() called for key ";
				SimpleHashMapKeyPrint<_key_type>::printKey(se.getStream(),v);
				se.getStream() << " which is not contained." << std::endl;
				se.finish();
				throw se;
			}

			// get count for value v without checks
			value_type getUnchecked(key_type const v) const
			{
				uint64_t p = hash(v);

				while ( true )
					// correct value stored
					if ( H[p].first == v )
						return H[p].second;
					else
						p = displace(p,v);
			}
		};

		template<typename _key_type, typename _value_type>
		struct ExtendingSimpleHashMap : public SimpleHashMap<_key_type,_value_type>
		{
			typedef _key_type key_type;
			typedef _value_type value_type;
			
			typedef SimpleHashMap<key_type,value_type> base_type;
			typedef ExtendingSimpleHashMap<key_type,value_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			typename ::libmaus::util::SimpleHashMap<key_type,value_type>::unique_ptr_type tmpMap;
		
			ExtendingSimpleHashMap(unsigned int const rslog) : base_type(rslog) {}
			ExtendingSimpleHashMap(std::istream & in) : base_type(in) {}

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
					::libmaus::parallel::OMPLock cntlock;
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
						// copy new hash map
						base_type::assign(*tmpMap);
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
