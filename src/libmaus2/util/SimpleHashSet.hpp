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

#if ! defined(SIMPLEHASHSET_HPP)
#define SIMPLEHASHSET_HPP

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
		struct SimpleHashSetConstants
		{
			typedef _key_type key_type;

			static key_type const unused()
			{
				return std::numeric_limits<key_type>::max();
			}
		};

		template<typename _key_type>
		struct SimpleHashSet : public SimpleHashSetConstants<_key_type>
		{
			typedef _key_type key_type;

			typedef SimpleHashSetConstants<key_type> base_type;
			typedef SimpleHashSet<key_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			protected:
			unsigned int slog;
			uint64_t hashsize;
			uint64_t hashmask;
			uint64_t fill;

			// hash array
			::libmaus2::autoarray::AutoArray<key_type> H;

			#if ! defined(LIBMAUS2_HAVE_SYNC_OPS)
			::libmaus2::parallel::OMPLock hlock;
			::libmaus2::parallel::OMPLock clock;
			#endif

			::libmaus2::parallel::OMPLock elock;

			public:
			key_type const * begin() const { return H.begin(); }
			key_type const * end() const { return H.begin()+hashsize; }
			key_type * begin() { return H.begin(); }
			key_type * end() { return H.begin()+hashsize; }

			uint64_t getTableSize() const
			{
				return H.size();
			}

			unique_ptr_type extend() const
			{
				unique_ptr_type O(new this_type(slog+1));
				for ( uint64_t i = 0; i < hashsize; ++i )
					if ( H[i] != base_type::unused() )
						O->insert ( H[i] );
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
				uint64_t const blocksize = (from.hashsize+numblocks-1) / numblocks;
				uint64_t const idlow = blockid*blocksize;
				uint64_t const idhigh = std::min(idlow+blocksize,from.hashsize);

				for ( uint64_t i = idlow; i < idhigh; ++i )
					if ( from.H[i] != base_type::unused() )
						to.insert(from.H[i]);
			}

			template<typename key_iterator, typename index_iterator>
			void clear(key_iterator K, index_iterator I, uint64_t const n)
			{
				for ( uint64_t i = 0; i < n; ++i )
					I[i] = index(K[i]);
				for ( uint64_t i = 0; i < n; ++i )
					H[I[i]] = base_type::unused();
				fill -= n;
			}

			void extendInternal(unsigned int const logadd = 1)
			{
				if ( ((1ull<<logadd)*hashsize+fill <= H.size()) )
				{
					key_type * p = H.end();
					for ( uint64_t i = 0; i < hashsize; ++i )
						if ( H[i] != base_type::unused() )
							*(--p) = H[i];

					slog += logadd;
					hashsize = (1ull<<slog);
					hashmask = hashsize-1;

					while ( p != H.end() )
						insert(*(p++));
				}
				else
				{
					unique_ptr_type O(new this_type(slog+logadd));
					for ( uint64_t i = 0; i < hashsize; ++i )
						if ( H[i] != base_type::unused() )
							O->insert ( H[i] );

					slog = O->slog;
					hashsize = O->hashsize;
					hashmask = O->hashmask;
					fill = O->fill;
					H = O->H;
				}
			}

			SimpleHashSet(unsigned int const rslog)
			: slog(rslog), hashsize(1ull << slog), hashmask(hashsize-1), fill(0), H(hashsize,false)
			{
				std::fill(H.begin(),H.end(),base_type::unused());
			}

			void setup(unsigned int const rslog)
			{
				H.release();
				slog = rslog;
				hashsize = 1ull<<slog;
				hashmask = hashsize-1;
				fill = 0;
				H = ::libmaus2::autoarray::AutoArray<key_type>(hashsize,false);
				std::fill(H.begin(),H.end(),base_type::unused());
			}

			uint64_t hash(uint64_t const v) const
			{
				return libmaus2::hashing::EvaHash::hash642(&v,1) & hashmask;
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
				return static_cast<double>(fill) / hashsize;
			}

			// insert value and return count after insertion
			void insert(key_type const v)
			{
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
						#if defined(LIBMAUS2_HAVE_SYNC_OPS)
						bool const ok = __sync_bool_compare_and_swap ( &(H[p]), base_type::unused(), v);
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

							return;
						}
						// someone else snapped position p before we got it
						else
						{
							p = displace(p,v);
						}
					}
				} while ( p != p0 );

				::libmaus2::exception::LibMausException se;
				se.getStream() << "SimpleHashSet::insert(): unable to insert, table is full." << std::endl;
				se.finish();
				throw se;
			}

			void insertExtend(key_type const k, double const loadthres = 0.7, unsigned int const logadd = 1)
			{
				while ( (loadFactor() >= loadthres) || (fill == hashsize) )
					extendInternal(logadd);

				insert(k);
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

			// returns true if value v is contained
			uint64_t index(key_type const v) const
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
					if ( H[p] == base_type::unused() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "SimpleHashSet::index: key " << v << " is not contained" << std::endl;
						lme.finish();
						throw lme;
					}
					else
					{
						p = displace(p,v);
					}
				} while ( p != p0 );

				libmaus2::exception::LibMausException lme;
				lme.getStream() << "SimpleHashSet::index: key " << v << " is not contained" << std::endl;
				lme.finish();
				throw lme;
			}
		};

		template<typename _key_type>
		struct ExtendingSimpleHashSet : public SimpleHashSet<_key_type>
		{
			typedef _key_type key_type;

			typedef SimpleHashSet<key_type> base_type;
			typedef ExtendingSimpleHashSet<key_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typename ::libmaus2::util::SimpleHashSet<key_type>::unique_ptr_type tmpSet;

			ExtendingSimpleHashSet(unsigned int const rslog) : base_type(rslog) {}

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
						tmpSet = UNIQUE_PTR_MOVE(base_type::extendEmpty());

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
							base_type::copy(*this,*tmpSet,id,omp_get_num_threads());
					}

					#pragma omp barrier

					if ( threadid == 0 )
					{
						// copy new hash map
						base_type::assign(*tmpSet);
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
