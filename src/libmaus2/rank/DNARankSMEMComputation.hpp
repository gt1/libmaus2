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

#include <libmaus2/util/SimpleQueue.hpp>
#include <libmaus2/rank/DNARank.hpp>
#include <libmaus2/rank/DNARankKmerCache.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <libmaus2/gamma/GammaIntervalEncoder.hpp>
#include <libmaus2/gamma/GammaIntervalDecoder.hpp>
#include <libmaus2/util/FiniteSizeHeap.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankSMEMComputation
		{

			/**
			 * enumerator for smems
			 **/
			template<typename iterator>
			struct SMEMEnumerator
			{
				libmaus2::rank::DNARank const & rank;
				libmaus2::rank::DNARankKmerCache const * cache;
				iterator const it;
				uint64_t const left;
				uint64_t const right;
				uint64_t const minfreq;
				uint64_t const minlen;
				uint64_t const maxlen;

				uint64_t const minsplitlength;
				uint64_t const minsplitsize;

				DNARankSMEMContext context;
				DNARankSMEMContext splitcontext;
				uint64_t next;

				libmaus2::util::SimpleQueue<DNARankMEM> Q;


				struct MEMComp
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

				libmaus2::util::FiniteSizeHeap<DNARankMEM,MEMComp> F;
				DNARankMEM prevMEM;

				libmaus2::autoarray::AutoArray<char> CV;

				SMEMEnumerator(
					libmaus2::rank::DNARank const & rrank,
					iterator rit,
					uint64_t const rleft,
					uint64_t const rright,
					uint64_t const rminfreq,
					uint64_t const rminlen,
					uint64_t const rmaxlen,
					uint64_t const rminsplitlength,
					uint64_t const rminsplitsize
				) : rank(rrank), cache(0), it(rit), left(rleft), right(rright), minfreq(rminfreq), minlen(rminlen), maxlen(rmaxlen), minsplitlength(rminsplitlength), minsplitsize(rminsplitsize), context(), next(left), F(1024)
				{
					prevMEM.left = 0;
				}

				SMEMEnumerator(
					libmaus2::rank::DNARank const & rrank,
					libmaus2::rank::DNARankKmerCache const * rcache,
					iterator rit,
					uint64_t const rleft,
					uint64_t const rright,
					uint64_t const rminfreq,
					uint64_t const rminlen,
					uint64_t const rmaxlen,
					uint64_t const rminsplitlength,
					uint64_t const rminsplitsize
				) : rank(rrank), cache(rcache), it(rit), left(rleft), right(rright), minfreq(rminfreq), minlen(rminlen), maxlen(rmaxlen), minsplitlength(rminsplitlength), minsplitsize(rminsplitsize), context(), next(left), F(1024), CV(right-left)
				{
					prevMEM.left = 0;

					assert ( right >= left );
					uint64_t const range = right-left;
					std::fill(CV.begin(),CV.begin()+range,0);
					uint64_t const numk = (range >= minlen) ? (range-minlen+1) : 0;

					for ( uint64_t i = 0; i < numk; ++i )
					{
						std::pair<uint64_t,uint64_t> const P = cache->search(it+left+i,minlen);
						if ( P.second-P.first >= minfreq )
						{
							uint64_t const p = left + i;
							int64_t const ifrom = std::max(static_cast<int64_t>(p)-static_cast<int64_t>(minlen)+1,static_cast<int64_t>(left));
							int64_t const ito = std::min(p+minlen,right);

							for ( int64_t i = ifrom; i < ito; ++i )
							{
								assert ( i >= left );
								assert ( i < right );
								CV [ i - left ] = 1;
							}
						}
					}
				}

				bool getNext(DNARankMEM & mem)
				{
					while ( Q.empty() && next < right )
					{
						context.resetMatch();
						uint64_t const p = next++;

						if ( cache && !CV[p-left] )
							continue;

						int64_t const ileft = std::max(
							static_cast<int64_t>(left),
							static_cast<int64_t>(p)-static_cast<int64_t>(maxlen)
						);
						int64_t const iright = std::min(p+maxlen,right);

						#if 0
						for ( int64_t i = ileft; i < iright; ++i )
							assert ( it[i] < 4 );
						#endif

						rank.smem(it,ileft,iright,p,minfreq,minlen,context);

						for ( DNARankSMEMContext::match_iterator ita = context.mbegin(); ita != context.mend(); ++ita )
						{
							assert ( ita->left+maxlen >= p );
							assert ( ita->right > p );
							F.pushBump(*ita);

							uint64_t const sdiam = ita->right-ita->left;
							if ( sdiam >= minsplitlength && ita->intv.size <= minsplitsize )
							{
								uint64_t const centre = (ita->left+ita->right)/2;
								assert ( static_cast<int64_t>(centre) >= static_cast<int64_t>(p) - static_cast<int64_t>((maxlen+1)/2) );

								int64_t const ileft = std::max(
									static_cast<int64_t>(left),
									static_cast<int64_t>(centre)-static_cast<int64_t>(maxlen)
								);
								int64_t const iright = std::min(centre+maxlen,right);

								for ( int64_t i = ileft; i < iright; ++i )
									assert ( it[i] < 4 );

								splitcontext.resetMatch();
								rank.smem(it,ileft,iright,centre,ita->intv.size+1,minlen,splitcontext);

								for ( DNARankSMEMContext::match_iterator sita = splitcontext.mbegin(); sita != splitcontext.mend(); ++sita )
								{
									assert ( sita->left + maxlen + (maxlen+1)/2 >= p );
									F.pushBump(*sita);
								}
							}
						}

						while ( ! F.empty() && F.top().left+maxlen+((maxlen+1)/2) <= p )
						{
							DNARankMEM mem = F.pop();
							if ( Q.empty() || mem != Q.back() )
								Q.push_back(mem);
						}
					}
					if ( next == right )
						while ( ! F.empty() )
						{
							DNARankMEM mem = F.pop();
							if ( Q.empty() || mem != Q.back() )
								Q.push_back(mem);
						}

					if ( Q.empty() )
						return false;
					else
					{
						mem = Q.front();
						Q.pop_front();
						assert ( mem.left >= prevMEM.left );
						assert ( mem != prevMEM );
						prevMEM = mem;
						return true;
					}
				}
			};

			/**
			 * enumerator for kmers
			 **/
			template<typename iterator>
			struct KmerEnumerator
			{
				libmaus2::rank::DNARank const & rank;
				libmaus2::rank::DNARankKmerCache const * cache;
				iterator const it;
				uint64_t const left;
				uint64_t const right;
				uint64_t const minfreq;
				uint64_t const kmersize;
				uint64_t next;

				KmerEnumerator(
					libmaus2::rank::DNARank const & rrank,
					iterator rit,
					uint64_t const rleft,
					uint64_t const rright,
					uint64_t const rminfreq,
					uint64_t const rkmersize
				) : rank(rrank), cache(0), it(rit), left(rleft), right(rright), minfreq(rminfreq), kmersize(rkmersize), next(left)
				{
				}

				KmerEnumerator(
					libmaus2::rank::DNARank const & rrank,
					libmaus2::rank::DNARankKmerCache const * rcache,
					iterator rit,
					uint64_t const rleft,
					uint64_t const rright,
					uint64_t const rminfreq,
					uint64_t const rkmersize
				) : rank(rrank), cache(rcache), it(rit), left(rleft), right(rright), minfreq(rminfreq), kmersize(rkmersize), next(left)
				{
				}

				bool getNext(DNARankMEM & mem)
				{
					while ( next < right )
					{
						uint64_t const p = next++;

						std::pair<uint64_t,uint64_t> const P =
							cache ?
								cache->search(it+p,kmersize)
								:
								rank.backwardSearch(it+p,kmersize);

						if ( P.second - P.first && P.second-P.first >= minfreq )
						{
							mem.left = next;
							mem.right = next + kmersize;
							mem.intv = rank.backwardSearchBi(it+p,kmersize);
							assert ( mem.intv.size == P.second-P.first );
							return true;
						}
					}

					return false;
				}
			};

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

			/**
			 * compute active regions in [left,right) containing matches of length >= minlen with frequency >= minfreq
			 * split by inactive regions of size >= mininactive
			 **/
			template<typename iterator>
			static std::string activeParallel(
				libmaus2::util::TempFileNameGenerator & tmpgen,
				libmaus2::rank::DNARankKmerCache const & cache,
				iterator it,
				uint64_t const left,
				uint64_t const right,
				uint64_t const minfreq,
				uint64_t const minlen,
				uint64_t const numthreads,
				uint64_t const mininactive
			)
			{
				uint64_t const range = right-left;
				uint64_t const numkmers = (range >= minlen) ? (range-minlen+1) : 0;

				if ( numkmers )
				{
					// minimum number of blocks (if possible)
					uint64_t const minblocks = 16*numthreads;
					// target block size
					uint64_t const tblocksize = 256*1024ull;
					// target number of blocks
					uint64_t const tblocks = (numkmers + tblocksize - 1)/tblocksize;
					// increase if smaller than minblocks
					uint64_t const ttblocks = std::max(minblocks,tblocks);

					// actual blocksize
					uint64_t const blocksize = (numkmers + ttblocks - 1)/ttblocks;
					// actual number of blocks
					uint64_t const numblocks = (numkmers + blocksize - 1)/blocksize;

					// std::cerr << "[V] num blocks " << numblocks << " block size " << blocksize << std::endl;

					// next block id taken
					uint64_t volatile nextblockid = 0;
					libmaus2::parallel::PosixSpinLock lock;

					// set up writers
					std::vector<std::string> Vfn(numthreads);
					libmaus2::autoarray::AutoArray< libmaus2::gamma::GammaIntervalEncoder::unique_ptr_type > Aintenc(numthreads);
					for ( uint64_t i = 0; i < numthreads; ++i )
					{
						Vfn[i] = tmpgen.getFileName(true);
						libmaus2::gamma::GammaIntervalEncoder::unique_ptr_type Tenc(new libmaus2::gamma::GammaIntervalEncoder(Vfn[i]));
						Aintenc[i] = UNIQUE_PTR_MOVE(Tenc);

					}

					// process blocks
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
					#endif
					for ( uint64_t tblockid = 0; tblockid < numblocks; ++tblockid )
					{
						// thread id
						#if defined(_OPENMP)
						uint64_t const tid = omp_get_thread_num();
						#else
						uint64_t const tid = 0;
						#endif

						// get encoder
						libmaus2::gamma::GammaIntervalEncoder * enc = Aintenc[tid].get();

						// get block id
						uint64_t blockid;
						lock.lock();
						blockid = nextblockid++;
						lock.unlock();

						// block bounds
						uint64_t const blocklow = blockid * blocksize;
						uint64_t const blockhigh = std::min(blocklow+blocksize,numkmers);
						assert ( blockhigh - blocklow );

						// is first position active?
						bool active;
						{
							std::pair<uint64_t,uint64_t> const P = cache.search(it+blocklow,minlen);
							active = (P.second-P.first) >= minfreq;
						}

						// start of previous range
						uint64_t prevstart = blocklow;

						// iterate over non first positions
						for ( uint64_t z = blocklow+1; z < blockhigh; ++z )
						{
							// search
							std::pair<uint64_t,uint64_t> const P = cache.search(it+z,minlen);

							// new active values
							bool const newactive = ((P.second-P.first) >= minfreq);

							// if different from previous
							if ( newactive != active )
							{
								// should be non empty
								assert ( z > prevstart );

								// encode interval if inactive
								if ( ! active )
									enc->put(std::pair<uint64_t,uint64_t>(prevstart,z));

								active = newactive;
								prevstart = z;
							}
						}
					}

					// flush encoders
					for ( uint64_t i = 0; i < numthreads; ++i )
					{
						Aintenc[i]->flush();
						Aintenc[i].reset();
					}

					// heap data structure
					struct HeapEntry
					{
						std::pair<uint64_t,uint64_t> intv;
						uint64_t id;

						bool operator<(HeapEntry const & H) const
						{
							if ( intv.first != H.intv.first )
								return intv.first < H.intv.first;
							else if ( intv.second != H.intv.second )
								return intv.second < H.intv.second;
							else
								return id < H.id;
						}

						HeapEntry()
						{

						}
						HeapEntry(std::pair<uint64_t,uint64_t> rintv, uint64_t rid) : intv(rintv), id(rid) {}
					};

					// heap
					libmaus2::util::FiniteSizeHeap<HeapEntry> H(numthreads);
					// decoders
					libmaus2::autoarray::AutoArray< libmaus2::gamma::GammaIntervalDecoder::unique_ptr_type > Aintdec(numthreads);

					// set up decoders and fill heap
					for ( uint64_t i = 0; i < numthreads; ++i )
					{
						libmaus2::gamma::GammaIntervalDecoder::unique_ptr_type tdec(new libmaus2::gamma::GammaIntervalDecoder(std::vector<std::string>(1,Vfn[i]),0/*offset*/,1/* numthreads */));
						Aintdec[i] = UNIQUE_PTR_MOVE(tdec);

						std::pair<uint64_t,uint64_t> P;
						if ( Aintdec[i]->getNext(P) )
						{
							H.push(HeapEntry(P,i));
						}
						else
						{
							Aintdec[i].reset();
							libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
						}
					}

					// previous
					std::pair<uint64_t,uint64_t> Pprev(0,0);
					// output file name
					std::string const outfn = tmpgen.getFileName(true);
					// encoder for merged inactive intervals
					libmaus2::gamma::GammaIntervalEncoder::unique_ptr_type Penc(new libmaus2::gamma::GammaIntervalEncoder(outfn));

					while ( ! H.empty() )
					{
						// get top of merge heap
						HeapEntry T = H.pop();

						// mergeable
						if ( T.intv.first == Pprev.second )
						{
							Pprev.second = T.intv.second;
						}
						else
						{
							// if non empty
							if ( expect_true(Pprev.second != Pprev.first) )
								// if sufficietly large
								if ( Pprev.second-Pprev.first >= mininactive )
									Penc->put(Pprev);

							Pprev = T.intv;
						}

						// try to get next
						if ( Aintdec[T.id]->getNext(T.intv) )
							H.push(T);
						else
						{
							Aintdec[T.id].reset();
							libmaus2::aio::FileRemoval::removeFile(Vfn[T.id]);
						}
					}

					// put final interval
					if ( expect_true(Pprev.second != Pprev.first) )
						if ( Pprev.second-Pprev.first >= mininactive )
							Penc->put(Pprev);

					Penc->flush();
					Penc.reset();

					// set up encoder for active intervals
					std::string const outfinfn = tmpgen.getFileName(true);
					libmaus2::gamma::GammaIntervalDecoder::unique_ptr_type Pdec(new libmaus2::gamma::GammaIntervalDecoder(std::vector<std::string>(1,outfn),0/*offset */,1 /* numthreads */));
					libmaus2::gamma::GammaIntervalEncoder::unique_ptr_type Pcomp(new libmaus2::gamma::GammaIntervalEncoder(outfinfn));

					// active low
					uint64_t actlow = left;
					std::pair<uint64_t,uint64_t> P;
					// get next inactive interval
					while ( Pdec->getNext(P) )
					{
						// active up to start of inactive
						if ( P.first > actlow )
							Pcomp->put(std::pair<uint64_t,uint64_t>(actlow,P.first));

						// active from end of inactive
						actlow = P.second;
					}
					// put final if non empty
					if ( actlow != right )
						Pcomp->put(std::pair<uint64_t,uint64_t>(actlow,right));
					Pcomp->flush();
					Pcomp.reset();
					Pdec.reset();

					libmaus2::aio::FileRemoval::removeFile(outfn);

					#if 0
					libmaus2::gamma::GammaIntervalDecoder::unique_ptr_type Pdec(new libmaus2::gamma::GammaIntervalDecoder(std::vector<std::string>(1,outfn),0/*offset */,1 /* numthreads */));
					std::pair<uint64_t,uint64_t> P;
					while ( Pdec->getNext(P) )
					{
						if ( P.second-P.first >= 700 )
							std::cerr << "[V] inactive [" << P.first << "," << P.second << ")" << std::endl;
					}
					#endif

					return outfinfn;
				}
				else
				{
					std::string const outfn = tmpgen.getFileName(true);
					libmaus2::gamma::GammaIntervalEncoder::unique_ptr_type Penc(new libmaus2::gamma::GammaIntervalEncoder(outfn));
					Penc->flush();
					Penc.reset();
					return outfn;
				}
			}

			template<typename iterator>
			static void smemLimitedParallel(
				libmaus2::rank::DNARank const & rank,
				libmaus2::rank::DNARankKmerCache const & cache,
				iterator it,
				uint64_t const left,
				uint64_t const right,
				uint64_t const n,
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

						int64_t const aleft =
							std::max(
								static_cast<int64_t>(blow) - static_cast<int64_t>(minlen) + static_cast<int64_t>(1),
								static_cast<int64_t>(0)
							);
						int64_t const aright =
							std::min(
								bhigh,
								n-minlen+1
							);

						for ( int64_t i = aleft; i < aright; ++i )
						{
							std::pair<uint64_t,uint64_t> const P = cache.search(it+i,minlen);

							if ( P.second-P.first >= minfreq )
								for ( int64_t j = i; j < i+minlen; ++j )
									if ( j >= blow && j < bhigh )
										active[j-blow] = 1;
						}

						for ( uint64_t i0 = blow; i0 < bhigh; ++i0 )
							if ( active[i0-blow] )
							{
								int64_t const ileft =
									std::max(
										static_cast<int64_t>(i0)-static_cast<int64_t>(limit),
										static_cast<int64_t>(0)
									);
								int64_t const iright =
									std::min(i0+limit,n);

								rank.smem(it,ileft,iright,i0,minfreq,minlen,context);
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
