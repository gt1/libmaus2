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
		
			SparseGammaGapMultiFileLevelSet(
				libmaus::util::TempFileNameGenerator & rtmpgen,
				uint64_t const rparts
			) : tmpgen(rtmpgen), addcnt(0), parts(rparts) {}
			
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
				
				uint64_t l = 0;
				std::pair<libmaus::gamma::SparseGammaGapMultiFile,libmaus::gamma::SparseGammaGapMultiFile> P;
				while ( needMerge(l,P) )
				{
					libmaus::gamma::SparseGammaGapMultiFile const N = doMerge(l,P,tmpgen.getFileName());

					libmaus::parallel::ScopeLock slock(lock);
					L[l+1].push_back(N);
				}
			}
			
			void addFile(std::string const & fn)
			{
				addFile(std::vector<std::string>(1,fn));
			}
			
			std::vector<std::string> merge(std::string const & outputfilenameprefix)
			{
				libmaus::parallel::ScopeLock slock(lock);

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

					libmaus::gamma::SparseGammaGapMultiFile N = doMerge(P.second.level,P,tmpgen.getFileName());
					
					Q.push(N);
				}
				
				std::vector<std::string> outputfilenames;
				
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

			void mergeToDense(std::string const & outputfilename, uint64_t const n)
			{
				std::string const tmpfilename = tmpgen.getFileName();				
				std::vector<std::string> const fno = merge(tmpfilename);

				libmaus::gamma::SparseGammaGapConcatDecoder SGGD(fno);
				libmaus::gamma::SparseGammaGapConcatDecoder::iterator it = SGGD.begin();
					
				libmaus::gamma::GammaGapEncoder GGE(outputfilename);
				GGE.encode(it,n);

				for ( uint64_t i = 0; i < fno.size(); ++i )
					remove(fno[i].c_str());
			}
		};
	}
}
#endif
