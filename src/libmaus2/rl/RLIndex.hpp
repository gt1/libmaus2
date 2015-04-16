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

#if ! defined(RLINDEX_HPP)
#define RLINDEX_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/LELoad.hpp>
#include <libmaus2/rank/ImpCacheLineRank.hpp>
#include <libmaus2/select/ImpCacheLineSelectSupport.hpp>
#include <libmaus2/math/ilog.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <map>
#include <cmath>

namespace libmaus2
{
	namespace rl
	{
		struct PairPushBuffer
		{
			::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t>, ::libmaus2::autoarray::alloc_type_c > A;
			std::pair<uint64_t,uint64_t> * P;
			
			PairPushBuffer()
			: A(), P(A.begin())
			{
			}
			
			std::pair<uint64_t,uint64_t> * begin() { return A.begin(); }
			std::pair<uint64_t,uint64_t> * end()   { return P; }
			std::pair<uint64_t,uint64_t> const * begin() const { return A.begin(); }
			std::pair<uint64_t,uint64_t> const * end()   const { return P; }
			
			void reset()
			{
				P = A.begin();
			}
			
			void push(int64_t const a, uint64_t const b)
			{
				if ( P == A.end() )
				{
					uint64_t const d = P-A.begin();
					A.resize( std::max(2*A.size(),static_cast<uint64_t>(1)) );
					P = A.begin() + d;
					assert ( P != A.end() );
				}
				
				*(P++) = std::pair<uint64_t,uint64_t>(a,b);
			}
		};
		
		struct RLSimpleIndexDataBase
		{
			uint64_t const n;
			uint64_t const numsyms;
			uint64_t const logsymsperblock;
			uint64_t const symsperblock;
			uint64_t const bytespern;
			::libmaus2::autoarray::AutoArray<uint64_t> const BP;
			uint64_t const numlogicalblocks;
			uint64_t const dictbytes;
			::libmaus2::autoarray::AutoArray<uint8_t> const RL;
			
			::libmaus2::autoarray::AutoArray<uint64_t> updateBP(::libmaus2::autoarray::AutoArray<uint64_t> BP)
			{
				if ( BP.size() )
				{
					uint64_t const base = BP[0];
					for ( uint64_t i = 0; i < BP.size(); ++i )
						BP[i] -= base;
				}
				
				return BP;
			}
			
			RLSimpleIndexDataBase(std::istream & in)
			:
				n ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				numsyms ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				logsymsperblock ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				symsperblock ( 1ull << logsymsperblock ),
				bytespern ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				BP ( updateBP(::libmaus2::autoarray::AutoArray<uint64_t>(in)) ),
				numlogicalblocks(BP.size()),
				dictbytes(numsyms * bytespern),
				RL ( in )
			{
			}

			RLSimpleIndexDataBase(std::istream & in, uint64_t & rsize)
			:
				n ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				numsyms ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				logsymsperblock ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				symsperblock ( 1ull << logsymsperblock ),
				bytespern ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				BP ( updateBP(::libmaus2::autoarray::AutoArray<uint64_t>(in,rsize)) ),
				numlogicalblocks(BP.size()),
				dictbytes(numsyms * bytespern),
				RL ( in, rsize )
			{
				rsize += sizeof(n);
				rsize += sizeof(numsyms);
				rsize += sizeof(logsymsperblock);
				rsize += sizeof(bytespern);
				rsize += sizeof(dictbytes);
			}
			
			uint64_t logicalBlockOffset(uint64_t const i) const
			{
				return BP[i];
			}
		};
		
		struct RLIndexDataBase
		{
			uint64_t const n;
			uint64_t const bitspern;
			uint64_t const bytespern;
			uint64_t const numsyms;
			uint64_t const dictbytes;
			uint64_t const cachelinebytes;
			uint64_t const paybytes;
			uint64_t const byterunthres;
			uint64_t const symsperblock;
			::libmaus2::autoarray::AutoArray<uint8_t> const RL;
			::libmaus2::rank::ImpCacheLineRank const ICLR;
			uint64_t const n1;
			uint64_t const n0;
			double const avg;
			double const rl;
			uint64_t const runsperblock;
			unsigned int const ilog;
			uint64_t const downrunsperblock;
			::libmaus2::select::ImpCacheLineSelectSupport const ISS;
			uint64_t const numlogicalblocks;
			// ::libmaus2::autoarray::AutoArray<uint64_t> const D;
			
