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

#if ! defined(LIBMAUS2_AIO_SINGLEFILEFRAGMENTMERGE_HPP)
#define LIBMAUS2_AIO_SINGLEFILEFRAGMENTMERGE_HPP

#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <queue>
#include <vector>

namespace libmaus2
{
	namespace aio
	{
		template<typename _element_type>
		struct SingleFileFragmentMerge
		{
			typedef _element_type element_type;
			typedef SingleFileFragmentMerge<element_type> this_type;

			template<typename _type>
			struct SingleFileFragmentMergeHeapComparator
			{
				typedef _type type;

				bool operator()(
					std::pair<uint64_t,type>  const & A,
					std::pair<uint64_t,type>  const & B)
				{
					bool const A_lt_B = A.second < B.second;
					bool const B_lt_A = B.second < A.second;

					// if A.second != B.second
					if ( A_lt_B || B_lt_A )
						return !(A_lt_B);
					else
						return A.first > B.first;
				}
			};


			std::string const & fn;
			libmaus2::aio::InputStream::unique_ptr_type pCIS;
			std::vector< std::pair<uint64_t,uint64_t> > frags;

			std::priority_queue<
				std::pair< uint64_t, element_type >,
				std::vector< std::pair< uint64_t, element_type > >,
				SingleFileFragmentMergeHeapComparator<element_type>
			> Q;

			SingleFileFragmentMerge(std::string const & rfn, std::vector< std::pair<uint64_t,uint64_t> > const & rfrags)
			: fn(rfn), pCIS(libmaus2::aio::InputStreamFactoryContainer::constructUnique(fn)), frags(rfrags)
			{
				for ( uint64_t i = 0; i < frags.size(); ++i )
					if ( frags[i].first != frags[i].second )
					{
						pCIS->seekg(frags[i].first);
						element_type BC;
						pCIS->read(reinterpret_cast<char *>(&BC), sizeof(element_type));
						frags[i].first += sizeof(element_type);
						Q.push(std::pair< uint64_t, element_type >(i,BC));
					}
			}

			bool getNext(element_type & BC)
			{
				if ( ! Q.size() )
					return false;

				std::pair< uint64_t, element_type > const P = Q.top();
				BC = P.second;
				Q.pop();

				uint64_t const fragindex = P.first;
				if ( frags[fragindex].first < frags[fragindex].second )
				{
					pCIS->seekg(frags[fragindex].first);
					element_type NBC;

					pCIS->read(reinterpret_cast<char *>(&NBC), sizeof(element_type));
					Q.push(std::pair< uint64_t, element_type >(fragindex,NBC));

					frags[fragindex].first += sizeof(element_type);
				}

				return true;
			}

			//! perform merge and return name of merged file name
			static std::string merge(
				std::string const & infn, std::vector< std::pair<uint64_t,uint64_t> > const & frags
			)
			{
				this_type BCM(infn,frags);
				std::string const nfile = infn + ".tmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(nfile);
				libmaus2::aio::OutputStream::unique_ptr_type pCOS(
					libmaus2::aio::OutputStreamFactoryContainer::constructUnique(nfile)
				);
				element_type BC;
				while ( BCM.getNext(BC) )
					pCOS->write(reinterpret_cast<char const *>(&BC),sizeof(element_type));
				pCOS->flush();
				pCOS.reset();

				return nfile;
			}
		};
	}
}
#endif
