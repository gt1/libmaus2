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
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPFILELEVELSET_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPFILELEVELSET_HPP

#include <libmaus/gamma/SparseGammaGapDecoder.hpp>
#include <libmaus/gamma/SparseGammaGapFile.hpp>
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
		struct SparseGammaGapFileLevelSet
		{
			libmaus::util::TempFileNameGenerator & tmpgen;
			std::map< uint64_t,std::deque<libmaus::gamma::SparseGammaGapFile> > L;
			libmaus::parallel::OMPLock lock;
			uint64_t addcnt;
		
			SparseGammaGapFileLevelSet(libmaus::util::TempFileNameGenerator & rtmpgen) : tmpgen(rtmpgen), addcnt(0) {}
			
			bool needMerge(uint64_t & l, std::pair<libmaus::gamma::SparseGammaGapFile,libmaus::gamma::SparseGammaGapFile> & P)
			{
				libmaus::parallel::ScopeLock slock(lock);
				
				for ( std::map< uint64_t,std::deque<libmaus::gamma::SparseGammaGapFile> >::iterator ita = L.begin();
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

			SparseGammaGapFile doMerge(
				uint64_t const l, 
				std::pair<libmaus::gamma::SparseGammaGapFile,libmaus::gamma::SparseGammaGapFile> const & P,
				std::string const & nfn
			)
			{
				SparseGammaGapFile const Sa = P.first;
				SparseGammaGapFile const Sb = P.second;
				
				libmaus::util::TempFileRemovalContainer::addTempFile(nfn);
				
				SparseGammaGapFile N(nfn,l+1);

				libmaus::aio::CheckedInputStream ina(Sa.fn);
				libmaus::aio::CheckedInputStream inb(Sb.fn);
				libmaus::aio::CheckedOutputStream out(nfn);
				libmaus::gamma::SparseGammaGapMerge::merge(ina,inb,out);
				
				// remove input files
				remove(Sa.fn.c_str());
				remove(Sb.fn.c_str());

				#if 1
				std::cerr << "merged " << Sa << " and " << Sb << " to " << N << std::endl;
				#endif

				return N;
			}

			void addFile(std::string const & fn)
			{
				libmaus::util::TempFileRemovalContainer::addTempFile(fn);
				
				SparseGammaGapFile S(fn,0);
				
				{
					libmaus::parallel::ScopeLock slock(lock);
					addcnt += 1;
					L[0].push_back(S);
				}
				
				uint64_t l = 0;
				std::pair<libmaus::gamma::SparseGammaGapFile,libmaus::gamma::SparseGammaGapFile> P;
				while ( needMerge(l,P) )
				{
					libmaus::gamma::SparseGammaGapFile const N = doMerge(l,P,tmpgen.getFileName());

					libmaus::parallel::ScopeLock slock(lock);
					L[l+1].push_back(N);
				}
			}
			
			bool merge(std::string const & outputfilename)
			{
				libmaus::parallel::ScopeLock slock(lock);

				// set up merge queue Q
				std::priority_queue<libmaus::gamma::SparseGammaGapFile> Q;

				for ( std::map< uint64_t,std::deque<libmaus::gamma::SparseGammaGapFile> >::iterator ita = L.begin();
					ita != L.end(); ++ita )
					for ( uint64_t i = 0; i < ita->second.size(); ++i )
						Q.push(ita->second[i]);
				
				// erase level data structure
				L.clear();

				// do merging
				while ( Q.size() > 1 )
				{
					std::pair<libmaus::gamma::SparseGammaGapFile,libmaus::gamma::SparseGammaGapFile> P;
					P.first = Q.top(); Q.pop();
					P.second = Q.top(); Q.pop();

					libmaus::gamma::SparseGammaGapFile N = doMerge(P.second.level,P,tmpgen.getFileName());
					
					Q.push(N);
				}
				
				if ( !Q.empty() )
				{
					rename(Q.top().fn.c_str(),outputfilename.c_str());
					return true;
				}
				else
				{
					return false;
				}
			}

			void mergeToDense(std::string const & outputfilename, uint64_t const n)
			{
				std::string const tmpfilename = tmpgen.getFileName();				
				libmaus::util::TempFileRemovalContainer::addTempFile(tmpfilename);

				if ( merge(tmpfilename) )
				{
					libmaus::aio::CheckedInputStream CIS(tmpfilename);
					libmaus::gamma::SparseGammaGapDecoder SGGD(CIS);
					libmaus::gamma::SparseGammaGapDecoder::iterator it = SGGD.begin();
					
					libmaus::gamma::GammaGapEncoder GGE(outputfilename);
					GGE.encode(it,n);

					remove(tmpfilename.c_str());
				}
			}
		};
	}
}
#endif