			static uint64_t getSymsPerBlock(std::istream & in)
			{
				::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				return ::libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			}
			
			static uint64_t getSymsPerBlock(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				return getSymsPerBlock(istr);
			}
			
			uint64_t logicalBlockOffset(uint64_t const blockid) const
			{
				uint64_t const onepos = ISS.select1(blockid);
				uint64_t const zrank = ICLR.rank0(onepos);
				return zrank * cachelinebytes;
			}
						
			RLIndexDataBase(std::istream & in)
			: 
				n ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				bitspern ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				bytespern ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				numsyms ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				dictbytes ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				cachelinebytes ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				paybytes ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				byterunthres ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				symsperblock ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				RL(in),
				ICLR(in),
				n1(ICLR.n ? ICLR.rank1(ICLR.n-1) : 0),
				n0(ICLR.n ? ICLR.rank0(ICLR.n-1) : 0),
				avg(n0/static_cast<double>(n1-1.0)),
				rl(1.0+avg),
				runsperblock ( static_cast<uint64_t>(std::floor((6.0*64.0)/rl)) ),
				ilog(::libmaus2::math::ilog(runsperblock)),
				downrunsperblock(1ull << ilog),
				ISS(ICLR,ilog),
				numlogicalblocks( (n + symsperblock -1 ) / symsperblock )
			{
			
			}

			RLIndexDataBase(std::istream & in, uint64_t & rsize)
			: 
				n ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				bitspern ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				bytespern ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				numsyms ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				dictbytes ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				cachelinebytes ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				paybytes ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				byterunthres ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				symsperblock ( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
				RL(in,rsize),
				ICLR(in,rsize),
				n1(ICLR.n ? ICLR.rank1(ICLR.n-1) : 0),
				n0(ICLR.n ? ICLR.rank0(ICLR.n-1) : 0),
				avg(n0/static_cast<double>(n1-1.0)),
				rl(1.0+avg),
				runsperblock ( static_cast<uint64_t>(std::floor((6.0*64.0)/rl)) ),
				ilog(::libmaus2::math::ilog(runsperblock)),
				downrunsperblock(1ull << ilog),
				ISS(ICLR,ilog),
				numlogicalblocks( (n + symsperblock -1 ) / symsperblock )
			{
				rsize += sizeof(n);
				rsize += sizeof(bitspern);
				rsize += sizeof(bytespern);
				rsize += sizeof(numsyms);
				rsize += sizeof(dictbytes);
				rsize += sizeof(cachelinebytes);
				rsize += sizeof(paybytes);
				rsize += sizeof(byterunthres);
				rsize += sizeof(symsperblock);
			}
			
			void checkSelect() const
			{
				std::cerr << "checking next1na...";
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t r = 0; r < static_cast<int64_t>(ISS.n1-1); ++r )
				{
					uint64_t const i1 = ISS.select1(r);
					uint64_t const i2 = ISS.next1NotAdjacent(i1);
					assert ( i2 == ISS.select1(r+1) );
				}
				std::cerr << "done." << std::endl;
				
				std::cerr << "comparing to slower...";
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t r = 0; r < static_cast<int64_t>(ISS.n1); ++r )
				{
					uint64_t i1 = ISS.select1(r);
					uint64_t ir = ICLR.select1(r);
					// std::cerr << "got " << i1 << " expected " << ir << std::endl;
					assert ( i1 == ir );
				}
				std::cerr << "done." << std::endl;
			}
		};

		template<typename _base_type>
		struct RLIndexTemplate : public _base_type
		{
			typedef _base_type base_type;
		
			typedef RLIndexTemplate<base_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			::libmaus2::autoarray::AutoArray<uint64_t> const D;

			::libmaus2::autoarray::AutoArray<uint64_t> computeDArray() const
			{
				::libmaus2::autoarray::AutoArray<uint64_t> D(base_type::numsyms);
				for ( uint64_t i = 0; i < base_type::numsyms; ++i )
					D[i] = base_type::n ? rank(i,base_type::n-1) : 0;
				return D;
			}

			::libmaus2::autoarray::AutoArray<int64_t> symbolArray() const
			{
				::libmaus2::autoarray::AutoArray<int64_t> A(base_type::numsyms);
				for ( uint64_t i = 0; i < base_type::numsyms; ++i )
					A[i] = i;
				return A;
			}

