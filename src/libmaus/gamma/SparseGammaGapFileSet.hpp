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
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPFILESET_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPFILESET_HPP

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
		struct SparseGammaGapFileSet
		{
			libmaus::util::TempFileNameGenerator & tmpgen;
			std::priority_queue<libmaus::gamma::SparseGammaGapFile> Q;
			libmaus::parallel::OMPLock lock;
			uint64_t addcnt;
		
			SparseGammaGapFileSet(libmaus::util::TempFileNameGenerator & rtmpgen) : tmpgen(rtmpgen), addcnt(0) {}
			
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
				
				libmaus::util::TempFileRemovalContainer::addTempFile(nfn);
				
				SparseGammaGapFile N(nfn,Sa.level+1);
				Q.push(N);

				libmaus::aio::CheckedInputStream ina(Sa.fn);
				libmaus::aio::CheckedInputStream inb(Sb.fn);
				libmaus::aio::CheckedOutputStream out(nfn);
				libmaus::gamma::SparseGammaGapMerge::merge(ina,inb,out);
				
				// remove input files
				remove(Sa.fn.c_str());
				remove(Sb.fn.c_str());
			}
			
			void addFile(std::string const & fn)
			{
				libmaus::util::TempFileRemovalContainer::addTempFile(fn);
				
				SparseGammaGapFile S(fn,0);
				
				libmaus::parallel::ScopeLock slock(lock);
				addcnt += 1;
				Q.push(S);
				
				while ( needMerge() )
					doMerge(tmpgen.getFileName());
			}
			
			void merge(std::string const & outputfilename)
			{
				libmaus::parallel::ScopeLock slock(lock);

				while ( canMerge() )
					doMerge(tmpgen.getFileName());
					
				if ( !Q.empty() )
					rename(Q.top().fn.c_str(),outputfilename.c_str());
			}

			void mergeToDense(std::string const & outputfilename, uint64_t const n)
			{
				libmaus::parallel::ScopeLock slock(lock);

				while ( canMerge() )
					doMerge(tmpgen.getFileName());
					
				if ( !Q.empty() )
				{
					libmaus::aio::CheckedInputStream CIS(Q.top().fn);
					libmaus::gamma::SparseGammaGapDecoder SGGD(CIS);
					libmaus::gamma::SparseGammaGapDecoder::iterator it = SGGD.begin();
					
					libmaus::gamma::GammaGapEncoder GGE(outputfilename);
					GGE.encode(it,n);
					
					remove(Q.top().fn.c_str());
				}
			}
		};
	}
}
#endif
