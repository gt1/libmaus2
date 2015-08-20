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
#if ! defined(LIBMAUS2_GAMMA_SPARSEGAMMAGAPFILESET_HPP)
#define LIBMAUS2_GAMMA_SPARSEGAMMAGAPFILESET_HPP

#include <libmaus2/gamma/SparseGammaGapDecoder.hpp>
#include <libmaus2/gamma/SparseGammaGapFile.hpp>
#include <libmaus2/gamma/SparseGammaGapMerge.hpp>
#include <libmaus2/gamma/GammaGapEncoder.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <queue>

namespace libmaus2
{
	namespace gamma
	{
		struct SparseGammaGapFileSet
		{
			libmaus2::util::TempFileNameGenerator & tmpgen;
			std::priority_queue<libmaus2::gamma::SparseGammaGapFile> Q;
			libmaus2::parallel::OMPLock lock;
			uint64_t addcnt;
		
			SparseGammaGapFileSet(libmaus2::util::TempFileNameGenerator & rtmpgen) : tmpgen(rtmpgen), addcnt(0) {}
			
			bool needMerge()
			{
				if ( Q.empty() )
					return false;
				
				SparseGammaGapFile const Sa = Q.top(); Q.pop();
				
				if ( Q.empty() )
				{
					Q.push(Sa);
					return false;
				}
				
				SparseGammaGapFile const Sb = Q.top(); Q.pop();
				
				bool const needmerge = Sa.level == Sb.level;
				
				Q.push(Sb);
				Q.push(Sa);
				
				return needmerge;
			}

			bool canMerge()
			{
				if ( Q.empty() )
					return false;
				
				SparseGammaGapFile const Sa = Q.top(); Q.pop();
				
				if ( Q.empty() )
				{
					Q.push(Sa);
					return false;
				}
				
				SparseGammaGapFile const Sb = Q.top(); Q.pop();
				
				Q.push(Sb);
				Q.push(Sa);
				
				return true;
			}
			
			void doMerge(std::string const & nfn)
			{
				assert ( ! Q.empty() );
				SparseGammaGapFile const Sa = Q.top(); Q.pop();
				assert ( ! Q.empty() );
				SparseGammaGapFile const Sb = Q.top(); Q.pop();
				// assert ( Sa.level == Sb.level );
				
				libmaus2::util::TempFileRemovalContainer::addTempFile(nfn);
				
				SparseGammaGapFile N(nfn,Sa.level+1);
				Q.push(N);

				libmaus2::aio::InputStreamInstance ina(Sa.fn);
				libmaus2::aio::InputStreamInstance inb(Sb.fn);
				libmaus2::aio::OutputStreamInstance out(nfn);
				libmaus2::gamma::SparseGammaGapMerge::merge(ina,inb,out);
				
				#if 0
				std::cerr << "merged " << Sa.fn << " and " << Sb.fn << " to " << nfn << std::endl;
				#endif
				
				// remove input files
				libmaus2::aio::FileRemoval::removeFile(Sa.fn);
				libmaus2::aio::FileRemoval::removeFile(Sb.fn);
			}
			
			void addFile(std::string const & fn)
			{
				libmaus2::util::TempFileRemovalContainer::addTempFile(fn);
				
				SparseGammaGapFile S(fn,0);
				
				libmaus2::parallel::ScopeLock slock(lock);
				addcnt += 1;
				Q.push(S);
				
				while ( needMerge() )
					doMerge(tmpgen.getFileName());
			}
			
			void merge(std::string const & outputfilename)
			{
				libmaus2::parallel::ScopeLock slock(lock);

				while ( canMerge() )
					doMerge(tmpgen.getFileName());
					
				if ( !Q.empty() )
					libmaus2::aio::OutputStreamFactoryContainer::rename(Q.top().fn.c_str(),outputfilename.c_str());
			}

			void mergeToDense(std::string const & outputfilename, uint64_t const n)
			{
				libmaus2::parallel::ScopeLock slock(lock);

				while ( canMerge() )
					doMerge(tmpgen.getFileName());
					
				if ( !Q.empty() )
				{
					libmaus2::aio::InputStreamInstance CIS(Q.top().fn);
					libmaus2::gamma::SparseGammaGapDecoder SGGD(CIS);
					libmaus2::gamma::SparseGammaGapDecoder::iterator it = SGGD.begin();
					
					libmaus2::gamma::GammaGapEncoder GGE(outputfilename);
					GGE.encode(it,n);
					
					libmaus2::aio::FileRemoval::removeFile(Q.top().fn);
				}
			}
		};
	}
}
#endif