			::libmaus2::autoarray::AutoArray<int64_t> getSymbolArray() const
			{
				return symbolArray();
			}
			
			uint64_t getN() const
			{
				return base_type::n;
			}

			static unique_ptr_type load(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				
				if ( ! istr.is_open() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "RLIndex::load(): failed to open file " << filename << std::endl;
					se.finish();
					throw se;
				}
				
				unique_ptr_type ptr(new this_type(istr));

				if ( ! istr )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "RLIndex::load(): failed to load file " << filename << std::endl;
					se.finish();
					throw se;
				}
				
				return UNIQUE_PTR_MOVE(ptr);
			}

			RLIndexTemplate(std::istream & in) : base_type(in), D(computeDArray()) {}
			RLIndexTemplate(std::istream & in, uint64_t & rsize) : base_type(in,rsize), D(computeDArray()) {}

			static unique_ptr_type construct(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				unique_ptr_type ptr(new this_type(istr));
				return UNIQUE_PTR_MOVE(ptr);
			}


			/*
			 * get low count for symbol in block
			 */
			uint64_t getBlockSymLow(uint64_t const blockid, uint64_t const sym) const
			{
				uint64_t const blockoff = base_type::logicalBlockOffset(blockid);

				uint64_t cnt = 0;
				uint8_t const * p = base_type::RL.begin() + blockoff + sym * base_type::bytespern;
				for ( uint64_t i = 0; i < (base_type::bytespern<<3); i += 8 )
					cnt += static_cast<uint64_t>(*(p++)) << i;

				return cnt;
			}
			
			/*
			 * access
			 */
			int64_t operator[](uint64_t r) const
			{
				uint64_t const logicalBlock = r / base_type::symsperblock;
				r -= logicalBlock*base_type::symsperblock;
				uint64_t off = base_type::logicalBlockOffset(logicalBlock);
				uint8_t const * p = base_type::RL.begin()+off+base_type::dictbytes;

				uint64_t rl;
				while ( r >= (rl=((*p)&0x1F)) )
					r -= rl, ++p;
				
				return (*p) >> 5;
			}
			
			uint64_t loadValueLE(uint8_t const * p) const
			{
				switch ( base_type::bytespern )
				{
					case 1: return ::libmaus2::util::loadValueLE1(p);
					case 2: return ::libmaus2::util::loadValueLE2(p);
					case 3: return ::libmaus2::util::loadValueLE3(p);
					case 4: return ::libmaus2::util::loadValueLE4(p);
					case 5: return ::libmaus2::util::loadValueLE5(p);
					default: assert(0);
				}
			}

			template<typename iterator_D>
			std::pair<uint64_t,uint64_t> rankm(
				uint64_t const qsym, 
				uint64_t l,
				uint64_t r,
				iterator_D const & D) const
			{
				uint64_t const ll = rankm(qsym,l); 
				uint64_t const rr = rankm(qsym,r); 
				return std::pair<uint64_t,uint64_t>(D[qsym]+ll,D[qsym]+rr);
			}

			uint64_t rankm(uint64_t const qsym, uint64_t r) const
			{
				//std::cerr << "r=" << r << std::endl;
				uint64_t const logicalBlock = r / base_type::symsperblock;
				r -= logicalBlock*base_type::symsperblock;
				//std::cerr << "logical block=" << logicalBlock << " r=" << r << std::endl;
				uint64_t off = base_type::logicalBlockOffset(logicalBlock);
				//std::cerr << "block offset=" << off << std::endl;
				uint8_t const * p = base_type::RL.begin()+off;

		#if defined(_MSC_VER) || defined(__MINGW32__)
				uint64_t * C = reinterpret_cast<uint64_t *>(_alloca( base_type::numsyms * sizeof(uint64_t) ));
		#else
				uint64_t * C = reinterpret_cast<uint64_t *>(alloca( base_type::numsyms * sizeof(uint64_t) ));
		#endif

				p += base_type::bytespern * qsym;
				C[qsym] = loadValueLE(p);
				p += base_type::bytespern * (base_type::numsyms-qsym);
						
				uint64_t rl;
				while ( r >= (rl=((*p)&0x1F)) )
				{
					int64_t const sym = (*p) >> 5;
					C[sym] += rl;
					r -= rl;
					++p;
				}
				
				// assert ( r < rl );
				int64_t const sym = (*p) >> 5;
				// uint64_t const rank = C[sym] + (r+1);
				// uint64_t const rankm = C[sym] + r;
				C[sym] += r;
				
				return C[qsym];
			}

