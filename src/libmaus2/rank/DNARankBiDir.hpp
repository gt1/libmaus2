/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_RANK_DNARANKBIDIR_HPP)
#define LIBMAUS2_RANK_DNARANKBIDIR_HPP

#include <libmaus2/rank/DNARank.hpp>
#include <libmaus2/rank/DNARankBiDirRange.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankBiDir
		{
			typedef DNARankBiDir this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			DNARank::unique_ptr_type Pforw;
			DNARank * forw;
			DNARank::unique_ptr_type Preco;
			DNARank * reco;

			private:
			DNARankBiDir() {}

			public:
			static unique_ptr_type loadFromSerialised(std::istream & in)
			{
				unique_ptr_type tptr(new this_type);

				DNARank::unique_ptr_type Pforwtmp(DNARank::loadFromSerialised(in));
				tptr->Pforw = UNIQUE_PTR_MOVE(Pforwtmp);
				tptr->forw = tptr->Pforw.get();

				DNARank::unique_ptr_type Precotmp(DNARank::loadFromSerialised(in));
				tptr->Preco = UNIQUE_PTR_MOVE(Precotmp);
				tptr->reco = tptr->Preco.get();

				return UNIQUE_PTR_MOVE(tptr);
			}

			static unique_ptr_type loadFromSerialised(std::istream & in0, std::istream & in1)
			{
				unique_ptr_type tptr(new this_type);

				DNARank::unique_ptr_type Pforwtmp(DNARank::loadFromSerialised(in0));
				tptr->Pforw = UNIQUE_PTR_MOVE(Pforwtmp);
				tptr->forw = tptr->Pforw.get();

				DNARank::unique_ptr_type Precotmp(DNARank::loadFromSerialised(in1));
				tptr->Preco = UNIQUE_PTR_MOVE(Precotmp);
				tptr->reco = tptr->Preco.get();

				return UNIQUE_PTR_MOVE(tptr);
			}

			static unique_ptr_type loadFromSerialised(std::string const & fn)
			{
				libmaus2::aio::InputStreamInstance ISI(fn);
				unique_ptr_type tptr(loadFromSerialised(ISI));
				return UNIQUE_PTR_MOVE(tptr);
			}

			static unique_ptr_type loadFromSerialised(std::string const & fn0, std::string const & fn1)
			{
				libmaus2::aio::InputStreamInstance ISI0(fn0);
				libmaus2::aio::InputStreamInstance ISI1(fn1);
				unique_ptr_type tptr(loadFromSerialised(ISI0,ISI1));
				return UNIQUE_PTR_MOVE(tptr);
			}

			void serialise(std::ostream & out) const
			{
				Pforw->serialise(out);
				Preco->serialise(out);
			}

			void serialise(std::ostream & out0, std::ostream & out1) const
			{
				Pforw->serialise(out0);
				Preco->serialise(out1);
			}

			void serialise(std::string const & fn) const
			{
				libmaus2::aio::OutputStreamInstance OSI(fn);
				serialise(OSI);
			}

			void serialise(std::string const & fn0, std::string const & fn1) const
			{
				libmaus2::aio::OutputStreamInstance OSI0(fn0);
				libmaus2::aio::OutputStreamInstance OSI1(fn1);
				serialise(OSI0,OSI1);
			}

			static unique_ptr_type loadFromRunLength(
				std::string const & rl0,
				std::string const & rl1,
				uint64_t const numthreads)
			{
				unique_ptr_type tptr(new this_type);

				DNARank::unique_ptr_type Pforwtmp(DNARank::loadFromRunLength(rl0,numthreads));
				tptr->Pforw = UNIQUE_PTR_MOVE(Pforwtmp);
				tptr->forw = tptr->Pforw.get();

				DNARank::unique_ptr_type Precotmp(DNARank::loadFromRunLength(rl1,numthreads));
				tptr->Preco = UNIQUE_PTR_MOVE(Precotmp);
				tptr->reco = tptr->Preco.get();

				return UNIQUE_PTR_MOVE(tptr);
			}

			DNARankBiDirRange epsilon() const
			{
				DNARankBiDirRange range;
				range.size = forw->size();
				range.forw = 0;
				range.reco = 0;
				return range;
			}

			/**
			 * extend interval on the left by sym
			 **/
			DNARankBiDirRange extendLeft(DNARankBiDirRange const & B, unsigned int const sym) const
			{
				uint64_t R0[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t R1[LIBMAUS2_RANK_DNARANK_SIGMA];
				forw->multiRank(B.forw, B.forw + B.size, &R0[0], &R1[0]);

				DNARankBiDirRange R;
				R.forw = R0[sym] + forw->getD(sym);
				R.size = R1[sym]-R0[sym];
				R.reco = B.reco;

				unsigned int const rsym = sym^3;
				for ( unsigned int i = 0; i < rsym; ++i )
					R.reco += (R1[i]-R0[i]);

				return R;
			}

			/**
			 * extend interval on the right by sym
			 **/
			DNARankBiDirRange extendRight(DNARankBiDirRange const & B, unsigned int const sym) const
			{
				uint64_t R0[LIBMAUS2_RANK_DNARANK_SIGMA];
				uint64_t R1[LIBMAUS2_RANK_DNARANK_SIGMA];
				reco->multiRank(B.reco, B.reco + B.size, &R0[0], &R1[0]);

				DNARankBiDirRange R;
				unsigned int const rsym = sym^3;
				R.reco = R0[rsym] + reco->getD(rsym);
				R.size = R1[rsym]-R0[rsym];
				R.forw = B.forw;

				for ( unsigned int i = rsym+1; i < LIBMAUS2_RANK_DNARANK_SIGMA; ++i )
					R.forw += (R1[i]-R0[i]);

				return R;
			}

			/**
			 * search s of length k by k calls to extendLeft
			 **/
			DNARankBiDirRange searchBackward(char const * s, uint64_t const k) const
			{
				DNARankBiDirRange R = epsilon();
				for ( uint64_t i = 0; i < k; ++i )
					R = extendLeft(R,s[k-i-1]);
				return R;
			}

			/**
			 * search s of length k by k calls to extendRight
			 **/
			DNARankBiDirRange searchForward(char const * s, uint64_t const k) const
			{
				DNARankBiDirRange R = epsilon();
				for ( uint64_t i = 0; i < k; ++i )
					R = extendRight(R,s[i]);
				return R;
			}

			/**
			 * test searching for all k-mers
			 **/
			void testSearch(unsigned int k, uint64_t const numthreads) const
			{
				uint64_t const lim = 1ull << (DNARank::getLogSigma()*k);

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
						C[i] = (z >> (DNARank::getLogSigma()*i)) & (DNARank::getSigma()-1);
					for ( unsigned int i = 0; i < k; ++i )
						R[i] = C[k-i-1]^3;

					std::pair<uint64_t,uint64_t> const P = forw->backwardSearch(C,k);
					std::pair<uint64_t,uint64_t> const PR = reco->backwardSearch(R,k);

					cerrlock.lock();
					std::cerr << "bidir checking ";
					for ( uint64_t i = 0; i < k; ++i )
						std::cerr << static_cast<int>(C[i]);
					std::cerr << " [" << P.first << "," << P.second << ")" << std::endl;
					cerrlock.unlock();

					for ( uint64_t r = P.first; r < P.second; ++r )
					{
						forw->decode(r,k,D);
						assert ( memcmp(C,D,k) == 0 );
					}

					if ( P.first )
					{
						forw->decode(P.first-1,k,D);
						assert ( memcmp(C,D,k) != 0 );
					}
					if ( P.second < forw->size() )
					{
						forw->decode(P.second,k,D);
						assert ( memcmp(C,D,k) != 0 );
					}

					assert ( PR.second-PR.first == P.second-P.first );

					DNARankBiDirRange const BR = searchBackward(C,k);
					DNARankBiDirRange const BF = searchForward(C,k);

					assert ( P.second-P.first == BR.size );
					assert ( (!BR.size) || (P.first == BR.forw) );

					assert ( P.second-P.first == BF.size );
					assert ( (!BF.size) || (P.first == BF.forw) );
				}
			}

		};
	}
}
#endif
