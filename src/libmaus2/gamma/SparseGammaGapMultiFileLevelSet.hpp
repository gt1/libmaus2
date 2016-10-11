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
#if ! defined(LIBMAUS2_GAMMA_SPARSEGAMMAGAPMULTIFILELEVELSET_HPP)
#define LIBMAUS2_GAMMA_SPARSEGAMMAGAPMULTIFILELEVELSET_HPP

#include <libmaus2/gamma/SparseGammaGapDecoder.hpp>
#include <libmaus2/gamma/SparseGammaGapMultiFile.hpp>
#include <libmaus2/gamma/SparseGammaGapMerge.hpp>
#include <libmaus2/gamma/GammaGapEncoder.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <libmaus2/parallel/PosixSemaphore.hpp>
#include <queue>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type >
		struct SparseGammaGapMultiFileLevelSetTemplate
		{
			typedef _data_type data_type;

			libmaus2::util::TempFileNameGenerator & tmpgen;
			std::map< uint64_t,std::deque<libmaus2::gamma::SparseGammaGapMultiFile> > L;
			libmaus2::parallel::OMPLock lock;
			uint64_t addcnt;
			uint64_t parts;

			uint64_t nextmergeQid;
			std::map<uint64_t, typename SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo::shared_ptr_type> mergehandoutq;
			std::map<uint64_t, typename SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo::shared_ptr_type> mergefinishq;
			std::map<uint64_t, typename SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo::shared_ptr_type> mergeallq;

			std::vector<libmaus2::parallel::PosixSemaphore *> mergepacksem;
			std::vector<libmaus2::parallel::PosixSemaphore *> termsem;
			uint64_t termsemcnt;
			libmaus2::parallel::OMPLock semlock;

			SparseGammaGapMultiFileLevelSetTemplate(
				libmaus2::util::TempFileNameGenerator & rtmpgen,
				uint64_t const rparts
			) : tmpgen(rtmpgen), addcnt(0), parts(rparts), nextmergeQid(0) {}

			void registerMergePackSemaphore(libmaus2::parallel::PosixSemaphore * sema)
			{
				mergepacksem.push_back(sema);
			}

			void registerTermSemaphore(libmaus2::parallel::PosixSemaphore * sema)
			{
				termsem.push_back(sema);
			}

			void setTermSemCnt(uint64_t const rtermsemcnt)
			{
				termsemcnt = rtermsemcnt;
			}

			bool trySemPair(libmaus2::parallel::PosixSemaphore & A, libmaus2::parallel::PosixSemaphore & B)
			{
				semlock.lock();

				bool const res = A.trywait();

				if ( res )
					B.wait();

				semlock.unlock();

				return res;
			}

			/**
			 * check whether merging is possible and return a pair to be merged if it is
			 **/
			bool needMerge(
				uint64_t & l,
				std::pair<libmaus2::gamma::SparseGammaGapMultiFile,libmaus2::gamma::SparseGammaGapMultiFile> & P
			)
			{
				libmaus2::parallel::ScopeLock slock(lock);

				for ( std::map< uint64_t,std::deque<libmaus2::gamma::SparseGammaGapMultiFile> >::iterator ita = L.begin();
					ita != L.end(); ++ita )
					if ( ita->second.size() > 1 )
					{
						l = ita->first;

						P.first = ita->second.front();
						ita->second.pop_front();

						P.second = ita->second.front();
						ita->second.pop_front();

						if ( ! ita->second.size() )
							L.erase(L.find(l));

						return true;
					}

				return false;
			}

			SparseGammaGapMultiFile doMerge(
				uint64_t const l,
				std::pair<libmaus2::gamma::SparseGammaGapMultiFile,libmaus2::gamma::SparseGammaGapMultiFile> const & P,
				std::string const & nfn // output file prefix
			)
			{
				SparseGammaGapMultiFile const Sa = P.first;
				SparseGammaGapMultiFile const Sb = P.second;

				std::vector<std::string> const fno =
					libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::merge(Sa.fn,Sb.fn,nfn,parts);

				SparseGammaGapMultiFile N(fno,l+1);

				// remove input files
				for ( uint64_t i = 0; i < Sa.fn.size(); ++i )
					libmaus2::aio::FileRemoval::removeFile(Sa.fn[i]);
				for ( uint64_t i = 0; i < Sb.fn.size(); ++i )
					libmaus2::aio::FileRemoval::removeFile(Sb.fn[i]);

				return N;
			}

			bool checkMergePacket(uint64_t & packetid, uint64_t & subid)
			{
				libmaus2::parallel::ScopeLock slock(lock);

				if ( ! mergehandoutq.size() )
					return false;

				typename std::map<uint64_t, typename SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo::shared_ptr_type>::iterator it = mergehandoutq.begin();
				assert ( it != mergehandoutq.end() );

				packetid = it->first;
				#if ! defined(NDEBUG)
				bool const ok =
				#endif
					it->second->getNextDispatchId(subid);

				#if ! defined(NDEBUG)
				assert ( ok );
				#endif

				// is this the last subid?
				if ( it->second->isHandoutFinished() )
				{
					mergefinishq[it->first] = it->second;
					mergehandoutq.erase(it);
				}

				return true;
			}

			typename SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo * getMergeInfo(uint64_t const packetid)
			{
				libmaus2::parallel::ScopeLock slock(lock);
				typename SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo * ptr = mergeallq.find(packetid)->second.get();
				return ptr;
			}

			// this method is not thread safe, so lock must be held when calling this method in parallel mode
			typename libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo * queueMergeInfo(libmaus2::gamma::SparseGammaGapMultiFile const & A, libmaus2::gamma::SparseGammaGapMultiFile const & B)
			{
				typename libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo SGGMI =
					libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::getMergeInfoDelayedInit(
						A.fn,B.fn,tmpgen.getFileName(),parts
					);
				SGGMI.setLevel(std::max(A.level,B.level)+1);

				typename libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo::shared_ptr_type sptr(
					new typename libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo
				);

				*sptr = SGGMI;

				uint64_t const mergeid = nextmergeQid++;
				mergehandoutq[mergeid] = sptr;
				mergeallq[mergeid] = sptr;

				sptr->setSemaphoreInfo(&semlock,&mergepacksem);

				return sptr.get();
				// sptr->initialise();
			}

			bool isMergingQueueEmpty()
			{
				libmaus2::parallel::ScopeLock slock(lock);
				return mergeallq.size() == 0;
			}

			void putFile(std::vector<std::string> const & fn)
			{
				for ( uint64_t i = 0; i < fn.size(); ++i )
					libmaus2::util::TempFileRemovalContainer::addTempFile(fn[i]);

				SparseGammaGapMultiFile Sa(fn,0);
				SparseGammaGapMultiFile Sb;
				typename libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo * nptr = 0;

				{
					libmaus2::parallel::ScopeLock slock(lock);
					addcnt += 1;

					if ( L[0].size() )
					{
						Sb = L[0].front();
						L[0].pop_front();
						nptr = queueMergeInfo(Sa,Sb);
					}
					else
						L[0].push_back(Sa);
				}

				if ( nptr )
					nptr->initialise();
			}

			void checkMergeSingle(bool const checkterm = false)
			{
				uint64_t packetid = 0;
				uint64_t subid = 0;

				if ( checkMergePacket(packetid,subid) )
				{
					typename SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo * ptr = getMergeInfo(packetid);
					ptr->dispatch(subid);

					if ( ptr->incrementFinished() )
					{
						typename libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo * nptr = 0;

						ptr->removeInputFiles();

						{
							libmaus2::parallel::ScopeLock slock(lock);
							uint64_t const level = ptr->getLevel();

							libmaus2::gamma::SparseGammaGapMultiFile Sa(ptr->getOutputFileNames(),level);

							if ( L[level].size() )
							{
								libmaus2::gamma::SparseGammaGapMultiFile Sb = L[level].front();
								L[level].pop_front();
								nptr = queueMergeInfo(Sa,Sb);
							}
							else
								L[level].push_front(Sa);

							assert ( mergeallq.find(packetid) != mergeallq.end() );
							assert ( mergehandoutq.find(packetid) == mergehandoutq.end() );
							assert ( mergefinishq.find(packetid) != mergefinishq.end() );

							mergeallq.erase(mergeallq.find(packetid));
							mergefinishq.erase(mergefinishq.find(packetid));

							if ( checkterm && (mergeallq.size() == 0) )
							{
								semlock.lock();
								for ( uint64_t i = 0; i < termsemcnt; ++i )
									for ( uint64_t j = 0; j < termsem.size(); ++j )
										termsem[j]->post();
								semlock.unlock();
							}
						}

						if ( nptr )
							nptr->initialise();
					}
				}
			}

			void checkMerge()
			{
				bool finished = false;

				while ( ! finished )
				{
					finished = true;

					uint64_t l = 0;
					std::pair<libmaus2::gamma::SparseGammaGapMultiFile,libmaus2::gamma::SparseGammaGapMultiFile> P;
					uint64_t packetid = 0;
					uint64_t subid = 0;
					typename libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo * nptr = 0;

					if ( needMerge(l,P) )
					{
						finished = false;
						libmaus2::parallel::ScopeLock slock(lock);
						nptr = queueMergeInfo(P.first,P.second);
					}
					else if ( checkMergePacket(packetid,subid) )
					{
						finished = false;

						typename SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo * ptr = getMergeInfo(packetid);
						ptr->dispatch(subid);

						if ( ptr->incrementFinished() )
						{
							ptr->removeInputFiles();

							libmaus2::parallel::ScopeLock slock(lock);
							uint64_t const l = ptr->getLevel();
							libmaus2::gamma::SparseGammaGapMultiFile const N(ptr->getOutputFileNames(),l);
							L[l].push_back(N);

							assert ( mergeallq.find(packetid) != mergeallq.end() );
							assert ( mergehandoutq.find(packetid) == mergehandoutq.end() );
							assert ( mergefinishq.find(packetid) != mergefinishq.end() );

							mergeallq.erase(mergeallq.find(packetid));
							mergefinishq.erase(mergefinishq.find(packetid));
						}
					}

					if ( nptr )
						nptr->initialise();
				}
			}

			void addFile(std::vector<std::string> const & fn)
			{
				for ( uint64_t i = 0; i < fn.size(); ++i )
					libmaus2::util::TempFileRemovalContainer::addTempFile(fn[i]);

				SparseGammaGapMultiFile S(fn,0);

				{
					libmaus2::parallel::ScopeLock slock(lock);
					addcnt += 1;
					L[0].push_back(S);
				}

				checkMerge();
			}

			void addFile(std::string const & fn)
			{
				addFile(std::vector<std::string>(1,fn));
			}

			std::vector<std::string> merge(
				std::string const & outputfilenameprefix,
				uint64_t const
					#if defined(_OPENMP)
					numthreads
					#endif
			)
			{
				// set up merge queue Q
				std::priority_queue<libmaus2::gamma::SparseGammaGapMultiFile> Q;

				for ( std::map< uint64_t,std::deque<libmaus2::gamma::SparseGammaGapMultiFile> >::iterator ita = L.begin();
					ita != L.end(); ++ita )
					for ( uint64_t i = 0; i < ita->second.size(); ++i )
						Q.push(ita->second[i]);

				// erase level data structure
				L.clear();

				// do merging
				while ( Q.size() > 1 )
				{
					std::pair<libmaus2::gamma::SparseGammaGapMultiFile,libmaus2::gamma::SparseGammaGapMultiFile> P;
					P.first = Q.top(); Q.pop();
					P.second = Q.top(); Q.pop();

					typename libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo * nptr = queueMergeInfo(P.first,P.second);
					nptr->initialise();

					#if defined(_OPENMP)
					#pragma omp parallel num_threads(numthreads)
					#endif
					{
						uint64_t packetid = 0, subid = 0;

						while ( checkMergePacket(packetid,subid) )
						{
							typename SparseGammaGapMergeTemplate<data_type>::SparseGammaGapMergeInfo * ptr = getMergeInfo(packetid);
							ptr->dispatch(subid);

							libmaus2::parallel::ScopeLock slock(lock);

							if ( ptr->incrementFinished() )
							{
								ptr->removeInputFiles();

								libmaus2::gamma::SparseGammaGapMultiFile const N(ptr->getOutputFileNames(),P.second.level+1);
								Q.push(N);

								assert ( mergeallq.find(packetid) != mergeallq.end() );
								assert ( mergehandoutq.find(packetid) == mergehandoutq.end() );
								assert ( mergefinishq.find(packetid) != mergefinishq.end() );

								mergeallq.erase(mergeallq.find(packetid));
								mergefinishq.erase(mergefinishq.find(packetid));
							}
						}
					}
				}

				std::vector<std::string> outputfilenames;

				// rename files if there are any
				if ( !Q.empty() )
				{
					for ( uint64_t i = 0; i < Q.top().fn.size(); ++i )
					{
						std::ostringstream fnostr;
						fnostr << outputfilenameprefix << "_" << std::setw(6) << std::setfill('0') << i;
						std::string const fn = fnostr.str();
						libmaus2::util::TempFileRemovalContainer::addTempFile(fn);
						libmaus2::aio::OutputStreamFactoryContainer::rename(Q.top().fn[i].c_str(),fn.c_str());
						outputfilenames.push_back(fn);
					}
				}

				return outputfilenames;
			}

			/**
			 * merge sparse gamma coded gap files to a set of dense files
			 **/
			std::vector<std::string> mergeToDense(libmaus2::util::TempFileNameGenerator & gtmpgen, uint64_t const n, uint64_t const numthreads)
			{
				std::string const tmpfilename = tmpgen.getFileName();
				std::vector<std::string> const fno = merge(tmpfilename,numthreads);
				// uint64_t const lparts = 1;
				uint64_t const lparts = parts;
				uint64_t const partsize = (n+lparts-1)/lparts;
				uint64_t const aparts = (n+partsize-1)/partsize;
				std::vector<std::string> outputfilenames(aparts);

				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
				#endif
				for ( uint64_t p = 0; p < aparts; ++p )
				{
					std::ostringstream fnostr;
					fnostr << gtmpgen.getFileName() << "_" << std::setw(6) << std::setfill('0') << p;
					std::string const fn = fnostr.str();
					outputfilenames[p] = fn;

					uint64_t const low = std::min(p * partsize,n);
					uint64_t const high = std::min(low+partsize,n);

					libmaus2::gamma::SparseGammaGapConcatDecoderTemplate<data_type> SGGD(fno,low);
					typename libmaus2::gamma::SparseGammaGapConcatDecoderTemplate<data_type>::iterator it = SGGD.begin();

					libmaus2::gamma::GammaGapEncoder GGE(fn);
					GGE.encode(it,high-low);
				}

				#if 0
				std::cerr << "verifying." << std::endl;
				libmaus2::gamma::SparseGammaGapConcatDecoderTemplate<data_type> SGGD(fno);
				libmaus2::gamma::GammaGapDecoder GGD(outputfilenames);
				for ( uint64_t i = 0; i < n; ++i )
				{
					uint64_t const v0= SGGD.decode();
					uint64_t const v1 = GGD.decode();
					assert ( v0 == v1 );
				}
				std::cerr << "done." << std::endl;
				#endif

				for ( uint64_t i = 0; i < fno.size(); ++i )
					libmaus2::aio::FileRemoval::removeFile(fno[i]);

				return outputfilenames;
			}
		};

		typedef SparseGammaGapMultiFileLevelSetTemplate<uint64_t> SparseGammaGapMultiFileLevelSet;
		typedef SparseGammaGapMultiFileLevelSetTemplate< libmaus2::math::UnsignedInteger<4> > SparseGammaGapMultiFileLevelSet2;
	}
}
#endif