			template<typename iterator_C>
			void rankmTemplate(uint64_t r, iterator_C & C) const
			{
				//std::cerr << "r=" << r << std::endl;
				uint64_t const logicalBlock = r / base_type::symsperblock;
				r -= logicalBlock*base_type::symsperblock;
				//std::cerr << "logical block=" << logicalBlock << " r=" << r << std::endl;
				uint64_t off = base_type::logicalBlockOffset(logicalBlock);
				//std::cerr << "block offset=" << off << std::endl;
				uint8_t const * p = base_type::RL.begin()+off;

				::libmaus2::util::LoadMultipleValuesCall::loadMultipleValues(p,C,base_type::numsyms,base_type::bytespern);
				p += base_type::dictbytes;
						
				uint64_t rl;
				while ( r >= (rl=((*p)&0x1F)) )
				{
					int64_t const sym = (*p) >> 5;
					C[sym] += rl;
					r -= rl;
					++p;
				}
				
				assert ( p < base_type::RL.end() );
				
				int64_t const sym = (*p) >> 5;
				C[sym] += r;		
			}
			
			struct UPairFirst
			{
				typedef std::pair<uint64_t,uint64_t> upair;
				upair * const P;
				
				private:
				void operator=(UPairFirst const &) {}

				public:
				UPairFirst(upair * const rP) : P(rP) {}
				
				uint64_t & operator[](uint64_t const i) { return P[i].first; }
				uint64_t const & operator[](uint64_t const i) const { return P[i].first; }
			};

			struct UPairSecond
			{
				typedef std::pair<uint64_t,uint64_t> upair;
				upair * const P;

				private:
				void operator=(UPairFirst const &) {}

				public:
				UPairSecond(upair * const rP) : P(rP) {}
				
				uint64_t & operator[](uint64_t const i) { return P[i].second; }
				uint64_t const & operator[](uint64_t const i) const { return P[i].second; }
			};
			
			void rankm(uint64_t const l, uint64_t const r, std::pair<uint64_t,uint64_t> * const C) const
			{
				UPairFirst up1(C); rankmTemplate(l,up1);
				UPairSecond up2(C); rankmTemplate(r,up2);
			}

			void stepDivArray(uint64_t const l, uint64_t const r, std::pair<uint64_t,uint64_t> * const C) const
			{
				if ( l < r )
				{
					UPairFirst  up1(C); rankmTemplate(l,up1);
					UPairSecond up2(C); rankmTemplate(r,up2);
					for ( uint64_t i = 0; i < base_type::numsyms; ++i )
						C[i].second -= C[i].first;
				}
				else
				{
					for ( uint64_t i = 0; i < base_type::numsyms; ++i )
						C[i].first = C[i].second = 0;
				}
			}
			
			template<typename iterator_D, typename iterator_P>
			uint64_t multiStepSortedNoEmpty(uint64_t const l, uint64_t const r, iterator_D D, iterator_P P) const
			{
				return multiStep<iterator_D,iterator_P,true,false>(l,r,D,P);
			}

			template<typename iterator_D, typename iterator_P, bool const sort /* = true */, bool includeEmpty /* = false */>
			uint64_t multiStep(uint64_t const l, uint64_t const r, iterator_D D, iterator_P P) const
			{
				typedef std::pair<uint64_t,uint64_t> upair;
#if defined(_MSC_VER) || defined(__MINGW32__)
				upair * C = reinterpret_cast<upair *>(_alloca( base_type::numsyms * sizeof(upair) ));
#else
				upair * C = reinterpret_cast<upair *>(alloca( base_type::numsyms * sizeof(upair) ));
#endif

				UPairFirst  up1(C); rankmTemplate(l,up1);
				UPairSecond up2(C); rankmTemplate(r,up2);
				
				iterator_P PP = P;
				for ( uint64_t i = 0; i < base_type::numsyms; ++i )
					if ( includeEmpty || C[i].second-C[i].first )
						*(PP++) = std::pair< int64_t,std::pair<uint64_t,uint64_t> > (i,std::pair<uint64_t,uint64_t>(D[i] + C[i].first,D[i] + C[i].second));

				return PP-P;				
			}

