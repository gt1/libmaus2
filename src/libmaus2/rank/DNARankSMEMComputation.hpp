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

#if ! defined(LIBMAUS2_RANK_DNARANKSMEMCOMPUTATION_HPP)
#define LIBMAUS2_RANK_DNARANKSMEMCOMPUTATION_HPP

#include <libmaus2/rank/DNARank.hpp>
#include <libmaus2/rank/DNARankKmerCache.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankSMEMComputation
		{
			template<typename iterator>
			static void smemSerial(
				libmaus2::rank::DNARank const & rank,
				iterator it,
				uint64_t const left,
				uint64_t const right,
				// uint64_t const i0,
				uint64_t const minfreq,
				uint64_t const minlen,
				DNARankSMEMContext & context
			)
			{
				context.resetMatch();
				for ( uint64_t i0 = left; i0 < right; ++i0 )
					rank.smem(it,left,right,i0,minfreq,minlen,context);
			}

			template<typename iterator>
			static void smemParallel(
				libmaus2::rank::DNARank const & rank,
				iterator it,
				uint64_t const left,
				uint64_t const right,
				uint64_t const minfreq,
				uint64_t const minlen,
				std::vector<DNARankMEM> & SMEM,
				uint64_t const numthreads
			)
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
							rank.smem(it,left,right,i0,minfreq,minlen,context);

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
			static void smemLimitedParallel(
				libmaus2::rank::DNARank const & rank,
				libmaus2::rank::DNARankKmerCache const & cache,
				iterator it,
				uint64_t const left,
				uint64_t const right,
				uint64_t const minfreq,
				uint64_t const minlen,
				uint64_t const limit,
				std::vector<DNARankMEM> & SMEM,
				uint64_t const numthreads
			)
			{
				uint64_t const packsperthread = 16;
				uint64_t const tpacks = packsperthread * numthreads;
				uint64_t const bs = 64*1024ull;

				uint64_t const posperthread = std::min(static_cast<uint64_t>(64*1024), (right-left + tpacks-1)/tpacks);
				uint64_t const numpack = (right-left) ? ((right-left+posperthread-1)/posperthread) : 0;

				libmaus2::autoarray::AutoArray<DNARankSMEMContext::unique_ptr_type> Acontext(numthreads);
				libmaus2::autoarray::AutoArray< libmaus2::autoarray::AutoArray<uint8_t>::unique_ptr_type > Aactive(numthreads);
				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads)
				#endif
				for ( uint64_t t = 0; t < numthreads; ++t )
				{
					DNARankSMEMContext::unique_ptr_type Pcontext(new DNARankSMEMContext);
					Acontext[t] = UNIQUE_PTR_MOVE(Pcontext);

					libmaus2::autoarray::AutoArray<uint8_t>::unique_ptr_type Tactive(new libmaus2::autoarray::AutoArray<uint8_t>(bs,false));
					Aactive[t] = UNIQUE_PTR_MOVE(Tactive);
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
					libmaus2::autoarray::AutoArray<uint8_t> & active = *(Aactive[tid]);

					uint64_t const numblocks = (thigh-tlow+bs-1)/bs;

					for ( uint64_t b = 0; b < numblocks; ++b )
					{
						uint64_t const blow = tlow + b * bs;
						uint64_t const bhigh = std::min(blow+bs,thigh);
						std::fill(active.begin(),active.end(),0);

						int64_t const checkleft =
							std::max(
								static_cast<int64_t>(left),
								static_cast<int64_t>(blow) - static_cast<int64_t>(minlen) + static_cast<int64_t>(1)
							);
						int64_t const checkright =
							std::min(
								static_cast<int64_t>(right)-static_cast<int64_t>(minlen) + static_cast<int64_t>(1),
								static_cast<int64_t>(bhigh)
							)
						;

						// kmers clipped on the left
						for (
							int64_t i = checkleft;
							i < std::min(static_cast<int64_t>(blow),checkright);
							++i
						)
						{
							assert ( i        >= left );
							assert ( i+minlen <= right );
							std::pair<uint64_t,uint64_t> const P = cache.search(it+i,minlen);
							if ( P.second-P.first >= minfreq )
								for ( int64_t j = i; j < i+minlen; ++j )
									if ( j >= blow && j < bhigh )
									{
										active[j-blow] = 1;
									}
						}
						// full kmers
						for ( int64_t i = blow; i < static_cast<int64_t>(bhigh)-static_cast<int64_t>(minlen)+static_cast<int64_t>(1); ++i )
						{
							assert ( i        >= left );
							assert ( i+minlen <= right );
							assert ( i        >= blow );
							assert ( i+minlen <= bhigh );
							std::pair<uint64_t,uint64_t> const P = cache.search(it+i,minlen);

							if ( P.second-P.first >= minfreq )
								for ( int64_t j = i; j < i+minlen; ++j )
								{
									assert ( j >= blow );
									assert ( j < bhigh );
									active[j-blow] = 1;
								}
						}
						// kmers clipped on the right
						for (
							int64_t i =
								std::max(checkleft,static_cast<int64_t>(bhigh)-static_cast<int64_t>(minlen)+static_cast<int64_t>(1)); i < checkright; ++i
						)
						{
							assert ( i        >= left );
							assert ( i+minlen <= right );
							std::pair<uint64_t,uint64_t> const P = cache.search(it+i,minlen);
							if ( P.second-P.first >= minfreq )
								for ( int64_t j = i; j < i+minlen; ++j )
									if ( j >= blow && j < bhigh )
									{
										active[j-blow] = 1;
									}
						}

						if ( blow >= left+limit && bhigh+limit <= right )
						{
							for ( uint64_t i0 = blow; i0 < bhigh; ++i0 )
								if ( active[i0-blow] )
								{
									rank.smem(it,i0-limit,i0+limit,i0,minfreq,minlen,context);
								}
						}
						else if ( blow >= left+limit )
						{
							for ( uint64_t i0 = blow; i0 < bhigh; ++i0 )
								if ( active[i0-blow] )
								{
									rank.smem(it,i0-limit,right,i0,minfreq,minlen,context);
								}
						}
						else if ( bhigh+limit <= right )
						{
							for ( uint64_t i0 = blow; i0 < bhigh; ++i0 )
								if ( active[i0-blow] )
								{
									rank.smem(it,left,i0+limit,i0,minfreq,minlen,context);
								}
						}
						else
						{
							for ( uint64_t i0 = blow; i0 < bhigh; ++i0 )
								if ( active[i0-blow] )
								{
									rank.smem(it,left,right,i0,minfreq,minlen,context);
								}
						}

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
			static void smemLimitedParallelSplit(
				libmaus2::rank::DNARank const & rank,
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
			)
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
					uint64_t const thigh = std::min(tlow+posperthread,static_cast<uint64_t>(SMEMin.size()));

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

								rank.smem(it,bleft,bright,(sleft+sright)>>1,in.intv.size+1,minlen,context);
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
		};
	}
}
#endif
