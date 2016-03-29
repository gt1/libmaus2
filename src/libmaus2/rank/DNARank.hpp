/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_RANK_DNARANK_HPP)
#define LIBMAUS2_RANK_DNARANK_HPP

#include <libmaus2/huffman/RLDecoder.hpp>
#include <libmaus2/rank/popcnt.hpp>
#include <libmaus2/fm/BidirectionalIndexInterval.hpp>
#include <sstream>

#include <libmaus2/random/Random.hpp>
#include <libmaus2/parallel/PosixSpinLock.hpp>

#include <libmaus2/util/Queue.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARank
		{
			typedef DNARank this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef libmaus2::autoarray::AutoArray<uint64_t,libmaus2::autoarray::alloc_type_memalign_cacheline> D_type;

			#define LIBMAUS2_RANK_DNARANK_SIGMA 4

			static unsigned int getSigma() { return LIBMAUS2_RANK_DNARANK_SIGMA; }
			static unsigned int getLogSigma() { return 2; }
			static unsigned int getBitsPerByte() { return 8; }
			static unsigned int getCacheLineSize() { return 64; }
			static unsigned int getWordsPerBlock() { return getCacheLineSize()/sizeof(uint64_t); }
			static unsigned int getDictWordsPerBlock() { return (getSigma()-1); }
			static unsigned int getDataWordsPerBlock() { return getWordsPerBlock() - getDictWordsPerBlock(); }
			static unsigned int getDataBytesPerBlock() { return getDataWordsPerBlock() * sizeof(uint64_t); }
			static unsigned int getBasesPerWord() { return (getBitsPerByte()*sizeof(uint64_t))/getLogSigma(); }
			static unsigned int getDataBasesPerBlock() { return getDataBytesPerBlock() * (getBitsPerByte()/getLogSigma()); }

			private:
			uint64_t n;
			libmaus2::autoarray::AutoArray<uint64_t,libmaus2::autoarray::alloc_type_memalign_cacheline> B;
			D_type D;

			void computeD()
			{
				D.resize(getSigma()+1);
				rankm(n,D.begin());
				D.prefixSums();
			}

			static uint64_t symcnt(uint64_t const w, uint64_t const mask)
			{
				uint64_t const wmask = w ^ mask;
				return libmaus2::rank::PopCnt8<sizeof(long)>::popcnt8((~(wmask | (wmask>>1))) & 0x5555555555555555ULL);
			}

			static uint64_t symcnt(uint64_t const w, uint64_t const mask, uint64_t const omask)
			{
				uint64_t const wmask = w ^ mask;
				return libmaus2::rank::PopCnt8<sizeof(long)>::popcnt8(((~(wmask | (wmask>>1))) & 0x5555555555555555ULL)&omask);
			}

			void testFromRunLength(std::vector<std::string> const & rl, uint64_t const numthreads) const
			{
				// total number of blocks
				uint64_t const numblocks = (n + getDataBasesPerBlock() - 1)/getDataBasesPerBlock();
				// blocks per thread
				uint64_t const blocksperthread = (numblocks + numthreads-1)/numthreads;
				// number of thread packages
				int64_t const numpacks = (numblocks + blocksperthread-1)/blocksperthread;

				uint64_t r_a[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t r_b[LIBMAUS2_RANK_DNARANK_SIGMA];

				rankm(n,&r_a[0]);
				rank(n-1,&r_b[0]);

				for ( uint64_t i = 0; i < getSigma(); ++i )
					assert ( r_a[i] == r_b[i] );
				assert ( std::accumulate(&r_a[0],&r_a[getSigma()],0ull) == n );

				std::cerr << "checking...";
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( int64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const block_low = t*blocksperthread;
					uint64_t const block_high = std::min(block_low + blocksperthread,numblocks);
					uint64_t const block_base_low = block_low * getDataBasesPerBlock();
					uint64_t const block_base_high = std::min(n,block_high * getDataBasesPerBlock());
					assert ( block_high > block_low );
					libmaus2::huffman::RLDecoder rldec(rl,block_base_low);

					for ( uint64_t i = block_base_low; i < block_base_high; ++i )
					{
						int64_t const sym = rldec.decode();
						assert ( sym >= 0 );
						bool const ok = (sym == (*this)[i]);
						if ( ! ok )
						{
							std::cerr << "[D] expecting " << sym << " got " << (*this)[i] << " index " << i << " t " << t << std::endl;
							assert ( ok );
						}
					}
				}
				std::cerr << "done." << std::endl;

				libmaus2::autoarray::AutoArray<char> C(n,false);
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( int64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const block_low = t*blocksperthread;
					uint64_t const block_high = std::min(block_low + blocksperthread,numblocks);
					uint64_t const block_base_low = block_low * getDataBasesPerBlock();
					uint64_t const block_base_high = std::min(n,block_high * getDataBasesPerBlock());
					assert ( block_high > block_low );
					libmaus2::huffman::RLDecoder rldec(rl,block_base_low);

					for ( uint64_t i = block_base_low; i < block_base_high; ++i )
					{
						int64_t const sym = rldec.decode();
						assert ( sym >= 0 );
						C[i] = sym;
					}
				}

				libmaus2::timing::RealTimeClock rtc; rtc.start();
				std::cerr << "checking rankm...";
				uint64_t acc[LIBMAUS2_RANK_DNARANK_SIGMA] = {0,0,0,0};
				uint64_t racc[LIBMAUS2_RANK_DNARANK_SIGMA];
				for ( uint64_t i = 0; i < n; ++i )
				{
					rankm(i,&racc[0]);
					bool ok = true;
					for ( unsigned int j = 0; j < getSigma(); ++j )
					{
						ok = ok && ( acc[j] == racc[j] );
					}
					if ( ! ok )
					{
						std::cerr << "failed at " << i << "/" << n << std::endl;

						for ( uint64_t j = 0; j < getSigma(); ++j )
							std::cerr << acc[j] << "\t" << racc[j] << std::endl;

						assert ( ok );
					}

					int64_t const sym = C[i];


					acc [ sym ] ++;

					rank(i,&racc[0]);
					ok = true;
					for ( unsigned int j = 0; j < getSigma(); ++j )
					{
						ok = ok && ( acc[j] == racc[j] );
					}
					if ( ! ok )
					{
						std::cerr << "rank failed at " << i << "/" << n << std::endl;

						for ( uint64_t j = 0; j < getSigma(); ++j )
							std::cerr << acc[j] << "\t" << racc[j] << std::endl;

						assert ( ok );
					}

					if ( i % (1024*1024*16) == 0 )
						std::cerr << "(" << static_cast<double>(i+1)/n << ")";
				}
				std::cerr << "done, " << (n/rtc.getElapsedSeconds()) << std::endl;

				std::vector<uint64_t> RV;
				for ( uint64_t i = 0; i < (256*1024*1024); ++i )
					RV.push_back(libmaus2::random::Random::rand64() % n);
				std::cerr << "checking random...";
				rtc.start();
				for ( uint64_t i = 0; i < RV.size(); ++i )
					rankm(RV[i],&racc[0]);
				std::cerr << "done, " << (RV.size()/rtc.getElapsedSeconds()) << std::endl;

				std::cerr << "[V] Checking select...";
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( int64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const block_low = t*blocksperthread;
					uint64_t const block_high = std::min(block_low + blocksperthread,numblocks);
					uint64_t const block_base_low = block_low * getDataBasesPerBlock();
					uint64_t const block_base_high = std::min(n,block_high * getDataBasesPerBlock());
					assert ( block_high > block_low );
					uint64_t R[LIBMAUS2_RANK_DNARANK_SIGMA];

					for ( uint64_t i = block_base_low; i < block_base_high; ++i )
					{
						int64_t const sym = C[i];
						rank(i,&R[0]);
						assert ( select(sym,R[sym]-1) == i );
					}
				}
				std::cerr << "done." << std::endl;
			}

			DNARank() : n(0), B()
			{
			}

			void deserialise(std::istream & in)
			{
				n = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				B.deserialize(in);
			}

			public:
			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(out,n);
				B.serialize(out);
				return out;
			}

			template<typename iterator>
			void extract(iterator it, uint64_t i, uint64_t s) const
			{
				// data base ptr
				uint64_t const * P = B.begin();

				// skip blocks until we reach block containing i
				uint64_t const blockskip = i / getDataBasesPerBlock();
				i -= blockskip * getDataBasesPerBlock();
				P += (blockskip * getWordsPerBlock());

				// if s and i
				if ( s && i )
				{
					// skip over index words
					P += getDictWordsPerBlock();

					// skip over words until we reach word containing i
					uint64_t symsinblock = getDataBasesPerBlock();
					uint64_t wordskip = i / getBasesPerWord();
					i -= wordskip * getBasesPerWord();
					symsinblock -= wordskip * getBasesPerWord();
					P += wordskip;

					// while block contains more data and we want more
					while ( s && symsinblock )
					{
						uint64_t w = *(P++);

						uint64_t symsinword = getBasesPerWord();

						assert ( symsinword > i );
						unsigned int const tocopy = std::min(s,symsinword-i);
						s -= tocopy;
						w >>= (i<<1);
						i = 0;

						iterator ite = it + tocopy;
						while ( it != ite )
						{
							*(it++) = w & 0x3;
							w >>= 2;
						}

						symsinblock -= getBasesPerWord();
					}
				}

				assert ( (!s) || (i == 0) );

				while ( s )
				{
					P += getDictWordsPerBlock();
					uint64_t symsinblock = getDataBasesPerBlock();

					while ( s && symsinblock )
					{
						uint64_t w = *(P++);

						unsigned int const tocopy = std::min(s,static_cast<uint64_t>(getBasesPerWord()));
						s -= tocopy;

						iterator ite = it + tocopy;
						while ( it != ite )
						{
							*(it++) = w & 0x3;
							w >>= 2;
						}

						symsinblock -= getBasesPerWord();
					}
				}
			}

			unsigned int operator[](uint64_t i) const
			{
				if ( i >= n )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::rank::DNARank::operator["<<i<<"]: index is out of range" << std::endl;
					lme.finish();
					throw lme;
				}

				uint64_t const * P = B.begin();
				uint64_t const blockskip = i / getDataBasesPerBlock();
				i -= blockskip * getDataBasesPerBlock();
				P += (blockskip * getWordsPerBlock()) + getDictWordsPerBlock();
				uint64_t wordskip = i / getBasesPerWord();
				i -= wordskip * getBasesPerWord();
				P += wordskip;
				return ((*P) >> (i<<1)) & (getSigma()-1);
			}

			uint64_t size() const
			{
				return n;
			}

			uint64_t rankm(unsigned int const c, uint64_t i) const
			{
				switch ( c )
				{
					case 0:  return rankmTemplate<0>(i); break;
					case 1:  return rankmTemplate<1>(i); break;
					case 2:  return rankmTemplate<2>(i); break;
					default: return rankmTemplate<3>(i); break;
				}
			}

			template<int sym>
			uint64_t rankmTemplate(uint64_t i) const
			{
				if ( sym < 3 )
				{
					uint64_t const * P = B.begin();
					uint64_t const blockskip = i / getDataBasesPerBlock();
					i -= blockskip * getDataBasesPerBlock();
					P += (blockskip * getWordsPerBlock());

					uint64_t r = P[sym];
					P += getSigma()-1;

					uint64_t wordskip = i / getBasesPerWord();

					i -= wordskip * getBasesPerWord();

					uint64_t w;
					while ( wordskip-- )
					{
						w = *(P++);
						switch ( sym )
						{
							case 0: r += symcnt(w,0x0000000000000000ULL); break;
							case 1: r += symcnt(w,0x5555555555555555ULL); break;
							case 2: r += symcnt(w,0xAAAAAAAAAAAAAAAAULL); break;
						}
					}

					if ( i )
					{
						w = *(P++);
						uint64_t const omask = (1ull << (getLogSigma()*i))-1;

						switch ( sym )
						{
							case 0: r += symcnt(w,0x0000000000000000ULL,omask); break;
							case 1: r += symcnt(w,0x5555555555555555ULL,omask); break;
							case 2: r += symcnt(w,0xAAAAAAAAAAAAAAAAULL,omask); break;
						}
					}

					return r;
				}
				else
				{
					uint64_t R[LIBMAUS2_RANK_DNARANK_SIGMA];
					rankm(i,&R[0]);
					return R[3];
				}
			}

			void rankm(uint64_t i, uint64_t * r) const
			{
				uint64_t const * P = B.begin();
				uint64_t const blockskip = i / getDataBasesPerBlock();
				i -= blockskip * getDataBasesPerBlock();
				P += (blockskip * getWordsPerBlock());

				for ( unsigned int i = 0; i < getSigma()-1; ++i )
					r[i] = *(P++);

				uint64_t wordskip = i / getBasesPerWord();

				r[(getSigma()-1)] = (blockskip * getDataBasesPerBlock()) + (wordskip*getBasesPerWord());

				i -= wordskip * getBasesPerWord();

				uint64_t w;
				while ( wordskip-- )
				{
					w = *(P++);
					r[0] += symcnt(w,0x0000000000000000ULL);
					r[1] += symcnt(w,0x5555555555555555ULL);
					r[2] += symcnt(w,0xAAAAAAAAAAAAAAAAULL);
				}

				r[getSigma()-1] -= (r[0]+r[1]+r[2]);

				if ( i )
				{
					w = *(P++);
					uint64_t const omask = (1ull << (getLogSigma()*i))-1;
					uint64_t const r0 = symcnt(w,0x0000000000000000ULL,omask);
					uint64_t const r1 = symcnt(w,0x5555555555555555ULL,omask);
					uint64_t const r2 = symcnt(w,0xAAAAAAAAAAAAAAAAULL,omask);
					r[0] += r0;
					r[1] += r1;
					r[2] += r2;
					r[3] += i-(r0+r1+r2);
				}
			}

			void multiLF(uint64_t const l, uint64_t const r, uint64_t * const R0, uint64_t * const R1) const
			{
				rankm(l,&R0[0]);
				rankm(r,&R1[0]);

				for ( unsigned int i = 0; i < getSigma(); ++i )
				{
					R0[i] += D[i];
					R1[i] += D[i];
				}
			}

			template<typename lcp_iterator, typename queue_type, typename iterator_D>
			inline uint64_t multiRankLCPSet(
				uint64_t const l,
				uint64_t const r,
				iterator_D /* D */,
				lcp_iterator WLCP,
				typename std::iterator_traits<lcp_iterator>::value_type const unset,
				typename std::iterator_traits<lcp_iterator>::value_type const cur_l,
				queue_type * PQ1
				) const
			{
				uint64_t R0[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t R1[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t s = 0;

				multiLF(l,r,&R0[0],&R1[0]);

				for ( uint64_t sym = 0; sym < LIBMAUS2_RANK_DNARANK_SIGMA; ++sym )
				{
					uint64_t const sp = R0[sym];
					uint64_t const ep = R1[sym];

					if ( (ep-sp) && WLCP[ep] == unset )
					{
						WLCP[ep] = cur_l;
						PQ1->push_back(std::pair<uint64_t,uint64_t>(sp,ep));
						s += 1;
					}
				}

				return s;
			}

			template<typename lcp_iterator, typename queue_type, typename iterator_D>
			inline uint64_t multiRankLCPSetLarge(
				uint64_t const l,
				uint64_t const r,
				iterator_D /* D */,
				lcp_iterator & WLCP,
				uint64_t const cur_l,
				queue_type * PQ1
				) const
			{
				uint64_t R0[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t R1[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t s = 0;

				multiLF(l,r,&R0[0],&R1[0]);

				for ( uint64_t sym = 0; sym < LIBMAUS2_RANK_DNARANK_SIGMA; ++sym )
				{
					uint64_t const sp = R0[sym];
					uint64_t const ep = R1[sym];

					if ( (ep-sp) && WLCP.isUnset(ep) )
					{
						WLCP.set(ep, cur_l);
						PQ1->push_back(std::pair<uint64_t,uint64_t>(sp,ep));
						s += 1;
					}
				}

				return s;
			}

			uint64_t getN() const
			{
				return n;
			}

			void multiRank(uint64_t const l, uint64_t const r, uint64_t * const R0, uint64_t * const R1) const
			{
				rankm(l,&R0[0]);
				rankm(r,&R1[0]);
			}

			uint64_t singleStep(uint64_t const l, unsigned int const sym) const
			{
				return D[sym] + rankm(sym,l);
			}

			std::pair<uint64_t,uint64_t> singleSymbolLF(uint64_t const l, uint64_t const r, unsigned int const sym) const
			{
				std::pair<uint64_t,uint64_t> const P(D[sym] + rankm(sym,l),D[sym] + rankm(sym,r));
				return P;
			}

			bool hasExtension(uint64_t const l, uint64_t const r, unsigned int const sym) const
			{
				std::pair<uint64_t,uint64_t> const P = singleSymbolLF(l,r,sym);
				return P.second != P.first;
			}

			std::pair<uint64_t,uint64_t> epsilon() const
			{
				return std::pair<uint64_t,uint64_t>(0,n);
			}

			template<typename iterator>
			std::pair<uint64_t,uint64_t> backwardSearch(iterator C, uint64_t const k) const
			{
				std::pair<uint64_t,uint64_t> intv = epsilon();
				iterator E = C;
				C += k;
				while ( C != E )
					intv = backwardExtend(intv,*(--C));
				return intv;
			}

			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const * P = D.end()-1;
				while ( *(--P) > r )
				{}
				assert ( P[0] <= r );
				assert ( P[1] > r );

				return (P-D.begin());
			}

			uint64_t phi(uint64_t r) const
			{
				// get symbol in first column by r
				uint64_t const sym = sortedSymbol(r);
				r -= D[sym];
				return select(sym,r);
			}

			template<typename iterator>
			void decode(uint64_t rr, unsigned int k, iterator C) const
			{
				for ( unsigned int i = 0; i < k; ++i )
				{
					*(C++) = sortedSymbol(rr);
					rr = phi(rr);
				}
			}

			std::string decode(uint64_t rr, unsigned int k) const
			{
				std::string s(k,' ');
				decode(rr,k,s.begin());
				return s;
			}

			bool sufcmp(uint64_t r0, uint64_t r1, unsigned int k) const
			{
				for ( unsigned int i = 0; i < k; ++i )
				{
					unsigned int c0 = sortedSymbol(r0);
					unsigned int c1 = sortedSymbol(r1);
					if ( c0 != c1 )
						return false;
					r0 = phi(r0);
					r1 = phi(r1);
				}

				return true;
			}

			/**
			 * test searching for all k-mers
			 **/
			void testSearch(unsigned int k, uint64_t const numthreads) const
			{
				uint64_t const lim = 1ull << (getLogSigma()*k);

				libmaus2::autoarray::AutoArray<libmaus2::autoarray::AutoArray<char > > AC(numthreads);
				libmaus2::autoarray::AutoArray<libmaus2::autoarray::AutoArray<char > > AD(numthreads);

				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					AC[i].resize(k);
					AD[i].resize(k);
				}

				libmaus2::parallel::PosixSpinLock cerrlock;

				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
				#endif
				for ( uint64_t z = 0; z < lim; ++z )
				{
					#if defined(_OPENMP)
					unsigned int const tid = omp_get_thread_num();
					#else
					unsigned int const tid = 0;
					#endif

					char * const C = AC[tid].begin();
					char * const D = AD[tid].begin();

					for ( unsigned int i = 0; i < k; ++i )
						C[i] = (z >> (getLogSigma()*i)) & (getSigma()-1);
					std::pair<uint64_t,uint64_t> const P = backwardSearch(C,k);

					cerrlock.lock();
					std::cerr << "checking ";
					for ( uint64_t i = 0; i < k; ++i )
						std::cerr << static_cast<int>(C[i]);
					std::cerr << " [" << P.first << "," << P.second << ")" << std::endl;
					cerrlock.unlock();

					for ( uint64_t r = P.first; r < P.second; ++r )
					{
						decode(r,k,D);
						assert ( memcmp(C,D,k) == 0 );
					}

					if ( P.first )
					{
						decode(P.first-1,k,D);
						assert ( memcmp(C,D,k) != 0 );
					}
					if ( P.second < n )
					{
						decode(P.second,k,D);
						assert ( memcmp(C,D,k) != 0 );
					}
				}
			}

			std::pair<uint64_t,uint64_t> backwardExtend(std::pair<uint64_t,uint64_t> const & P, unsigned int const sym) const
			{
				return singleSymbolLF(P.first,P.second,sym);
			}

			unsigned int inverseSelect(uint64_t i, uint64_t * r) const
			{
				uint64_t const * P = B.begin();
				uint64_t const blockskip = i / getDataBasesPerBlock();
				i -= blockskip * getDataBasesPerBlock();
				P += (blockskip * getWordsPerBlock());

				for ( unsigned int i = 0; i < getSigma()-1; ++i )
					r[i] = *(P++);

				uint64_t wordskip = i / getBasesPerWord();

				r[getSigma()-1] = (blockskip * getDataBasesPerBlock()) + (wordskip*getBasesPerWord());

				i -= wordskip * getBasesPerWord();

				uint64_t w;
				while ( wordskip-- )
				{
					w = *(P++);
					r[0] += symcnt(w,0x0000000000000000ULL);
					r[1] += symcnt(w,0x5555555555555555ULL);
					r[2] += symcnt(w,0xAAAAAAAAAAAAAAAAULL);
				}

				r[getSigma()-1] -= (r[0]+r[1]+r[2]);

				w = *(P++);

				if ( i )
				{
					uint64_t const omask = (1ull << (getLogSigma()*i))-1;
					uint64_t const r0 = symcnt(w,0x0000000000000000ULL,omask);
					uint64_t const r1 = symcnt(w,0x5555555555555555ULL,omask);
					uint64_t const r2 = symcnt(w,0xAAAAAAAAAAAAAAAAULL,omask);
					r[0] += r0;
					r[1] += r1;
					r[2] += r2;
					r[3] += i-(r0+r1+r2);
				}

				return (w >> (getLogSigma()*i)) & (getSigma()-1);
			}

			std::pair<uint64_t,uint64_t> simpleLFUntilMask(uint64_t r, uint64_t const mask)
			{
				uint64_t d = 0;
				while ( r & mask )
				{
					r = simpleLF(r);
					d += 1;
				}
				return std::pair<uint64_t,uint64_t>(r,d);
			}

			/**
			 * return ISA[SA[i]-1] (assuming vector stores the respective BWT)
			 **/
			uint64_t simpleLF(uint64_t const i) const
			{
				uint64_t R[LIBMAUS2_RANK_DNARANK_SIGMA];
				unsigned int const sym = inverseSelect(i,&R[0]);
				return D[sym] + R[sym];
			}

			std::pair<uint64_t, uint64_t> extendedLF(uint64_t const i) const
			{
				uint64_t R[LIBMAUS2_RANK_DNARANK_SIGMA];
				unsigned int const sym = inverseSelect(i,&R[0]);
				return std::pair<uint64_t, uint64_t>(sym,D[sym] + R[sym]);
			}

			void rank(uint64_t i, uint64_t * r) const
			{
				unsigned int const sym = inverseSelect(i,r);
				r[sym] += 1;
			}

			uint64_t select(unsigned int const sym, uint64_t i) const
			{
				uint64_t R[LIBMAUS2_RANK_DNARANK_SIGMA];

				int64_t l = 0, h = static_cast<int64_t>(n)-1;
				uint64_t c = std::numeric_limits<uint64_t>::max();

				while ( h > l )
				{
					uint64_t const m = (h+l)>>1;
					rank(m,&R[0]);
					c = R[sym];

					if ( c >= i+1 )
						h = m;
					else // c < i+1
						l = m+1;
				}

				assert ( l == h );
				rank(l,&R[0]);
				assert ( R[sym] == i+1 );
				assert ( (*this)[l] == sym );

				return l;
			}

			static unique_ptr_type loadFromRunLength(std::string const & rl, uint64_t const numthreads)
			{
				unique_ptr_type tptr(loadFromRunLength(std::vector<std::string>(1,rl),numthreads));
				return UNIQUE_PTR_MOVE(tptr);
			}

			DNARank const & getW() const
			{
				return *this;
			}

			static unique_ptr_type loadFromRunLength(std::vector<std::string> const & rl, uint64_t const numthreads)
			{
				unique_ptr_type P(new this_type);

				uint64_t const n = libmaus2::huffman::RLDecoder::getLength(rl);

				// number of blocks allocated
				uint64_t const numallocblocks = ((n+1) + getDataBasesPerBlock() - 1)/getDataBasesPerBlock();
				// total number of blocks
				uint64_t const numblocks = (n + getDataBasesPerBlock() - 1)/getDataBasesPerBlock();
				// blocks per thread
				uint64_t const blocksperthread = (numblocks + numthreads-1)/numthreads;
				// number of thread packages
				int64_t const numpacks = (numblocks + blocksperthread-1)/blocksperthread;

				P->B = libmaus2::autoarray::AutoArray<uint64_t,libmaus2::autoarray::alloc_type_memalign_cacheline>(numallocblocks * getWordsPerBlock(), false);
				P->n = n;

				libmaus2::autoarray::AutoArray<uint64_t> symacc((numpacks+1)*getSigma(),false);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( int64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const block_low = t*blocksperthread;
					uint64_t const block_high = std::min(block_low + blocksperthread,numblocks);
					uint64_t const block_base_low = block_low * getDataBasesPerBlock();
					uint64_t const block_base_high = std::min(n,block_high * getDataBasesPerBlock());
					uint64_t const block_base_syms = block_base_high - block_base_low;
					assert ( block_high > block_low );
					libmaus2::huffman::RLDecoder rldec(rl,block_base_low);

					uint64_t * p = P->B.begin() + block_low * getWordsPerBlock();

					uint64_t lsymacc[LIBMAUS2_RANK_DNARANK_SIGMA] = {0,0,0,0};

					uint64_t const numblocks  = block_high-block_low;
					uint64_t const fullblocks = block_base_syms / getDataBasesPerBlock();
					uint64_t const restblocks = numblocks-fullblocks;

					for ( uint64_t b = 0; b < fullblocks; ++b )
					{
						*(p++) = lsymacc[0];
						*(p++) = lsymacc[1];
						*(p++) = lsymacc[2];

						for ( uint64_t j = 0; j < getDataWordsPerBlock(); ++j )
						{
							uint64_t w = 0;
							for ( uint64_t k = 0; k < getBasesPerWord(); ++k )
							{
								uint64_t const sym = rldec.decode();
								lsymacc[sym]++;
								w |= (sym<<(k<<1));
							}
							*(p++) = w;
						}
					}
					if ( restblocks )
					{
						*(p++) = lsymacc[0];
						*(p++) = lsymacc[1];
						*(p++) = lsymacc[2];

						uint64_t const numsyms = block_base_syms - fullblocks * getDataBasesPerBlock();
						uint64_t const fullwords = numsyms / getBasesPerWord();

						for ( uint64_t j = 0; j < fullwords; ++j )
						{
							uint64_t w = 0;
							for ( uint64_t k = 0; k < getBasesPerWord(); ++k )
							{
								uint64_t const sym = rldec.decode();
								lsymacc[sym]++;
								w |= (sym<<(k<<1));
							}
							*(p++) = w;
						}
						if ( numsyms - fullwords * getBasesPerWord() )
						{
							uint64_t w = 0;
							for ( uint64_t k = 0; k < (numsyms - fullwords * getBasesPerWord()); ++k )
							{
								uint64_t const sym = rldec.decode();
								lsymacc[sym]++;
								w |= (sym<<(k<<1));
							}
							*(p++) = w;
						}
					}

					for ( unsigned int i = 0; i < getSigma(); ++i )
						symacc[t*getSigma() + i] = lsymacc[i];
				}

				uint64_t a[LIBMAUS2_RANK_DNARANK_SIGMA] = {0,0,0,0};
				for ( int64_t z = 0; z < (numpacks+1); ++z )
				{
					uint64_t t[LIBMAUS2_RANK_DNARANK_SIGMA];
					for ( uint64_t i = 0; i < getSigma(); ++i )
					{
						t[i] = symacc[z*getSigma() + i];
						symacc[z*getSigma() + i] = a[i];
						a[i] += t[i];
					}
				}

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( int64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const block_low = t*blocksperthread;
					uint64_t const block_high = std::min(block_low + blocksperthread,numblocks);
					assert ( block_high > block_low );
					uint64_t * p = P->B.begin() + block_low * getWordsPerBlock();
					uint64_t const numblocks = block_high-block_low;

					for ( uint64_t b = 0; b < numblocks; ++b )
					{
						(*p++) += symacc[t*getSigma()+0];
						(*p++) += symacc[t*getSigma()+1];
						(*p++) += symacc[t*getSigma()+2];
						p += getDataWordsPerBlock();
					}
				}

				// make sure rankm works on argument n
				if ( numallocblocks != numblocks )
				{
					uint64_t * p = P->B.begin() + (numblocks * getWordsPerBlock());
					uint64_t const * acc = symacc.begin() + getSigma()*numpacks;
					for ( uint64_t i = 0; i < 3; ++i )
						*(p++) = *(acc++);
				}

				P->computeD();

				#if defined(LIBMAUS2_RANK_DNARANK_TESTFROMRUNLENGTH)
				P->testFromRunLength(rl);
				#endif

				return UNIQUE_PTR_MOVE(P);
			}

			static unique_ptr_type loadFromSerialised(std::istream & in)
			{
				unique_ptr_type P(new this_type);
				P->deserialise(in);
				P->computeD();
				return UNIQUE_PTR_MOVE(P);
			}

			static unique_ptr_type loadFromSerialised(std::string const & fn)
			{
				libmaus2::aio::InputStreamInstance ISI(fn);
				unique_ptr_type tptr(loadFromSerialised(ISI));
				return UNIQUE_PTR_MOVE(tptr);
			}

			uint64_t getD(unsigned int const sym) const
			{
				return D[sym];
			}

			struct ApproximateSearchQueueElement
			{
				uint64_t low;
				uint64_t high;
				uint64_t err;
				uint64_t qlen;
				uint64_t rlen;
				double p;

				ApproximateSearchQueueElement(
					uint64_t const rlow = 0,
					uint64_t const rhigh = 0,
					uint64_t const rerr = 0,
					uint64_t const rqlen = 0,
					uint64_t const rrlen = 0,
					double const rp = 0
				) : low(rlow), high(rhigh), err(rerr), qlen(rqlen), rlen(rrlen), p(rp)
				{
				}
			};

			// return all matching intervals within an edit distance of at most maxerr for the query it[0],it[1],...,it[m-1]
			// the actual edit may be smaller for some intervals in the output as we do not keep track of the type of
			// error inserted (e.g. we may insert a character and immediately delete it)
			template<typename iterator>
			std::vector < ApproximateSearchQueueElement > approximateSearch(
				iterator it,
				uint64_t const m,
				uint64_t const maxerr,
				double const probcor,
				double const probsubst,
				double const probins,
				double const probdel
			)
			{
				std::deque<ApproximateSearchQueueElement> todo;
				std::vector<ApproximateSearchQueueElement> V;

				uint64_t R0[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t R1[LIBMAUS2_RANK_DNARANK_SIGMA];

				double const probincor = 1.0-probcor;

				// push interval for empty word
				todo.push_back(ApproximateSearchQueueElement(0,size(),0,0,0,1.0));

				while ( todo.size() )
				{
					ApproximateSearchQueueElement const P = todo.front();
					todo.pop_front();

					// push interval if we reached the end of the query
					if ( P.qlen == m )
					{
						V.push_back(P);
					}

					// compute possible extension intervals
					multiLF(P.low,P.high,&R0[0],&R1[0]);

					// if we have not yet reached the end of the query
					if ( P.qlen < m )
					{
						// get extension symbol
						unsigned int const sym = it[m-P.qlen-1];

						// match
						if ( R1[sym]-R0[sym] )
						{
							todo.push_back(ApproximateSearchQueueElement(R0[sym],R1[sym],P.err,P.qlen+1,P.rlen+1,P.p * probcor));
						}

						// more errors available?
						if ( P.err < maxerr )
						{
							// mismatch
							for ( unsigned int i = 0; i < LIBMAUS2_RANK_DNARANK_SIGMA; ++i )
								 if ( R1[i]-R0[i] && i != sym )
								 	todo.push_back(ApproximateSearchQueueElement(R0[i],R1[i],P.err+1,P.qlen+1,P.rlen+1, P.p * probincor * probsubst));

							// insertion (consume query base without moving on reference)
						 	todo.push_back(ApproximateSearchQueueElement(P.low,P.high,P.err+1,P.qlen+1,P.rlen, P.p * probincor * probins));
						}
					}

					// more errors available
					if ( P.err < maxerr )
						// deletion (consume reference base without using query base)
						for ( unsigned int i = 0; i < LIBMAUS2_RANK_DNARANK_SIGMA; ++i )
							 if ( R1[i]-R0[i] )
							 	todo.push_back(ApproximateSearchQueueElement(R0[i],R1[i],P.err+1,P.qlen,P.rlen+1, P.p * probincor * probdel));
				}

				return V;
			}

			// return all matching intervals within an edit distance of at most maxerr for the query it[0],it[1],...,it[m-1]
			// the actual edit may be smaller for some intervals in the output as we do not keep track of the type of
			// error inserted (e.g. we may insert a character and immediately delete it)
			template<typename iterator>
			void approximateSearch(
				iterator it,
				uint64_t const m,
				uint64_t const maxerr,
				double const probcor,
				double const probsubst,
				double const probins,
				double const probdel,
				libmaus2::util::Queue<ApproximateSearchQueueElement> & todo,
				libmaus2::util::Queue<ApproximateSearchQueueElement> & V
			)
			{
				V.clear();

				uint64_t R0[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t R1[LIBMAUS2_RANK_DNARANK_SIGMA];

				double const probincor = 1.0-probcor;

				// push interval for empty word
				todo.push(ApproximateSearchQueueElement(0,size(),0,0,0,1.0));

				while ( ! (todo.empty()) )
				{
					ApproximateSearchQueueElement const P = todo.pop();

					// push interval if we reached the end of the query
					if ( P.qlen == m )
					{
						V.push(P);
					}

					// compute possible extension intervals
					multiLF(P.low,P.high,&R0[0],&R1[0]);

					// if we have not yet reached the end of the query
					if ( P.qlen < m )
					{
						// get extension symbol
						unsigned int const sym = it[m-P.qlen-1];

						// match
						if ( R1[sym]-R0[sym] )
						{
							todo.push(ApproximateSearchQueueElement(R0[sym],R1[sym],P.err,P.qlen+1,P.rlen+1,P.p * probcor));
						}

						// more errors available?
						if ( P.err < maxerr )
						{
							// mismatch
							for ( unsigned int i = 0; i < LIBMAUS2_RANK_DNARANK_SIGMA; ++i )
								 if ( R1[i]-R0[i] && i != sym )
								 	todo.push(ApproximateSearchQueueElement(R0[i],R1[i],P.err+1,P.qlen+1,P.rlen+1, P.p * probincor * probsubst));

							// insertion (consume query base without moving on reference)
						 	todo.push(ApproximateSearchQueueElement(P.low,P.high,P.err+1,P.qlen+1,P.rlen, P.p * probincor * probins));
						}
					}

					// more errors available
					if ( P.err < maxerr )
						// deletion (consume reference base without using query base)
						for ( unsigned int i = 0; i < LIBMAUS2_RANK_DNARANK_SIGMA; ++i )
							 if ( R1[i]-R0[i] )
							 	todo.push(ApproximateSearchQueueElement(R0[i],R1[i],P.err+1,P.qlen,P.rlen+1, P.p * probincor * probdel));
				}
			}

			static int64_t getSymbolThres()
			{
				return LIBMAUS2_RANK_DNARANK_SIGMA;
			}

			D_type const & getD() const
			{
				return D;
			}
		};
	}
}
#endif