			template<typename iterator_D, typename callback_type>
			void multiRankCallBack(
				uint64_t const l, 
				uint64_t const r, 
				iterator_D D, 
				callback_type & cb) const
			{
				multiRankCallBackTemplate<iterator_D,callback_type,false>(l,r,D,cb);
			}

			template<typename iterator_D, typename callback_type, bool includeEmpty /* = false */>
			void multiRankCallBackTemplate(uint64_t const l,  uint64_t const r,  iterator_D D,  callback_type & cb) const
			{
				typedef std::pair<uint64_t,uint64_t> upair;
#if defined(_MSC_VER) || defined(__MINGW32__)
				upair * C = reinterpret_cast<upair *>(_alloca( base_type::numsyms * sizeof(upair) ));
#else
				upair * C = reinterpret_cast<upair *>(alloca( base_type::numsyms * sizeof(upair) ));
#endif

				UPairFirst  up1(C); rankmTemplate(l,up1);
				UPairSecond up2(C); rankmTemplate(r,up2);
				
				for ( uint64_t i = 0; i < base_type::numsyms; ++i )
					if ( includeEmpty || C[i].second-C[i].first )
						cb(i,D[i] + C[i].first,D[i] + C[i].second);
			}

			template<typename lcp_iterator, typename queue_type, typename iterator_D>
			inline uint64_t multiRankLCPSet(
				uint64_t const l, 
				uint64_t const r, 
				iterator_D D,
				lcp_iterator WLCP,
				typename std::iterator_traits<lcp_iterator>::value_type const unset,
				typename std::iterator_traits<lcp_iterator>::value_type const cur_l,
				queue_type * PQ1
				) const
			{
				typedef std::pair<uint64_t,uint64_t> upair;
#if defined(_MSC_VER) || defined(__MINGW32__)
				upair * C = reinterpret_cast<upair *>(_alloca( base_type::numsyms * sizeof(upair) ));
#else
				upair * C = reinterpret_cast<upair *>(alloca( base_type::numsyms * sizeof(upair) ));
#endif

				UPairFirst  up1(C); rankmTemplate(l,up1);
				UPairSecond up2(C); rankmTemplate(r,up2);
				
				uint64_t s = 0;
				for ( uint64_t i = 0; i < base_type::numsyms; ++i )
					if ( C[i].second-C[i].first )
					{
						uint64_t const sp = D[i] + C[i].first;
						uint64_t const ep = D[i] + C[i].second;

						if ( WLCP[ep] == unset )
						{
							WLCP[ep] = cur_l;
							PQ1->push_back( std::pair<uint64_t,uint64_t>(sp,ep) );
							s++;
						}
					}

				return s;
			}

			template<typename lcp_iterator, typename queue_type, typename iterator_D>
			inline uint64_t multiRankLCPSetLarge(
				uint64_t const l, 
				uint64_t const r, 
				iterator_D D,
				lcp_iterator & WLCP,
				uint64_t const cur_l,
				queue_type * PQ1
				) const
			{
				typedef std::pair<uint64_t,uint64_t> upair;
#if defined(_MSC_VER) || defined(__MINGW32__)
				upair * C = reinterpret_cast<upair *>(_alloca( base_type::numsyms * sizeof(upair) ));
#else
				upair * C = reinterpret_cast<upair *>(alloca( base_type::numsyms * sizeof(upair) ));
#endif

				UPairFirst  up1(C); rankmTemplate(l,up1);
				UPairSecond up2(C); rankmTemplate(r,up2);
				
				uint64_t s = 0;
				for ( uint64_t i = 0; i < base_type::numsyms; ++i )
					if ( C[i].second-C[i].first )
					{
						uint64_t const sp = D[i] + C[i].first;
						uint64_t const ep = D[i] + C[i].second;

						if ( WLCP.isUnset(ep) )
	                                        {
       		                                        WLCP.set(ep, cur_l);
               		                                PQ1->push_back( std::pair<uint64_t,uint64_t>(sp,ep) );
               		                                s++;
						}				
					}

				return s;
			
			}
			
