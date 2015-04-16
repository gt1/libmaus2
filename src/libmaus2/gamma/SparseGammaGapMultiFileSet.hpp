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
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPMULTIFILESET_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPMULTIFILESET_HPP

#include <libmaus2/gamma/SparseGammaGapDecoder.hpp>
#include <libmaus2/gamma/SparseGammaGapMultiFile.hpp>
#include <libmaus2/gamma/SparseGammaGapMerge.hpp>
#include <libmaus2/gamma/GammaGapEncoder.hpp>
#include <libmaus2/aio/CheckedInputStream.hpp>
#include <libmaus2/aio/CheckedOutputStream.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <queue>

namespace libmaus2
{
	namespace gamma
	{
		struct SparseGammaGapMultiFileSet
		{
			libmaus2::util::TempFileNameGenerator & tmpgen;
			std::priority_queue<libmaus2::gamma::SparseGammaGapMultiFile> Q;
			libmaus2::parallel::OMPLock lock;
			uint64_t addcnt;
			uint64_t parts;
		
			SparseGammaGapMultiFileSet(
				libmaus2::util::TempFileNameGenerator & rtmpgen,
				uint64_t rparts
			) : tmpgen(rtmpgen), addcnt(0), parts(rparts) {}

			bool needMerge()
			{
				if ( Q.empty() )
					return false;
				
				SparseGammaGapMultiFile const Sa = Q.top(); Q.pop();
				
				if ( Q.empty() )
				{
					Q.push(Sa);
					return false;
				}
				
				SparseGammaGapMultiFile const Sb = Q.top(); Q.pop();
				
				bool const needmerge = Sa.level == Sb.level;
				
				Q.push(Sb);
				Q.push(Sa);
				
				return needmerge;
			}

			bool canMerge()
			{
				if ( Q.empty() )
					return false;
				
				SparseGammaGapMultiFile const Sa = Q.top(); Q.pop();
				
				if ( Q.empty() )
				{
					Q.push(Sa);
					return false;
				}
				
				SparseGammaGapMultiFile const Sb = Q.top(); Q.pop();
				
				Q.push(Sb);
				Q.push(Sa);
				
				return true;
			}
			
			void doMerge(std::string const & nfn)
			{
				assert ( ! Q.empty() );
				SparseGammaGapMultiFile const Sa = Q.top(); Q.pop();
				assert ( ! Q.empty() );
				SparseGammaGapMultiFile const Sb = Q.top(); Q.pop();
				
				std::vector<std::string> const fno = libmaus2::gamma::SparseGammaGapMerge::merge(Sa.fn,Sb.fn,nfn,parts,true);
				SparseGammaGapMultiFile N(fno,Sa.level+1);
				Q.push(N);
				
				#if 0
				std::cerr << "merged " << Sa.fn << " and " << Sb.fn << " to " << nfn << std::endl;
				#endif
				
				// remove input files
				for ( uint64_t i = 0; i < Sa.fn.size(); ++i )
					remove(Sa.fn[i].c_str());
				for ( uint64_t i = 0; i < Sb.fn.size(); ++i )
					remove(Sb.fn[i].c_str());
			}
			
			void addFile(std::vector<std::string> const & fn)
			{
				for ( uint64_t i = 0; i < fn.size(); ++i )
					libmaus2::util::TempFileRemovalContainer::addTempFile(fn[i]);
				
				SparseGammaGapMultiFile S(fn,0);
				
				libmaus2::parallel::ScopeLock slock(lock);
				addcnt += 1;
				Q.push(S);
				
				while ( needMerge() )
					doMerge(tmpgen.getFileName());
			}

			void addFile(std::string const & fn)
			{
				addFile(std::vector<std::string>(1,fn));
			}
			
			std::vector<std::string> merge(std::string const & outputfilenameprefix)
			{
				libmaus2::parallel::ScopeLock slock(lock);

				while ( canMerge() )
					doMerge(tmpgen.getFileName());
				
				std::vector<std::string> fno;	
				
				if ( !Q.empty() )
				{
					SparseGammaGapMultiFile const S = Q.top();
					
					for ( uint64_t i = 0; i < S.fn.size(); ++i )
					{
						std::ostringstream ostr;
						ostr << outputfilenameprefix << "_" << std::setw(6) << std::setfill('0') << i;
						std::string const fn = ostr.str();
						rename(S.fn[i].c_str(),fn.c_str());
						fno.push_back(fn);
					}
				}
				
				return fno;
			}

			void mergeToDense(std::string const & outputfilename, uint64_t const n)
			{
				libmaus2::parallel::ScopeLock slock(lock);

				while ( canMerge() )
					doMerge(tmpgen.getFileName());
					
				if ( !Q.empty() )
				{
					libmaus2::gamma::SparseGammaGapConcatDecoder SGGD(Q.top().fn);
					libmaus2::gamma::SparseGammaGapConcatDecoder::iterator it = SGGD.begin();
					
					libmaus2::gamma::GammaGapEncoder GGE(outputfilename);
					GGE.encode(it,n);
				
					for ( uint64_t i = 0; i < Q.top().fn.size(); ++i )
						remove(Q.top().fn[i].c_str());
				}
			}
		};
	}
}
#endif
