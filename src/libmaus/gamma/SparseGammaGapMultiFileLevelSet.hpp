/*
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
*/
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPMULTIFILELEVELSET_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPMULTIFILELEVELSET_HPP

#include <libmaus/gamma/SparseGammaGapDecoder.hpp>
#include <libmaus/gamma/SparseGammaGapMultiFile.hpp>
#include <libmaus/gamma/SparseGammaGapMerge.hpp>
#include <libmaus/gamma/GammaGapEncoder.hpp>
#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/util/TempFileNameGenerator.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>
#include <libmaus/parallel/OMPLock.hpp>
#include <queue>

namespace libmaus
{
	namespace gamma
	{
		struct SparseGammaGapMultiFileLevelSet
		{
			libmaus::util::TempFileNameGenerator & tmpgen;
			std::map< uint64_t,std::deque<libmaus::gamma::SparseGammaGapMultiFile> > L;
			libmaus::parallel::OMPLock lock;
			uint64_t addcnt;
			uint64_t parts;		

			uint64_t nextmergeQid;
			std::map<uint64_t, SparseGammaGapMerge::SparseGammaGapMergeInfo::shared_ptr_type> mergehandoutq;
			std::map<uint64_t, SparseGammaGapMerge::SparseGammaGapMergeInfo::shared_ptr_type> mergefinishq;
			std::map<uint64_t, SparseGammaGapMerge::SparseGammaGapMergeInfo::shared_ptr_type> mergeallq;

			SparseGammaGapMultiFileLevelSet(
				libmaus::util::TempFileNameGenerator & rtmpgen,
				uint64_t const rparts
			) : tmpgen(rtmpgen), addcnt(0), parts(rparts), nextmergeQid(0) {}
			