			std::map<int64_t,uint64_t> enumerateSymbolsInRange(uint64_t const l, uint64_t const r) const
			{
				typedef std::pair<uint64_t,uint64_t> upair;
#if defined(_MSC_VER) || defined(__MINGW32__)
				upair * C = reinterpret_cast<upair *>(_alloca( base_type::numsyms * sizeof(upair) ));
#else
				upair * C = reinterpret_cast<upair *>(alloca( base_type::numsyms * sizeof(upair) ));
#endif
				
				rankm(l,r,C);
				
				std::map<int64_t,uint64_t> M;
				for ( uint64_t i = 0; i < base_type::numsyms; ++i )
					if ( C[i].second-C[i].first )
						M[i] = C[i].second-C[i].first;

				return M;
			}
			
			uint64_t rank(uint64_t const qsym, uint64_t r) const
			{
				//std::cerr << "r=" << r << std::endl;
				uint64_t const logicalBlock = r / base_type::symsperblock;
				r -= logicalBlock*base_type::symsperblock;
				//std::cerr << "logical block=" << logicalBlock << " r=" << r << std::endl;
				uint64_t off = base_type::logicalBlockOffset(logicalBlock);
				//std::cerr << "block offset=" << off << std::endl;
				uint8_t const * p = base_type::RL.begin()+off;

		#if defined(_MSC_VER) || defined(__MINGW32__)
				uint64_t * C = reinterpret_cast<uint64_t *>(_alloca( base_type::numsyms * sizeof(uint64_t) ));
		#else
				uint64_t * C = reinterpret_cast<uint64_t *>(alloca( base_type::numsyms * sizeof(uint64_t) ));
		#endif

				p += base_type::bytespern * qsym;
				C[qsym] = loadValueLE(p);
				p += base_type::bytespern * (base_type::numsyms-qsym);
						
				uint64_t rl;
				while ( r >= (rl=((*p)&0x1F)) )
				{
					int64_t const sym = (*p) >> 5;
					C[sym] += rl;
					r -= rl;
					++p;
				}
				
				int64_t const sym = (*p) >> 5;
				// uint64_t const rank = C[sym] + (r+1);
				// uint64_t const rankm = C[sym] + r;
				C[sym] += (r+1);
				
				return C[qsym];
			}

			std::pair<int64_t,uint64_t> inverseSelect(uint64_t r) const
			{
				//std::cerr << "r=" << r << std::endl;
				uint64_t const logicalBlock = r / base_type::symsperblock;
				r -= logicalBlock*base_type::symsperblock;
				//std::cerr << "logical block=" << logicalBlock << " r=" << r << std::endl;
				uint64_t off = base_type::logicalBlockOffset(logicalBlock);
				//std::cerr << "block offset=" << off << std::endl;
				uint8_t const * p = base_type::RL.begin()+off;

#if defined(_MSC_VER) || defined(__MINGW32__)
				uint64_t * C = reinterpret_cast<uint64_t *>(_alloca( base_type::numsyms * sizeof(uint64_t) ));
#else
				uint64_t * C = reinterpret_cast<uint64_t *>(alloca( base_type::numsyms * sizeof(uint64_t) ));
#endif

				::libmaus2::util::LoadMultipleValuesCall::loadMultipleValues(p,C,base_type::numsyms,base_type::bytespern);
				p += base_type::dictbytes;

				uint64_t rl;
				while ( r >= (rl=((*p)&0x1F)) )
				{
					int64_t const sym = (*p) >> 5;
					C[sym] += rl;
					r -= rl;
					++p;
				}
				
				int64_t const sym = (*p) >> 5;
				// uint64_t const rank = C[sym] + (r+1);
				uint64_t const rankm = C[sym] + r;
				
				return std::pair<int64_t,uint64_t>(sym,rankm);
			}


