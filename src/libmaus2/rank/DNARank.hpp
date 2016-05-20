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
#include <libmaus2/rank/DNARankBiDirRange.hpp>
#include <sstream>

#include <libmaus2/random/Random.hpp>
#include <libmaus2/parallel/PosixSpinLock.hpp>

#include <libmaus2/util/Queue.hpp>
#include <libmaus2/util/PrefixSums.hpp>
#include <libmaus2/sorting/InPlaceParallelSort.hpp>
#include <libmaus2/bitio/BitVector.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankMEM
		{
			libmaus2::rank::DNARankBiDirRange intv;
			uint64_t left;
			uint64_t right;

			DNARankMEM()
			{

			}

			uint64_t diam() const
			{
				return right-left;
			}

			DNARankMEM(libmaus2::rank::DNARankBiDirRange const & rintv, uint64_t const rleft, uint64_t const rright)
			: intv(rintv), left(rleft), right(rright) {}

			bool operator<(DNARankMEM const & O) const
			{
				if ( intv < O.intv )
					return true;
				else if ( O.intv < intv )
					return false;
				else if ( left != O.left )
					return left < O.left;
				else
					return right < O.right;
			}

			bool operator==(DNARankMEM const & O) const
			{
				if ( *this < O )
					return false;
				else if ( O < *this )
					return false;
				else
					return true;
			}

			bool operator!=(DNARankMEM const & O) const
			{
				return !operator==(O);
			}
		};

		struct DNARankMEMPosComparator
		{
			bool operator()(DNARankMEM const & A, DNARankMEM const & B) const
			{
				if ( A.left != B.left )
					return A.left < B.left;
				else if ( A.right != B.right )
					return A.right < B.right;
				else
					return A.intv < B.intv;
			}
		};

		inline std::ostream & operator<<(std::ostream & out, DNARankMEM const & D)
		{
			return out << "DNARankMEM(intv=" << D.intv << ",left=" << D.left << ",right=" << D.right << ")";
		}

		struct DNARankSMEMContext
		{
			typedef DNARankSMEMContext this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			//typedef std::vector < std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > >::const_iterator prev_iterator;
			typedef std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > const * prev_iterator;
			// typedef std::vector < DNARankMEM > match_container_type;
			typedef libmaus2::autoarray::AutoArray < DNARankMEM > match_container_type;
			// typedef match_container_type::const_iterator match_iterator;
			typedef DNARankMEM const * match_iterator;

			private:
			//std::vector < std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > > Vcur;
			libmaus2::autoarray::AutoArray< std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > > Acur;
			uint64_t ocur;
			//std::vector < std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > > Vprev;
			libmaus2::autoarray::AutoArray< std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > > Aprev;
			uint64_t oprev;
			match_container_type Amatch;
			uint64_t omatch;

			public:
			DNARankSMEMContext() : ocur(0), oprev(0), omatch(0)
			{

			}

			match_container_type const & getMatches() const
			{
				//return Vmatch;
				return Amatch;
			}

			uint64_t getNumMatches() const
			{
				//return Vmatch.size();
				return omatch;
			}

			prev_iterator pbegin() const
			{
				//return Vprev.begin();
				return Aprev.begin();
			}

			prev_iterator pend() const
			{
				//return Vprev.end();
				return Aprev.begin()+oprev;
			}

			match_iterator mbegin() const
			{
				//return Vmatch.begin();
				return Amatch.begin();
			}

			match_iterator mend() const
			{
				//return Vmatch.end();
				return Amatch.begin()+omatch;
			}

			void reset()
			{
				//Vcur.resize(0);
				//Vprev.resize(0);
				ocur = 0;
				oprev = 0;
			}

			void resetCur()
			{
				//Vcur.resize(0);
				ocur = 0;
			}

			void reverseCur()
			{
				//std::reverse(Vcur.begin(),Vcur.end());
				std::reverse(Acur.begin(),Acur.begin()+ocur);
			}

			void resetMatch()
			{
				//Vmatch.resize(0);
				omatch = 0;
			}

			void reverseMatch(uint64_t const start)
			{
				std::reverse(Amatch.begin()+start,Amatch.begin()+omatch);
			}

			void pushCur(std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > const & I)
			{
				//Vcur.push_back(I);
				Acur.push(ocur,I);
			}

			void pushMatch(DNARankMEM const & I)
			{
				//Vmatch.push_back(I);
				Amatch.push(omatch,I);
			}

			void swapCurPrev()
			{
				// Vcur.swap(Vprev);
				Acur.swap(Aprev);
				std::swap(ocur,oprev);
			}

			bool curEmpty() const
			{
				//return Vcur.empty();
				return ocur == 0;
			}

			bool prevEmpty() const
			{
				//return Vprev.empty();
				return oprev == 0;
			}
		};

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
			// typedef libmaus2::autoarray::AutoArray<uint64_t,libmaus2::autoarray::alloc_type_hugepages_memalign_cacheline> B_type;
			typedef libmaus2::autoarray::AutoArray<uint64_t,libmaus2::autoarray::alloc_type_memalign_cacheline> B_type;
			B_type B;
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
					libmaus2::huffman::RLDecoder rldec(rl,block_base_low,1/*numthreads*/);

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
					libmaus2::huffman::RLDecoder rldec(rl,block_base_low,1/*numthreads*/);

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

			// this method assumes the index is based on a sequence ( S S^RC )
			void backwardExtendBi(libmaus2::rank::DNARankBiDirRange I, libmaus2::rank::DNARankBiDirRange O[LIBMAUS2_RANK_DNARANK_SIGMA]) const
			{
				uint64_t R0[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t R1[LIBMAUS2_RANK_DNARANK_SIGMA];

				multiLF(I.forw,I.forw+I.size,&R0[0],&R1[0]);

				uint64_t rs = 0;
				for ( int64_t sym = LIBMAUS2_RANK_DNARANK_SIGMA-1; sym >= 0; --sym )
				{
					R1[sym] -= R0[sym];
					O [sym] . forw = R0[sym];
					O [sym] . size = R1[sym];
					O [sym] . reco = I.reco + rs;
					rs += R1 [sym];
				}
			}

			// this method assumes the index is based on a sequence ( S S^RC )
			libmaus2::rank::DNARankBiDirRange backwardExtendBi(libmaus2::rank::DNARankBiDirRange const & I, uint64_t const sym) const
			{
				std::pair<uint64_t,uint64_t> Psym;
				singleSymbolLF(I.forw,I.forw+I.size,sym,Psym);

				libmaus2::rank::DNARankBiDirRange R;
				R.forw = Psym.first;
				R.size = Psym.second-Psym.first;
				R.reco = I.reco;

				switch ( sym )
				{
					case 0:
						// R.reco += (R1[1]-R0[1]);
						R.reco += singleSymbolLFRange(I.forw,I.forw+I.size,1);
					case 1:
						// R.reco += (R1[2]-R0[2]);
						R.reco += singleSymbolLFRange(I.forw,I.forw+I.size,2);
					case 2:
						// R.reco += (R1[3]-R0[3]);
						R.reco += singleSymbolLFRange(I.forw,I.forw+I.size,3);
					default:
						break;
				}

				#if 0
				libmaus2::rank::DNARankBiDirRange OO[LIBMAUS2_RANK_DNARANK_SIGMA];
				backwardExtendBi(I,&OO[0]);
				assert ( R == OO[sym] );
				#endif

				return R;
			}

			// this method assumes the index is based on a sequence ( S S^RC )
			void forwardExtendBi(libmaus2::rank::DNARankBiDirRange I, libmaus2::rank::DNARankBiDirRange O[LIBMAUS2_RANK_DNARANK_SIGMA]) const
			{
				uint64_t R0[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t R1[LIBMAUS2_RANK_DNARANK_SIGMA];

				multiLF(I.reco,I.reco+I.size,&R0[0],&R1[0]);

				uint64_t rs = 0;
				for ( uint64_t sym = 0; sym < LIBMAUS2_RANK_DNARANK_SIGMA; ++sym )
				{
					uint64_t const rsym = sym ^ 3;
					libmaus2::rank::DNARankBiDirRange & R = O[sym];

					R.reco = R0[rsym];
					R.size = R1[rsym]-R0[rsym];
					R.forw = I.forw + rs;

					rs += R1[rsym]-R0[rsym];
				}
			}

			// this method assumes the index is based on a sequence ( S S^RC )
			libmaus2::rank::DNARankBiDirRange forwardExtendBi(libmaus2::rank::DNARankBiDirRange I, uint64_t const sym) const
			{
				libmaus2::rank::DNARankBiDirRange R;
				uint64_t const rsym = sym^3;

				std::pair<uint64_t,uint64_t> Psym;
				singleSymbolLF(I.reco,I.reco+I.size,rsym,Psym);

				R.reco = Psym.first; // R0[rsym];
				R.size = Psym.second-Psym.first; // R1[rsym]-R0[rsym];
				R.forw = I.forw;

				switch ( sym )
				{
					case 3:
						R.forw += singleSymbolLFRange(I.reco,I.reco+I.size,1);
					case 2:
						R.forw += singleSymbolLFRange(I.reco,I.reco+I.size,2);
					case 1:
						R.forw += singleSymbolLFRange(I.reco,I.reco+I.size,3);
					default:
						break;
				}

				#if 0
				libmaus2::rank::DNARankBiDirRange OO[LIBMAUS2_RANK_DNARANK_SIGMA];
				forwardExtendBi(I,&OO[0]);
				assert ( R == OO[sym] );
				#endif

				return R;
			}

			libmaus2::rank::DNARankBiDirRange epsilonBi() const
			{
				libmaus2::rank::DNARankBiDirRange R;
				R.forw = R.reco = 0;
				R.size = n;
				return R;
			}

			template<typename iterator>
			libmaus2::rank::DNARankBiDirRange backwardSearchBi(iterator it, uint64_t const n) const
			{
				libmaus2::rank::DNARankBiDirRange R = epsilonBi();
				for ( uint64_t i = 0; i < n; ++i )
					R = backwardExtendBi(R,it[n-i-1]);
				return R;
			}

			template<typename iterator>
			libmaus2::rank::DNARankBiDirRange forwardSearchBi(iterator it, uint64_t const n) const
			{
				libmaus2::rank::DNARankBiDirRange R = epsilonBi();
				for ( uint64_t i = 0; i < n; ++i )
					R = forwardExtendBi(R,it[i]);
				return R;
			}


			struct DNARankMEMContComp
			{
				bool operator()(DNARankMEM const & A, DNARankMEM const & B) const
				{
					if ( A.left != B.left )
						return A.left < B.left;
					// same left
					else
						return A.right > B.right;
				}
			};

			template<typename iterator>
			void mem(
				iterator it,
				uint64_t const left,
				uint64_t const right,
				uint64_t const i0,
				uint64_t const minfreq,
				uint64_t const minlen,
				std::vector<DNARankMEM> & Vmem
				) const
			{
				assert ( i0 >= left );
				assert ( i0 < right );

				// initialise with centre symbol at position i0
				libmaus2::rank::DNARankBiDirRange I = backwardExtendBi(epsilonBi(),it[i0]);

				std::vector < std::pair<libmaus2::rank::DNARankBiDirRange,uint64_t> > Vcur;

				assert ( minfreq );
				uint64_t ir;
				// extend to the right as far as possible
				for ( ir = i0+1; I.size >= minfreq && ir < right; ++ir )
				{
					// extend by one symbol on the right
					libmaus2::rank::DNARankBiDirRange IP = forwardExtendBi(I,it[ir]);

					// push previous interval if new one has a smaller range
					if ( IP.size != I.size )
					{
						assert ( IP.size < I.size );
						assert ( I.size >= minfreq );
						Vcur.push_back(std::pair<libmaus2::rank::DNARankBiDirRange,uint64_t>(I,ir));
					}

					I = IP;
				}
				// push final interval if it is not empty
				if ( I.size >= minfreq )
					Vcur.push_back(std::pair<libmaus2::rank::DNARankBiDirRange,uint64_t>(I,ir));

				int64_t ip;
				for ( ip = static_cast<int64_t>(i0)-1; Vcur.size() && ip >= static_cast<int64_t>(left); --ip )
				{
					std::vector < std::pair<libmaus2::rank::DNARankBiDirRange,uint64_t> > Vnext;

					for ( uint64_t j = 0; j < Vcur.size(); ++j )
					{
						std::pair<libmaus2::rank::DNARankBiDirRange,uint64_t> const & P = Vcur[j];
						assert ( P.first.size >= minfreq );

						libmaus2::rank::DNARankBiDirRange IP = backwardExtendBi(P.first,it[ip]);

						if ( IP.size != P.first.size )
						{
							if ( static_cast<int64_t>(P.second)-(ip+1) >= minlen )
								Vmem.push_back(DNARankMEM(P.first,ip+1,P.second));
						}
						if ( IP.size >= minfreq )
							Vnext.push_back(std::pair<libmaus2::rank::DNARankBiDirRange,uint64_t>(IP,P.second));
					}

					Vcur.swap(Vnext);
				}

				for ( uint64_t i = 0; i < Vcur.size(); ++i )
				{
					std::pair<libmaus2::rank::DNARankBiDirRange,uint64_t> const & P = Vcur[i];
					if ( P.first.size >= minfreq )
					{
						if ( static_cast<int64_t>(P.second)-(ip+1) >= minlen )
							Vmem.push_back(DNARankMEM(P.first,ip+1,P.second));
					}
				}
			}

			template<typename iterator>
			void mems(
				iterator it,
				uint64_t const left,
				uint64_t const right,
				uint64_t const i0,
				uint64_t const minfreq,
				uint64_t const minlen,
				std::vector<DNARankMEM> & Vmem
				) const
			{
				Vmem.resize(0);
				mem(it,left,right,i0,minfreq,minlen,Vmem);
				std::sort(Vmem.begin(),Vmem.end(),DNARankMEMContComp());

				// remove contained mems
				std::pair<int64_t,int64_t> maxp(-1,-1);
				uint64_t o = 0;
				for ( uint64_t i = 0; i < Vmem.size(); ++i )
					if ( static_cast<int64_t>(Vmem[i].right) <= maxp.second )
						Vmem[i].left = Vmem[i].right = 0;
					else
					{
						maxp.first = Vmem[i].left;
						maxp.second = Vmem[i].right;
						Vmem[o++] = Vmem[i];
					}
				Vmem.resize(o);
			}

			template<typename iterator>
			uint64_t smem(
				iterator it,
				uint64_t const left,
				uint64_t const right,
				uint64_t const i0,
				uint64_t const minfreq,
				uint64_t const minlen,
				DNARankSMEMContext & context
			) const
			{
				assert ( i0 >= left );
				assert ( i0 < right );

				uint64_t const startmatch = context.getNumMatches();

				// initialise with centre symbol at position i0
				libmaus2::rank::DNARankBiDirRange I = backwardExtendBi(epsilonBi(),it[i0]);

				context.reset();

				uint64_t ir;
				// extend to the right as far as possible
				for ( ir = i0+1; I.size >= minfreq && ir < right; ++ir )
				{
					// extend by one symbol on the right
					libmaus2::rank::DNARankBiDirRange IP = forwardExtendBi(I,it[ir]);

					// push previous interval if new one has a smaller range
					if ( IP.size != I.size )
					{
						assert ( IP.size < I.size );
						assert ( I.size >= minfreq );
						context.pushCur(std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t >(I,ir));
					}

					I = IP;
				}
				// push final interval if it is not empty
				if ( I.size >= minfreq )
					context.pushCur(std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t >(I,ir));

				// reverse interval order to have a longest to shortest order
				context.reverseCur();
				// swap Vcur and Vprev
				context.swapCurPrev();

				// go left as long as the prev vector is not empty
				int64_t ip;
				for (
					ip = static_cast<int64_t>(i0)-1
					;
						(!(context.prevEmpty()))
						&&
						ip >= static_cast<int64_t>(left)
					;
					--ip
				)
				{
					// reset current vector
					context.resetCur();
					uint64_t sprime = 0;
					bool matchadded = false;

					for ( DNARankSMEMContext::prev_iterator pit = context.pbegin(); pit != context.pend(); ++pit )
					{
						std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > const & Iprev = *pit;
						// libmaus2::rank::DNARankBiDirRange const & Iprev = *pit;
						assert ( Iprev.first.size >= minfreq );
						libmaus2::rank::DNARankBiDirRange const Inext = backwardExtendBi(Iprev.first,it[ip]);

						// if there is no extension on the left
						if ( Inext.size < minfreq )
						{
							// we failed to extend an interval, there should be no longer interval we were able to extend
							assert ( context.curEmpty() );

							// if there is no match yet
							if ( ! matchadded )
							{
								matchadded = true;
								if ( static_cast<int64_t>(Iprev.second) - (ip+1) >= static_cast<int64_t>(minlen) )
									context.pushMatch(DNARankMEM(Iprev.first,ip+1,Iprev.second));
							}
						}
						// otherwise if extension on the left is not empty
						else
						{
							// if size is different from previous one
							if ( Inext.size != sprime )
							{
								// sanity check, we are moving from longer to shorter matches, so the amount of matches should rise
								assert ( Inext.size > sprime );
								// if freq sufficient
								if ( Inext.size >= minfreq )
								{
									// push extension
									context.pushCur(std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t >(Inext,Iprev.second));
								}
								// update sprime
								sprime = Inext.size;
							}
						}
					}
					// set up for next round
					context.swapCurPrev();
				}
				// if there is anything left
				if ( !(context.prevEmpty()) )
				{
					std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > const & Iprev = *(context.pbegin());
					// put the longest match on the match list
					if ( static_cast<int64_t>(Iprev.second) - (ip+1) >= static_cast<int64_t>(minlen) )
						context.pushMatch(DNARankMEM(Iprev.first,ip+1,Iprev.second));
				}

				context.reverseMatch(startmatch);

				return context.getNumMatches() - startmatch;
			}

			template<typename iterator>
			void smemSerial(
				iterator it,
				uint64_t const left,
				uint64_t const right,
				// uint64_t const i0,
				uint64_t const minfreq,
				uint64_t const minlen,
				DNARankSMEMContext & context
			) const
			{
				context.resetMatch();
				for ( uint64_t i0 = left; i0 < right; ++i0 )
					smem(it,left,right,i0,minfreq,minlen,context);
			}

			template<typename iterator>
			void smemParallel(
				iterator it,
				uint64_t const left,
				uint64_t const right,
				uint64_t const minfreq,
				uint64_t const minlen,
				std::vector<DNARankMEM> & SMEM,
				uint64_t const numthreads
			) const
			{
				uint64_t const packsperthread = 16;
				uint64_t const tpacks = packsperthread * numthreads;

				uint64_t const posperthread = std::min(static_cast<uint64_t>(64*1024),(right-left + tpacks-1)/tpacks);
				uint64_t const numpack = (right-left) ? ((right-left+posperthread-1)/posperthread) : 0;

				libmaus2::autoarray::AutoArray<DNARankSMEMContext::unique_ptr_type> Acontext(numthreads);
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t t = 0; t < numthreads; ++t )
				{
					DNARankSMEMContext::unique_ptr_type Pcontext(new DNARankSMEMContext);
					Acontext[t] = UNIQUE_PTR_MOVE(Pcontext);
				}

				uint64_t volatile gdone = 0;
				libmaus2::parallel::PosixSpinLock gdonelock;

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
				#endif
				for ( uint64_t t = 0; t < numpack; ++t )
				{
					uint64_t const tlow = left + t*posperthread;
					uint64_t const thigh = std::min(tlow+posperthread,right);

					#if defined(_OPENMP)
					uint64_t const tid = omp_get_thread_num();
					#else
					uint64_t const tid = 0;
					#endif

					DNARankSMEMContext & context = *(Acontext[tid]);

					uint64_t const bs = 64*1024ull;
					uint64_t const numblocks = (thigh-tlow+bs-1)/bs;

					for ( uint64_t b = 0; b < numblocks; ++b )
					{
						uint64_t const blow = tlow + b * bs;
						uint64_t const bhigh = std::min(blow+bs,thigh);

						for ( uint64_t i0 = blow; i0 < bhigh; ++i0 )
							smem(it,left,right,i0,minfreq,minlen,context);

						gdonelock.lock();
						gdone += (bhigh-blow);

						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << gdone << "/" << (right-left) << "\t" << static_cast<double>(gdone) / (right-left) << std::endl;
						}

						gdonelock.unlock();
					}
				}

				std::vector<uint64_t> Vnummatches(Acontext.size());

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < Acontext.size(); ++i )
					Vnummatches[i] = Acontext[i]->getNumMatches();

				uint64_t const sum = libmaus2::util::PrefixSums::prefixSums(Vnummatches.begin(),Vnummatches.end());

				SMEM.resize(sum);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < Acontext.size(); ++i )
				{
					std::copy(
						Acontext[i]->mbegin(),
						Acontext[i]->mend(),
						SMEM.begin() + Vnummatches[i]
					);
					Acontext[i].reset();
				}

				libmaus2::sorting::InPlaceParallelSort::inplacesort2(SMEM.begin(),SMEM.end(),numthreads,DNARankMEMPosComparator());

				#if 0
				libmaus2::bitio::BitVector BV(SMEM.size());
				if ( SMEM.size() )
					BV.setSync(0);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 1; i < SMEM.size(); ++i )
					if ( SMEM[i-1] != SMEM[i] )
						BV.setSync(i);
				#endif

				uint64_t o = SMEM.size() ? 1 : 0;
				for ( uint64_t i = 1; i < SMEM.size(); ++i )
					if ( SMEM[i] != SMEM[i-1] )
						SMEM[o++] = SMEM[i];
				SMEM.resize(o);
			}

			template<typename iterator>
			void smemLimitedParallel(
				iterator it,
				uint64_t const left,
				uint64_t const right,
				uint64_t const minfreq,
				uint64_t const minlen,
				uint64_t const limit,
				std::vector<DNARankMEM> & SMEM,
				uint64_t const numthreads
			) const
			{
				uint64_t const packsperthread = 16;
				uint64_t const tpacks = packsperthread * numthreads;

				uint64_t const posperthread = std::min(static_cast<uint64_t>(64*1024), (right-left + tpacks-1)/tpacks);
				uint64_t const numpack = (right-left) ? ((right-left+posperthread-1)/posperthread) : 0;

				libmaus2::autoarray::AutoArray<DNARankSMEMContext::unique_ptr_type> Acontext(numthreads);
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t t = 0; t < numthreads; ++t )
				{
					DNARankSMEMContext::unique_ptr_type Pcontext(new DNARankSMEMContext);
					Acontext[t] = UNIQUE_PTR_MOVE(Pcontext);
				}

				uint64_t volatile gdone = 0;
				uint64_t volatile lp = std::numeric_limits<uint64_t>::max();
				libmaus2::parallel::PosixSpinLock gdonelock;

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
				#endif
				for ( uint64_t t = 0; t < numpack; ++t )
				{
					uint64_t const tlow = left + t*posperthread;
					uint64_t const thigh = std::min(tlow+posperthread,right);

					#if defined(_OPENMP)
					uint64_t const tid = omp_get_thread_num();
					#else
					uint64_t const tid = 0;
					#endif

					DNARankSMEMContext & context = *(Acontext[tid]);

					uint64_t const bs = 64*1024ull;
					uint64_t const numblocks = (thigh-tlow+bs-1)/bs;

					for ( uint64_t b = 0; b < numblocks; ++b )
					{
						uint64_t const blow = tlow + b * bs;
						uint64_t const bhigh = std::min(blow+bs,thigh);

						if ( blow >= left+limit && bhigh+limit <= right )
							for ( uint64_t i0 = blow; i0 < bhigh; ++i0 )
								smem(it,i0-limit,i0+limit,i0,minfreq,minlen,context);
						else if ( blow >= left+limit )
							for ( uint64_t i0 = blow; i0 < bhigh; ++i0 )
								smem(it,i0-limit,right,i0,minfreq,minlen,context);
						else if ( bhigh+limit <= right )
							for ( uint64_t i0 = blow; i0 < bhigh; ++i0 )
								smem(it,left,i0+limit,i0,minfreq,minlen,context);
						else
							for ( uint64_t i0 = blow; i0 < bhigh; ++i0 )
								smem(it,left,right,i0,minfreq,minlen,context);

						gdonelock.lock();
						gdone += (bhigh-blow);

						if ( gdone >> 20 != lp >> 20 )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << gdone << "/" << (right-left) << "\t" << static_cast<double>(gdone) / (right-left) << std::endl;
							lp = gdone;
						}

						gdonelock.unlock();
					}
				}

				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					std::cerr << gdone << "/" << (right-left) << "\t" << static_cast<double>(gdone) / (right-left) << std::endl;
				}

				std::vector<uint64_t> Vnummatches(Acontext.size());

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < Acontext.size(); ++i )
					Vnummatches[i] = Acontext[i]->getNumMatches();

				uint64_t const sum = libmaus2::util::PrefixSums::prefixSums(Vnummatches.begin(),Vnummatches.end());

				SMEM.resize(sum);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < Acontext.size(); ++i )
				{
					std::copy(
						Acontext[i]->mbegin(),
						Acontext[i]->mend(),
						SMEM.begin() + Vnummatches[i]
					);
					Acontext[i].reset();
				}

				libmaus2::sorting::InPlaceParallelSort::inplacesort2(SMEM.begin(),SMEM.end(),numthreads,DNARankMEMPosComparator());

				#if 0
				libmaus2::bitio::BitVector BV(SMEM.size());
				if ( SMEM.size() )
					BV.setSync(0);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 1; i < SMEM.size(); ++i )
					if ( SMEM[i-1] != SMEM[i] )
						BV.setSync(i);
				#endif

				uint64_t o = SMEM.size() ? 1 : 0;
				for ( uint64_t i = 1; i < SMEM.size(); ++i )
					if ( SMEM[i] != SMEM[i-1] )
						SMEM[o++] = SMEM[i];
				SMEM.resize(o);
			}

			template<typename iterator>
			void smemLimitedParallelSplit(
				iterator it,
				uint64_t const left,
				uint64_t const right,
				uint64_t const minlen,
				uint64_t const limit,
				uint64_t const minsplitlength,
				uint64_t const minsplitsize,
				std::vector<DNARankMEM> const & SMEMin,
				std::vector<DNARankMEM> & SMEMout,
				uint64_t const numthreads
			) const
			{
				uint64_t const packsperthread = 16;
				uint64_t const tpacks = packsperthread * numthreads;

				uint64_t const posperthread = std::min(static_cast<uint64_t>(64*1024), (SMEMin.size()+tpacks-1)/tpacks);
				uint64_t const numpack = (SMEMin.size()) ? ((SMEMin.size()+posperthread-1)/posperthread) : 0;

				libmaus2::autoarray::AutoArray<DNARankSMEMContext::unique_ptr_type> Acontext(numthreads);
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t t = 0; t < numthreads; ++t )
				{
					DNARankSMEMContext::unique_ptr_type Pcontext(new DNARankSMEMContext);
					Acontext[t] = UNIQUE_PTR_MOVE(Pcontext);
				}

				uint64_t volatile gdone = 0;
				uint64_t volatile lp = std::numeric_limits<uint64_t>::max();
				libmaus2::parallel::PosixSpinLock gdonelock;

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
				#endif
				for ( uint64_t t = 0; t < numpack; ++t )
				{
					uint64_t const tlow = t*posperthread;
					uint64_t const thigh = std::min(tlow+posperthread,SMEMin.size());

					#if defined(_OPENMP)
					uint64_t const tid = omp_get_thread_num();
					#else
					uint64_t const tid = 0;
					#endif

					DNARankSMEMContext & context = *(Acontext[tid]);

					uint64_t const bs = 64*1024ull;
					uint64_t const numblocks = (thigh-tlow+bs-1)/bs;

					for ( uint64_t b = 0; b < numblocks; ++b )
					{
						uint64_t const blow = tlow + b * bs;
						uint64_t const bhigh = std::min(blow+bs,thigh);

						for ( uint64_t i = blow; i < bhigh; ++i )
						{
							DNARankMEM const & in = SMEMin[i];
							uint64_t const sleft = in.left;
							uint64_t const sright = in.right;
							uint64_t const sdiam = in.right-in.left;

							if ( sdiam >= minsplitlength && in.intv.size <= minsplitsize )
							{
								uint64_t const scentre = (sleft+sright)>>1;

								uint64_t const bleft =
									std::max(
										static_cast<int64_t>(left),
										static_cast<int64_t>(scentre)-static_cast<int64_t>(limit)
									);
								uint64_t const bright =
									std::min(
										right,
										scentre+limit
									);

								smem(it,bleft,bright,(sleft+sright)>>1,in.intv.size+1,minlen,context);
							}
						}

						gdonelock.lock();
						gdone += (bhigh-blow);

						if ( gdone >> 20 != lp >> 20 )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
							std::cerr << gdone << "/" << (SMEMin.size()) << "\t" << static_cast<double>(gdone) / (SMEMin.size()) << std::endl;
							lp = gdone;
						}

						gdonelock.unlock();
					}
				}

				{
					libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
					std::cerr << gdone << "/" << (SMEMin.size()) << "\t" << static_cast<double>(gdone) / (SMEMin.size()) << std::endl;
				}

				std::vector<uint64_t> Vnummatches(Acontext.size());

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < Acontext.size(); ++i )
					Vnummatches[i] = Acontext[i]->getNumMatches();

				uint64_t const sum = libmaus2::util::PrefixSums::prefixSums(Vnummatches.begin(),Vnummatches.end());

				SMEMout.resize(sum);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t i = 0; i < Acontext.size(); ++i )
				{
					std::copy(
						Acontext[i]->mbegin(),
						Acontext[i]->mend(),
						SMEMout.begin() + Vnummatches[i]
					);
					Acontext[i].reset();
				}

				libmaus2::sorting::InPlaceParallelSort::inplacesort2(SMEMout.begin(),SMEMout.end(),numthreads,DNARankMEMPosComparator());

				uint64_t o = SMEMout.size() ? 1 : 0;
				for ( uint64_t i = 1; i < SMEMout.size(); ++i )
					if ( SMEMout[i] != SMEMout[i-1] )
						SMEMout[o++] = SMEMout[i];
				SMEMout.resize(o);
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

			void singleSymbolLF(uint64_t const l, uint64_t const r, unsigned int const sym, std::pair<uint64_t,uint64_t> & P) const
			{
				P.first = D[sym] + rankm(sym,l);
				P.second = D[sym] + rankm(sym,r);
			}

			uint64_t singleSymbolLFRange(uint64_t const l, uint64_t const r, unsigned int const sym) const
			{
				return
					(D[sym] + rankm(sym,r))
					-
					(D[sym] + rankm(sym,l));
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
				libmaus2::autoarray::AutoArray<libmaus2::autoarray::AutoArray<char > > AR(numthreads);

				for ( uint64_t i = 0; i < numthreads; ++i )
				{
					AC[i].resize(k);
					AD[i].resize(k);
					AR[i].resize(k);
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
					char * const R = AR[tid].begin();

					for ( unsigned int i = 0; i < k; ++i )
					{
						C[i] = (z >> (getLogSigma()*i)) & (getSigma()-1);
						R[k-i-1] = C[i]^3;
					}

					std::pair<uint64_t,uint64_t> const PF = backwardSearch(C,k);
					std::pair<uint64_t,uint64_t> const PR = backwardSearch(R,k);

					libmaus2::rank::DNARankBiDirRange const P = backwardSearchBi(C,k);
					libmaus2::rank::DNARankBiDirRange const FP = forwardSearchBi(C,k);

					uint64_t const minfreq = 2;
					uint64_t const minlen = 5;

					std::vector<DNARankMEM> Vmems;
					mems(C,0,k,k/2,minfreq /* minfreq */,minlen,Vmems);

					#if 0
					std::vector<DNARankMEM> Vmem;
					mem(C,0,k,k/2,Vmem);
					#endif

					DNARankSMEMContext smemcontext;
					smem(C,0,k,k/2,minfreq/* min freq */,minlen,smemcontext);

					std::vector < DNARankMEM > V_smem;
					for ( DNARankSMEMContext::match_iterator ita = smemcontext.mbegin(); ita != smemcontext.mend(); ++ita )
						V_smem.push_back(*ita);

					cerrlock.lock();
					std::cerr << "checking Bidir ";
					for ( uint64_t i = 0; i < k; ++i )
						std::cerr << static_cast<int>(C[i]);
					std::cerr << " ";
					for ( uint64_t i = 0; i < k; ++i )
						std::cerr << static_cast<int>(R[i]);
					std::cerr << " [" << P.forw << "," << P.forw+P.size << ")" << " [" << P.reco << "," << P.reco + P.size << ")";
					#if 0
						<< " expect " << PR.first << "," << PR.second;
					for ( uint64_t i = 0; i < LIBMAUS2_RANK_DNARANK_SIGMA; ++i )
						std::cerr << " " << backwardExtend(PF,i).second-backwardExtend(PF,i).first;
					#endif
					std::cerr << std::endl;
					cerrlock.unlock();

					bool const s_ok = (V_smem == Vmems);

					if ( ! s_ok )
					{
						cerrlock.lock();

						for ( uint64_t i = 0; i < Vmems.size(); ++i )
							std::cerr << "mems " << Vmems[i] << std::endl;
						for ( uint64_t i = 0; i < V_smem.size(); ++i )
							std::cerr << "mems " << V_smem[i] << std::endl;

						cerrlock.unlock();

						assert ( s_ok );
					}

					assert ( PF.first == P.forw );
					assert ( PF.second == P.forw + P.size );
					assert ( PR.first == P.reco );
					assert ( PR.second == P.reco + P.size );
					assert ( FP == P );

					for ( uint64_t r = P.forw; r < P.forw+P.size; ++r )
					{
						decode(r,k,D);
						assert ( memcmp(C,D,k) == 0 );
					}
					if ( P.forw )
					{
						decode(P.forw-1,k,D);
						assert ( memcmp(C,D,k) != 0 );
					}
					if ( P.forw+P.size < n )
					{
						decode(P.forw+P.size,k,D);
						assert ( memcmp(C,D,k) != 0 );
					}

					for ( uint64_t r = P.reco; r < P.reco+P.size; ++r )
					{
						decode(r,k,D);
						assert ( memcmp(R,D,k) == 0 );
					}
					if ( P.reco )
					{
						decode(P.reco-1,k,D);
						assert ( memcmp(R,D,k) != 0 );
					}
					if ( P.reco+P.size < n )
					{
						decode(P.reco+P.size,k,D);
						assert ( memcmp(R,D,k) != 0 );
					}

				}

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

				uint64_t const n = libmaus2::huffman::RLDecoder::getLength(rl,numthreads);

				// number of blocks allocated
				uint64_t const numallocblocks = ((n+1) + getDataBasesPerBlock() - 1)/getDataBasesPerBlock();
				// total number of blocks
				uint64_t const numblocks = (n + getDataBasesPerBlock() - 1)/getDataBasesPerBlock();
				// blocks per thread
				uint64_t const blocksperthread = (numblocks + numthreads-1)/numthreads;
				// number of thread packages
				int64_t const numpacks = (numblocks + blocksperthread-1)/blocksperthread;

				P->B = B_type(numallocblocks * getWordsPerBlock(), false);
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
					libmaus2::huffman::RLDecoder rldec(rl,block_base_low,1/*numthreads*/);

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
