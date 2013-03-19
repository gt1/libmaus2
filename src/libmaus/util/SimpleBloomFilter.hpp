/**
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
**/


#if ! defined(SIMPLEBLOOMFILTER_HPP)
#define SIMPLEBLOOMFILTER_HPP

#include <libmaus/hashing/hash.hpp>
#include <libmaus/bitio/BitVector.hpp>
#include <libmaus/math/ilog.hpp>
#include <cmath>

namespace libmaus
{
	namespace util
	{
		struct SimpleBloomFilter
		{
			typedef SimpleBloomFilter this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			static uint64_t const baseseed = 0x9e3779b97f4a7c13ULL;
			static uint64_t const lockblocksize = 64;
			unsigned int const numvectors;
			unsigned int const vectorsizelog;
			uint64_t const vectorsize;
			uint64_t const hashmask;
			
			::libmaus::autoarray::AutoArray< ::libmaus::bitio::BitVector::unique_ptr_type > vectors;
			::libmaus::autoarray::AutoArray< uint64_t > lockvector;
			
			SimpleBloomFilter(unsigned int const rnumvectors, unsigned int const rvectorsizelog)
			: numvectors(rnumvectors), vectorsizelog(rvectorsizelog), vectorsize(1ull << vectorsizelog), hashmask(vectorsize-1ull), vectors(numvectors),
			  lockvector((vectorsize+lockblocksize-1)/lockblocksize)
			{
				for ( unsigned int i = 0; i < numvectors; ++i )
					vectors[i] = UNIQUE_PTR_MOVE(::libmaus::bitio::BitVector::unique_ptr_type(new ::libmaus::bitio::BitVector(vectorsize)));
			}
			
			void lockBlock(uint64_t const i)
			{
				#if defined(LIBMAUS_HAVE_SYNC_OPS)
				uint64_t const wordoff = i >> 6;
				uint64_t const bitoff = i - (wordoff<<6);
				uint64_t const mask = (1ull << bitoff);
				uint64_t * const word = lockvector.begin()+wordoff;
				
				bool locked = false;
				while ( !locked )
				{
					uint64_t v;
					while ( (v=*word) & mask ) {}
					
					locked = __sync_bool_compare_and_swap(word,v,v | mask);
				}			
				#else
				::libmaus::exception::LibMausException se;
				se.getStream() << "SimpleBloomFilter::lockWord() called, but binary does not support atomic/sync operations." << std::endl;
				se.finish();
				throw se;
				#endif	
			}
			
			void unlockBlock(uint64_t const i)
			{
				#if defined(LIBMAUS_HAVE_SYNC_OPS)
				uint64_t const wordoff = i >> 6;
				uint64_t const bitoff = i - (wordoff<<6);
				uint64_t const mask = (1ull << bitoff);
				uint64_t const invmask = ~mask;
				uint64_t * const word = lockvector.begin()+wordoff;
				__sync_fetch_and_and(word,invmask);
				#else
				::libmaus::exception::LibMausException se;
				se.getStream() << "SimpleBloomFilter::lockWord() called, but binary does not support atomic/sync operations." << std::endl;
				se.finish();
				throw se;				
				#endif
			}
			
			void lockForIndex(uint64_t const i)
			{
				lockBlock(i/lockblocksize);
			}
			void unlockForIndex(uint64_t const i)
			{
				unlockBlock(i/lockblocksize);
			}
			
			uint64_t hash(uint64_t const v, unsigned int const i) const
			{
				return libmaus::hashing::EvaHash::hash642(&v,1,baseseed+i) & hashmask;
			}
			
			bool contains(uint64_t const v) const
			{
				bool present = true;
				
				for ( unsigned int i = 0; i < numvectors; ++i )
					present = present && vectors[i]->get(hash(v,i));
				
				return present;	
			}

			bool containsNot(uint64_t const v) const
			{
				bool containsNot = true;
				
				for ( unsigned int i = 0; i < numvectors; ++i )
					containsNot = containsNot && (!(vectors[i]->get(hash(v,i))));
				
				return containsNot;	
			}
			
			bool insert(uint64_t const v)
			{
				bool present = true;
				
				for ( unsigned int i = 0; i < numvectors; ++i )
				{
					uint64_t const h = hash(v,i);
					present = present && vectors[i]->get(h);
					vectors[i]->set(h,1);
				}
				
				return present;
			}

			/*
			 * insert value v using synchronous bit set operations
			 * and return true if all the respective bits where set previously
			 * WARNING:
			 * this function DOES NOT GUARANTEE correct concurrent operations
			 * in the sense that all but one concurrently running inserts
			 * for the same value v will return true. If you need this guarantee,
			 * you need to use insertSyncPrecise (which is about an order of
			 * magnitude slower than insertSync)
			 */
			bool insertSync(uint64_t const v)
			{
				bool present = true;
				
				for ( unsigned int i = 0; i < numvectors; ++i )
					present = present && vectors[i]->setSync(hash(v,i)); 
								
				return present;
			}

			bool insertSyncPrecise(uint64_t const v)
			{
				uint64_t const h0 = hash(v,0);
				
				lockForIndex(h0);
				
				bool present = true;
				
				present = present && vectors[0]->setSync(h0); 
				for ( unsigned int i = 1; i < numvectors; ++i )
					present = present && vectors[i]->setSync(hash(v,i)); 
				
				unlockForIndex(h0);
				
				return present;
			}

			static unsigned int optimalK(uint64_t const m, uint64_t const n)
			{
				double const dm = m;
				double const dn = n;
				double const l2 = ::std::log(2.0);
				double const optk = (dm*l2)/dn;
				unsigned int const roundk = ::std::floor(optk+0.5);
				return std::max(roundk,1u);
			}
			static uint64_t optimalM(uint64_t const n, double const p)
			{
				double const l2 = ::std::log(2.0);
				double const optm = -(static_cast<double>(n) * ::std::log(p)) / (l2*l2);
				uint64_t const roundm = std::floor(optm+0.5);
				uint64_t const nexttwopow = ::libmaus::math::nextTwoPow(roundm);
				return std::max(nexttwopow,static_cast<uint64_t>(1u));
			}
			
			static unique_ptr_type construct(uint64_t const n, double const p)
			{
				uint64_t const optm = optimalM(n,p);
				unsigned int const optk = optimalK(optm,n);
				unsigned int const logoptm = ::libmaus::math::ilog(optm);
				// std::cerr << "optm=" << optm << " logoptm=" << logoptm << " optk=" << optk << std::endl;
				return UNIQUE_PTR_MOVE(unique_ptr_type(new this_type(optk,logoptm)));
			}
		};
	}
}
#endif
