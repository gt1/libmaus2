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
#if ! defined(LIBMAUS2_GAMMA_SPARSEGAMMAGAPFILELEVELSET_HPP)
#define LIBMAUS2_GAMMA_SPARSEGAMMAGAPFILELEVELSET_HPP

#include <libmaus2/gamma/SparseGammaGapDecoder.hpp>
#include <libmaus2/gamma/SparseGammaGapFile.hpp>
#include <libmaus2/gamma/SparseGammaGapMerge.hpp>
#include <libmaus2/gamma/GammaGapEncoder.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <queue>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type>
		struct SparseGammaGapFileLevelSetTemplate
		{
			typedef _data_type data_type;
			typedef SparseGammaGapFileLevelSetTemplate<data_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::util::TempFileNameGenerator & tmpgen;
			std::map< uint64_t,std::deque<libmaus2::gamma::SparseGammaGapFile> > L;
			libmaus2::parallel::OMPLock lock;
			uint64_t addcnt;

			SparseGammaGapFileLevelSetTemplate(libmaus2::util::TempFileNameGenerator & rtmpgen) : tmpgen(rtmpgen), addcnt(0) {}

			bool needMerge(uint64_t & l, std::pair<libmaus2::gamma::SparseGammaGapFile,libmaus2::gamma::SparseGammaGapFile> & P)
			{
				libmaus2::parallel::ScopeLock slock(lock);

				for ( std::map< uint64_t,std::deque<libmaus2::gamma::SparseGammaGapFile> >::iterator ita = L.begin();
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
				std::pair<libmaus2::gamma::SparseGammaGapFile,libmaus2::gamma::SparseGammaGapFile> const & P,
				std::string const & nfn
			)
			{
				SparseGammaGapFile const Sa = P.first;
				SparseGammaGapFile const Sb = P.second;

				libmaus2::util::TempFileRemovalContainer::addTempFile(nfn);

				SparseGammaGapFile N(nfn,l+1);

				libmaus2::aio::InputStreamInstance ina(Sa.fn);
				libmaus2::aio::InputStreamInstance inb(Sb.fn);
				libmaus2::aio::OutputStreamInstance out(nfn);
				libmaus2::gamma::SparseGammaGapMergeTemplate<data_type>::merge(ina,inb,out);

				// remove input files
				libmaus2::aio::FileRemoval::removeFile(Sa.fn);
				libmaus2::aio::FileRemoval::removeFile(Sb.fn);

				#if 0
				std::cerr << "merged " << Sa << " and " << Sb << " to " << N << std::endl;
				#endif

				return N;
			}

			void addFile(std::string const & fn)
			{
				libmaus2::util::TempFileRemovalContainer::addTempFile(fn);

				SparseGammaGapFile S(fn,0);

				{
					libmaus2::parallel::ScopeLock slock(lock);
					addcnt += 1;
					L[0].push_back(S);
				}

				uint64_t l = 0;
				std::pair<libmaus2::gamma::SparseGammaGapFile,libmaus2::gamma::SparseGammaGapFile> P;
				while ( needMerge(l,P) )
				{
					libmaus2::gamma::SparseGammaGapFile const N = doMerge(l,P,tmpgen.getFileName());

					libmaus2::parallel::ScopeLock slock(lock);
					L[l+1].push_back(N);
				}
			}

			bool merge(std::string const & outputfilename)
			{
				libmaus2::parallel::ScopeLock slock(lock);

				// set up merge queue Q
				std::priority_queue<libmaus2::gamma::SparseGammaGapFile> Q;

				for ( std::map< uint64_t,std::deque<libmaus2::gamma::SparseGammaGapFile> >::iterator ita = L.begin();
					ita != L.end(); ++ita )
					for ( uint64_t i = 0; i < ita->second.size(); ++i )
						Q.push(ita->second[i]);

				// erase level data structure
				L.clear();

				// do merging
				while ( Q.size() > 1 )
				{
					std::pair<libmaus2::gamma::SparseGammaGapFile,libmaus2::gamma::SparseGammaGapFile> P;
					P.first = Q.top(); Q.pop();
					P.second = Q.top(); Q.pop();

					libmaus2::gamma::SparseGammaGapFile N = doMerge(P.second.level,P,tmpgen.getFileName());

					Q.push(N);
				}

				if ( !Q.empty() )
				{
					libmaus2::aio::OutputStreamFactoryContainer::rename(Q.top().fn.c_str(),outputfilename.c_str());
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
				libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfilename);

				if ( merge(tmpfilename) )
				{
					libmaus2::aio::InputStreamInstance CIS(tmpfilename);
					libmaus2::gamma::SparseGammaGapDecoderTemplate<data_type> SGGD(CIS);
					typename libmaus2::gamma::SparseGammaGapDecoderTemplate<data_type>::iterator it = SGGD.begin();

					libmaus2::gamma::GammaGapEncoder GGE(outputfilename);
					GGE.encode(it,n);

					libmaus2::aio::FileRemoval::removeFile(tmpfilename);
				}
			}
		};

		typedef SparseGammaGapFileLevelSetTemplate<uint64_t> SparseGammaGapFileLevelSet;
		typedef SparseGammaGapFileLevelSetTemplate< libmaus2::math::UnsignedInteger<4> > SparseGammaGapFileLevelSet2;
	}
}
#endif