			void rangeEnumZero(uint64_t l, uint64_t const e, PairPushBuffer & P) const
			{
				if ( l >=e )
					return;
			
				// find block for l
				uint64_t const logicalBlockLow  = l/base_type::symsperblock;
				// find block for e-1
				uint64_t const logicalBlockHigh = (e-1)/base_type::symsperblock+1;
				uint64_t skip = l-logicalBlockLow*base_type::symsperblock;

#if defined(_MSC_VER) || defined(__MINGW32__)
				uint64_t * C = reinterpret_cast<uint64_t *>(_alloca( base_type::numsyms * sizeof(uint64_t) ));
#else
				uint64_t * C = reinterpret_cast<uint64_t *>(alloca( base_type::numsyms * sizeof(uint64_t) ));
#endif

				P.reset();

				// first block
				if ( logicalBlockLow != logicalBlockHigh )
				{
					uint8_t const * p = base_type::RL.begin() + base_type::logicalBlockOffset(logicalBlockLow);
					C[0] = loadValueLE(p);
					p += base_type::dictbytes;

					uint64_t blockdecsyms = 0;
					uint64_t rl;
					while ( skip >= (rl=((*p)&0x1F)) )
					{
						int64_t const sym = (*p) >> 5;
						C[sym] += rl;
						skip -= rl;
						blockdecsyms += rl;
						++p;
					}
					
					int64_t const sym = (*p) >> 5;
					assert ( skip < rl );
					rl -= skip;
					C[sym] += skip;
					blockdecsyms += skip;
				
					assert ( rl );
					
					if ( ! sym )
					{
						while ( rl && (l != e) )
						{
							rl--;
							blockdecsyms++;
							assert ( ! (*this)[l] );
							P.push(l++,C[0]++);
						}
					}
					else
					{
						blockdecsyms += rl;
						l += rl;
					}
					
					while ( (l < e) && (blockdecsyms != base_type::symsperblock) )
					{
						++p;
						int64_t const sym = (*p) >> 5;
						uint64_t rl = (*p)&0x1F;

						if ( ! sym )
						{
							while ( rl && (l < e) )
							{
								rl--;
								blockdecsyms++;
								assert ( ! (*this)[l] );
								P.push(l++,C[0]++);
							}
						}
						else
						{
							blockdecsyms += rl;
							l += rl;
						}
					}
				}

				for ( uint64_t lblock = logicalBlockLow+1; lblock < logicalBlockHigh; ++lblock )
				{
					uint8_t const * p = base_type::RL.begin() + base_type::logicalBlockOffset(lblock);
					p += base_type::dictbytes;
					uint64_t blockdecsyms = 0;

					while ( (l < e) && (blockdecsyms != base_type::symsperblock) )
					{
						int64_t const sym = (*p) >> 5;
						uint64_t rl = (*p)&0x1F;

						if ( ! sym )
						{
							while ( rl && (l < e) )
							{
								rl--;
								blockdecsyms++;
								P.push(l++,C[0]++);
							}
						}
						else
						{
							blockdecsyms += rl;
							l += rl;
						}

						++p;
					}
				}
			}
			
			/**
			 * select block containing rank for symbol
			 **/
			uint64_t selectBlock(uint64_t const sym, uint64_t const rank) const
			{
				uint64_t left = 0; 
				uint64_t right = base_type::numlogicalblocks;
				
				while ( right-left > 1 )
				{
					uint64_t const mid = (left+right)>>1;
					uint64_t const midlow = getBlockSymLow(mid,sym);
					
					if ( rank < midlow )
						right = mid;
					else
						left = mid;
				}
				
				assert ( right-left == 1 );
				
				return left;
			}

			/**
			 * Return the position of the ii'th (zero based) sym symbol.
			 **/
			uint64_t select(int64_t const sym, uint64_t ii) const
			{
				uint64_t const selblock = selectBlock(sym,ii);
				uint64_t const off = base_type::logicalBlockOffset(selblock);
				uint8_t const * p = base_type::RL.begin()+off+sym*base_type::bytespern;		
				uint64_t const cnt = loadValueLE(p);
				p += base_type::bytespern*(base_type::numsyms-sym);
				
				uint64_t pos = selblock * base_type::symsperblock;
				assert ( ii >= cnt );
				ii -= cnt;
				
				// now perform linear scan until we reached the correct position
				while ( ((*p) >> 5 ) != sym )
					pos += (*(p++)) & 0x1FULL;
				
				while ( true )
				{
					uint64_t const srl = ((*(p++)) & 0x1FULL);
					if ( ii < srl )
					{
						pos += ii;
						return pos;
					}
					else
					{
						pos += srl;
						ii -= srl;
					}
					
					while ( ((*p) >> 5 ) != sym )
						pos += (*(p++)) & 0x1FULL;
				}
			}

		};

		typedef RLIndexTemplate < RLIndexDataBase > RLIndex;
		typedef RLIndexTemplate < RLSimpleIndexDataBase > RLSimpleIndex;
	}
}
#endif
