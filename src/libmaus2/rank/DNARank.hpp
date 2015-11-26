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

namespace libmaus2
{
	namespace rank
	{
		struct DNARank
		{
			typedef DNARank this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			static unsigned int const sigma = 4;
			static unsigned int const log_sigma = 2;
			static unsigned int const bits_per_byte = 8;
			static unsigned int const cache_line_size = 64;

			static unsigned int const words_per_block = cache_line_size/sizeof(uint64_t);
			static unsigned int const dict_words_per_block = (sigma-1);
			static unsigned int const data_words_per_block = words_per_block - dict_words_per_block;
			static unsigned int const data_bytes_per_block = data_words_per_block * sizeof(uint64_t);
			static unsigned int const bases_per_word = (bits_per_byte*sizeof(uint64_t))/log_sigma;
			static unsigned int const data_bases_per_block = data_bytes_per_block * (bits_per_byte/log_sigma);

			uint64_t n;
			libmaus2::autoarray::AutoArray<uint64_t,libmaus2::autoarray::alloc_type_memalign_cacheline> B;
			libmaus2::autoarray::AutoArray<uint64_t,libmaus2::autoarray::alloc_type_memalign_cacheline> D;

			void computeD()
			{
				D.resize(sigma+1);
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

			void testFromRunLength(std::vector<std::string> const & rl) const
			{
				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif

				// total number of blocks
				uint64_t const numblocks = (n + data_bases_per_block - 1)/data_bases_per_block;
				// blocks per thread
				uint64_t const blocksperthread = (numblocks + numthreads-1)/numthreads;
				// number of thread packages
				int64_t const numpacks = (numblocks + blocksperthread-1)/blocksperthread;

				uint64_t r_a[sigma];
				uint64_t r_b[sigma];

				rankm(n,&r_a[0]);
				rank(n-1,&r_b[0]);

				for ( uint64_t i = 0; i < sigma; ++i )
					assert ( r_a[i] == r_b[i] );
				assert ( std::accumulate(&r_a[0],&r_a[sigma],0ull) == n );

				std::cerr << "checking...";
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const block_low = t*blocksperthread;
					uint64_t const block_high = std::min(block_low + blocksperthread,numblocks);
					uint64_t const block_base_low = block_low * data_bases_per_block;
					uint64_t const block_base_high = std::min(n,block_high * data_bases_per_block);
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
				#pragma omp parallel for
				#endif
				for ( int64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const block_low = t*blocksperthread;
					uint64_t const block_high = std::min(block_low + blocksperthread,numblocks);
					uint64_t const block_base_low = block_low * data_bases_per_block;
					uint64_t const block_base_high = std::min(n,block_high * data_bases_per_block);
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
				uint64_t acc[sigma] = {0,0,0,0};
				uint64_t racc[sigma];
				for ( uint64_t i = 0; i < n; ++i )
				{
					rankm(i,&racc[0]);
					bool ok = true;
					for ( unsigned int j = 0; j < sigma; ++j )
					{
						ok = ok && ( acc[j] == racc[j] );
					}
					if ( ! ok )
					{
						std::cerr << "failed at " << i << "/" << n << std::endl;

						for ( uint64_t j = 0; j < sigma; ++j )
							std::cerr << acc[j] << "\t" << racc[j] << std::endl;

						assert ( ok );
					}

					int64_t const sym = C[i];


					acc [ sym ] ++;

					rank(i,&racc[0]);
					ok = true;
					for ( unsigned int j = 0; j < sigma; ++j )
					{
						ok = ok && ( acc[j] == racc[j] );
					}
					if ( ! ok )
					{
						std::cerr << "rank failed at " << i << "/" << n << std::endl;

						for ( uint64_t j = 0; j < sigma; ++j )
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
				#pragma omp parallel for
				#endif
				for ( int64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const block_low = t*blocksperthread;
					uint64_t const block_high = std::min(block_low + blocksperthread,numblocks);
					uint64_t const block_base_low = block_low * data_bases_per_block;
					uint64_t const block_base_high = std::min(n,block_high * data_bases_per_block);
					assert ( block_high > block_low );
					uint64_t R[sigma];

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
				uint64_t const blockskip = i / data_bases_per_block;
				i -= blockskip * data_bases_per_block;
				P += (blockskip * words_per_block) + dict_words_per_block;
				uint64_t wordskip = i / bases_per_word;
				i -= wordskip * bases_per_word;
				P += wordskip;
				return ((*P) >> (i<<1)) & (sigma-1);
			}

			uint64_t size() const
			{
				return n;
			}

			void rankm(uint64_t i, uint64_t * r) const
			{
				uint64_t const * P = B.begin();
				uint64_t const blockskip = i / data_bases_per_block;
				i -= blockskip * data_bases_per_block;
				P += (blockskip * words_per_block);

				for ( unsigned int i = 0; i < sigma-1; ++i )
					r[i] = *(P++);

				uint64_t wordskip = i / bases_per_word;

				r[(sigma-1)] = (blockskip * data_bases_per_block) + (wordskip*bases_per_word);

				i -= wordskip * bases_per_word;

				uint64_t w;
				while ( wordskip-- )
				{
					w = *(P++);
					r[0] += symcnt(w,0x0000000000000000ULL);
					r[1] += symcnt(w,0x5555555555555555ULL);
					r[2] += symcnt(w,0xAAAAAAAAAAAAAAAAULL);
				}

				r[sigma-1] -= (r[0]+r[1]+r[2]);

				if ( i )
				{
					w = *(P++);
					uint64_t const omask = (1ull << (log_sigma*i))-1;
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

				for ( unsigned int i = 0; i < sigma; ++i )
				{
					R0[i] += D[i];
					R1[i] += D[i];
				}
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

			void decode(uint64_t rr, unsigned int k, char * C) const
			{
				for ( unsigned int i = 0; i < k; ++i )
				{
					*(C++) = sortedSymbol(rr);
					rr = phi(rr);
				}
			}

			/**
			 * test searching for all k-mers
			 **/
			void testSearch(unsigned int k) const
			{
				uint64_t const lim = 1ull << (log_sigma*k);

				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif

				libmaus2::autoarray::AutoArray<libmaus2::autoarray::AutoArray<char > > AC(numthreads);
				libmaus2::autoarray::AutoArray<libmaus2::autoarray::AutoArray<char > > AD(numthreads);

				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					AC[i].resize(k);
					AD[i].resize(k);
				}

				libmaus2::parallel::PosixSpinLock cerrlock;

				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1)
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
						C[i] = (z >> (log_sigma*i)) & (sigma-1);
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
				uint64_t R0[sigma], R1[sigma];
				multiLF(P.first, P.second, &R0[0], &R1[0]);
				return std::pair<uint64_t,uint64_t>(R0[sym],R1[sym]);
			}

			unsigned int inverseSelect(uint64_t i, uint64_t * r) const
			{
				uint64_t const * P = B.begin();
				uint64_t const blockskip = i / data_bases_per_block;
				i -= blockskip * data_bases_per_block;
				P += (blockskip * words_per_block);

				for ( unsigned int i = 0; i < sigma-1; ++i )
					r[i] = *(P++);

				uint64_t wordskip = i / bases_per_word;

				r[sigma-1] = (blockskip * data_bases_per_block) + (wordskip*bases_per_word);

				i -= wordskip * bases_per_word;

				uint64_t w;
				while ( wordskip-- )
				{
					w = *(P++);
					r[0] += symcnt(w,0x0000000000000000ULL);
					r[1] += symcnt(w,0x5555555555555555ULL);
					r[2] += symcnt(w,0xAAAAAAAAAAAAAAAAULL);
				}

				r[sigma-1] -= (r[0]+r[1]+r[2]);

				w = *(P++);

				if ( i )
				{
					uint64_t const omask = (1ull << (log_sigma*i))-1;
					uint64_t const r0 = symcnt(w,0x0000000000000000ULL,omask);
					uint64_t const r1 = symcnt(w,0x5555555555555555ULL,omask);
					uint64_t const r2 = symcnt(w,0xAAAAAAAAAAAAAAAAULL,omask);
					r[0] += r0;
					r[1] += r1;
					r[2] += r2;
					r[3] += i-(r0+r1+r2);
				}

				return (w >> (log_sigma*i)) & (sigma-1);
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
				uint64_t R[sigma];
				unsigned int const sym = inverseSelect(i,&R[0]);
				return D[sym] + R[sym];
			}

			std::pair<uint64_t, uint64_t> extendedLF(uint64_t const i) const
			{
				uint64_t R[sigma];
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
				uint64_t R[sigma];

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

			static unique_ptr_type loadFromRunLength(std::string const & rl)
			{
				unique_ptr_type tptr(loadFromRunLength(std::vector<std::string>(1,rl)));
				return UNIQUE_PTR_MOVE(tptr);
			}

			static unique_ptr_type loadFromRunLength(std::vector<std::string> const & rl)
			{
				unique_ptr_type P(new this_type);

				uint64_t const n = libmaus2::huffman::RLDecoder::getLength(rl);

				#if defined(_OPENMP)
				uint64_t const numthreads = omp_get_max_threads();
				#else
				uint64_t const numthreads = 1;
				#endif

				// number of blocks allocated
				uint64_t const numallocblocks = ((n+1) + data_bases_per_block - 1)/data_bases_per_block;
				// total number of blocks
				uint64_t const numblocks = (n + data_bases_per_block - 1)/data_bases_per_block;
				// blocks per thread
				uint64_t const blocksperthread = (numblocks + numthreads-1)/numthreads;
				// number of thread packages
				int64_t const numpacks = (numblocks + blocksperthread-1)/blocksperthread;

				P->B = libmaus2::autoarray::AutoArray<uint64_t,libmaus2::autoarray::alloc_type_memalign_cacheline>(numallocblocks * words_per_block, false);
				P->n = n;

				libmaus2::autoarray::AutoArray<uint64_t> symacc((numpacks+1)*sigma,false);

				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const block_low = t*blocksperthread;
					uint64_t const block_high = std::min(block_low + blocksperthread,numblocks);
					uint64_t const block_base_low = block_low * data_bases_per_block;
					uint64_t const block_base_high = std::min(n,block_high * data_bases_per_block);
					uint64_t const block_base_syms = block_base_high - block_base_low;
					assert ( block_high > block_low );
					libmaus2::huffman::RLDecoder rldec(rl,block_base_low);

					uint64_t * p = P->B.begin() + block_low * words_per_block;

					uint64_t lsymacc[sigma] = {0,0,0,0};

					uint64_t const numblocks  = block_high-block_low;
					uint64_t const fullblocks = block_base_syms / data_bases_per_block;
					uint64_t const restblocks = numblocks-fullblocks;

					for ( uint64_t b = 0; b < fullblocks; ++b )
					{
						*(p++) = lsymacc[0];
						*(p++) = lsymacc[1];
						*(p++) = lsymacc[2];

						for ( uint64_t j = 0; j < data_words_per_block; ++j )
						{
							uint64_t w = 0;
							for ( uint64_t k = 0; k < bases_per_word; ++k )
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

						uint64_t const numsyms = block_base_syms - fullblocks * data_bases_per_block;
						uint64_t const fullwords = numsyms / bases_per_word;

						for ( uint64_t j = 0; j < fullwords; ++j )
						{
							uint64_t w = 0;
							for ( uint64_t k = 0; k < bases_per_word; ++k )
							{
								uint64_t const sym = rldec.decode();
								lsymacc[sym]++;
								w |= (sym<<(k<<1));
							}
							*(p++) = w;
						}
						if ( numsyms - fullwords * bases_per_word )
						{
							uint64_t w = 0;
							for ( uint64_t k = 0; k < (numsyms - fullwords * bases_per_word); ++k )
							{
								uint64_t const sym = rldec.decode();
								lsymacc[sym]++;
								w |= (sym<<(k<<1));
							}
							*(p++) = w;
						}
					}

					for ( unsigned int i = 0; i < sigma; ++i )
						symacc[t*sigma + i] = lsymacc[i];
				}

				uint64_t a[sigma] = {0,0,0,0};
				for ( int64_t z = 0; z < (numpacks+1); ++z )
				{
					uint64_t t[sigma];
					for ( uint64_t i = 0; i < sigma; ++i )
					{
						t[i] = symacc[z*sigma + i];
						symacc[z*sigma + i] = a[i];
						a[i] += t[i];
					}
				}

				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t t = 0; t < numpacks; ++t )
				{
					uint64_t const block_low = t*blocksperthread;
					uint64_t const block_high = std::min(block_low + blocksperthread,numblocks);
					assert ( block_high > block_low );
					uint64_t * p = P->B.begin() + block_low * words_per_block;
					uint64_t const numblocks = block_high-block_low;

					for ( uint64_t b = 0; b < numblocks; ++b )
					{
						(*p++) += symacc[t*sigma+0];
						(*p++) += symacc[t*sigma+1];
						(*p++) += symacc[t*sigma+2];
						p += data_words_per_block;
					}
				}

				// make sure rankm works on argument n
				if ( numallocblocks != numblocks )
				{
					uint64_t * p = P->B.begin() + (numblocks * words_per_block);
					uint64_t const * acc = symacc.begin() + sigma*numpacks;
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

		};
	}
}
#endif