			/**
			 * check whether merging is possible and return a pair to be merged if it is
			 **/
			bool needMerge(
				uint64_t & l, 
				std::pair<libmaus::gamma::SparseGammaGapMultiFile,libmaus::gamma::SparseGammaGapMultiFile> & P
			)
			{
				libmaus::parallel::ScopeLock slock(lock);
				
				for ( std::map< uint64_t,std::deque<libmaus::gamma::SparseGammaGapMultiFile> >::iterator ita = L.begin();
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
				std::pair<libmaus::gamma::SparseGammaGapMultiFile,libmaus::gamma::SparseGammaGapMultiFile> const & P,
				std::string const & nfn // output file prefix
			)
			{
				SparseGammaGapMultiFile const Sa = P.first;
				SparseGammaGapMultiFile const Sb = P.second;
				
				std::vector<std::string> const fno = 
					libmaus::gamma::SparseGammaGapMerge::merge(Sa.fn,Sb.fn,nfn,parts);

				SparseGammaGapMultiFile N(fno,l+1);
				
				// remove input files
				for ( uint64_t i = 0; i < Sa.fn.size(); ++i )
					remove(Sa.fn[i].c_str());
				for ( uint64_t i = 0; i < Sb.fn.size(); ++i )
					remove(Sb.fn[i].c_str());

				#if 0
				std::cerr << "merged " << Sa << " and " << Sb << " to " << N << std::endl;
				#endif

				return N;
			}
			
			bool checkMergePacket(uint64_t & packetid, uint64_t & subid)
			{
				libmaus::parallel::ScopeLock slock(lock);
				
				if ( ! mergehandoutq.size() )
					return false;
					
				std::map<uint64_t, SparseGammaGapMerge::SparseGammaGapMergeInfo::shared_ptr_type>::iterator it = mergehandoutq.begin();
				assert ( it != mergehandoutq.end() );
				
				packetid = it->first;
				bool const ok = it->second->getNextDispatchId(subid);
				
				assert ( ok );

				// is this the last subid?
				if ( it->second->next == it->second->fno.size() )
				{
					mergefinishq[it->first] = it->second;
					mergehandoutq.erase(it);
				}
				
				return true;
			}
			
			SparseGammaGapMerge::SparseGammaGapMergeInfo * getMergeInfo(uint64_t const packetid)
			{
				libmaus::parallel::ScopeLock slock(lock);
				SparseGammaGapMerge::SparseGammaGapMergeInfo * ptr = mergeallq.find(packetid)->second.get();
				return ptr;
			}
			
			void queueMergeInfo(libmaus::gamma::SparseGammaGapMultiFile const & A, libmaus::gamma::SparseGammaGapMultiFile const & B)
			{
				libmaus::gamma::SparseGammaGapMerge::SparseGammaGapMergeInfo SGGMI = 
					libmaus::gamma::SparseGammaGapMerge::getMergeInfo(A.fn,B.fn,tmpgen.getFileName(),parts);
						
				if ( SGGMI.fno.size() )
				{
					libmaus::gamma::SparseGammaGapMerge::SparseGammaGapMergeInfo::shared_ptr_type sptr(
						new libmaus::gamma::SparseGammaGapMerge::SparseGammaGapMergeInfo
					);
					*sptr = SGGMI;
					uint64_t const mergeid = nextmergeQid++;
					mergehandoutq[mergeid] = sptr;
					mergeallq[mergeid] = sptr;
				}			
			}
			
			void checkMerge()
			{
				bool finished = false;
				
				while ( ! finished )
				{
					finished = true;
				
					uint64_t l = 0;
					std::pair<libmaus::gamma::SparseGammaGapMultiFile,libmaus::gamma::SparseGammaGapMultiFile> P;
					uint64_t packetid = 0;
					uint64_t subid = 0;
					
					if ( needMerge(l,P) )
					{
						finished = false;		
						libmaus::parallel::ScopeLock slock(lock);
						queueMergeInfo(P.first,P.second);
					}
					else if ( checkMergePacket(packetid,subid) )
					{
						finished = false;
					
						SparseGammaGapMerge::SparseGammaGapMergeInfo * ptr = getMergeInfo(packetid);
						ptr->dispatch(subid);
						
						libmaus::parallel::ScopeLock slock(lock);
						if ( ptr->incrementFinished() )
						{
							libmaus::gamma::SparseGammaGapMultiFile const N(ptr->fno,l+1);
							L[l+1].push_back(N);
							
							assert ( mergeallq.find(packetid) != mergeallq.end() );
							assert ( mergehandoutq.find(packetid) == mergehandoutq.end() );
							assert ( mergefinishq.find(packetid) != mergefinishq.end() );

							mergeallq.erase(mergeallq.find(packetid));		
							mergefinishq.erase(mergefinishq.find(packetid));							
						}
					}
				}
			}

			void addFile(std::vector<std::string> const & fn)
			{
				for ( uint64_t i = 0; i < fn.size(); ++i )
					libmaus::util::TempFileRemovalContainer::addTempFile(fn[i]);
				
				SparseGammaGapMultiFile S(fn,0);
				
				{
					libmaus::parallel::ScopeLock slock(lock);
					addcnt += 1;
					L[0].push_back(S);
				}
				
				checkMerge();
			}
			
			void addFile(std::string const & fn)
			{
				addFile(std::vector<std::string>(1,fn));
			}

			std::vector<std::string> merge(std::string const & outputfilenameprefix)
			{
				// set up merge queue Q
				std::priority_queue<libmaus::gamma::SparseGammaGapMultiFile> Q;

				for ( std::map< uint64_t,std::deque<libmaus::gamma::SparseGammaGapMultiFile> >::iterator ita = L.begin();
					ita != L.end(); ++ita )
					for ( uint64_t i = 0; i < ita->second.size(); ++i )
						Q.push(ita->second[i]);
				
				// erase level data structure
				L.clear();

				// do merging
				while ( Q.size() > 1 )
				{
					std::pair<libmaus::gamma::SparseGammaGapMultiFile,libmaus::gamma::SparseGammaGapMultiFile> P;
					P.first = Q.top(); Q.pop();
					P.second = Q.top(); Q.pop();

					queueMergeInfo(P.first,P.second);
					
					#pragma omp parallel
					{
						uint64_t packetid = 0, subid = 0;
						
						while ( checkMergePacket(packetid,subid) )
						{
							SparseGammaGapMerge::SparseGammaGapMergeInfo * ptr = getMergeInfo(packetid);
							ptr->dispatch(subid);
						
							libmaus::parallel::ScopeLock slock(lock);

							if ( ptr->incrementFinished() )
							{
								libmaus::gamma::SparseGammaGapMultiFile const N(ptr->fno,P.second.level+1);
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
						libmaus::util::TempFileRemovalContainer::addTempFile(fn);
						rename(Q.top().fn[i].c_str(),fn.c_str());
						outputfilenames.push_back(fn);
					}
				}

				return outputfilenames;
			}

			/**
			 * merge sparse gamma coded gap files to a set of dense files
			 **/
			std::vector<std::string> mergeToDense(std::string const & outputfilenameprefix, uint64_t const n)
			{
				std::string const tmpfilename = tmpgen.getFileName();				
				std::vector<std::string> const fno = merge(tmpfilename);
				uint64_t const partsize = (n+parts-1)/parts;
				uint64_t const aparts = (n+partsize-1)/partsize;
				std::vector<std::string> outputfilenames(aparts);

				#pragma omp parallel for schedule(dynamic,1)
				for ( uint64_t p = 0; p < aparts; ++p )
				{
					std::ostringstream fnostr;
					fnostr << outputfilenameprefix << "_" << std::setw(6) << std::setfill('0') << p;
					std::string const fn = fnostr.str();
					outputfilenames[p] = fn;
				
					uint64_t const low = std::min(p * partsize,n);
					uint64_t const high = std::min(low+partsize,n);
				
					libmaus::gamma::SparseGammaGapConcatDecoder SGGD(fno,low);
					libmaus::gamma::SparseGammaGapConcatDecoder::iterator it = SGGD.begin();
					
					libmaus::gamma::GammaGapEncoder GGE(fn);
					GGE.encode(it,high-low);
				}

				for ( uint64_t i = 0; i < fno.size(); ++i )
					remove(fno[i].c_str());
					
				return outputfilenames;
			}
		};
	}
}
#endif
