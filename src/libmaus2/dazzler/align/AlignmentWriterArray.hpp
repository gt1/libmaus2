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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTWRITERARRAY_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTWRITERARRAY_HPP

#include <libmaus2/dazzler/align/AlignmentWriter.hpp>
#include <libmaus2/dazzler/align/SortingOverlapOutputBuffer.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct AlignmentWriterArray
			{
				typedef AlignmentWriterArray this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::vector<std::string> Vfn;
				libmaus2::autoarray::AutoArray<libmaus2::dazzler::align::AlignmentWriter::unique_ptr_type> A;

				AlignmentWriterArray(std::string const basefn, uint64_t const n, int64_t const tspace)
				: Vfn(n), A(n)
				{
					for ( uint64_t i = 0; i < n; ++i )
					{
						std::ostringstream ostr;
						ostr << basefn << "_" << std::setw(6) << std::setfill('0') << i;
						Vfn[i] = ostr.str();
						libmaus2::util::TempFileRemovalContainer::addTempFile(Vfn[i]);
						libmaus2::util::TempFileRemovalContainer::addTempFile(libmaus2::dazzler::align::OverlapIndexer::getIndexName(Vfn[i]));
						libmaus2::dazzler::align::AlignmentWriter::unique_ptr_type tptr(
							new libmaus2::dazzler::align::AlignmentWriter(Vfn[i],tspace)
						);
						A[i] = UNIQUE_PTR_MOVE(tptr);
					}
				}

				libmaus2::dazzler::align::AlignmentWriter & operator[](uint64_t const i)
				{
					return *A[i];
				}

				void merge(std::string const & fn, std::string const & tmp)
				{
					for ( uint64_t i = 0; i < A.size(); ++i )
						A[i].reset();

					// std::cerr << "Calling sort and merge..." << std::endl;

					libmaus2::dazzler::align::SortingOverlapOutputBuffer<>::sortAndMerge(Vfn,fn,tmp);

					for ( uint64_t i = 0; i < Vfn.size(); ++i )
						libmaus2::dazzler::align::SortingOverlapOutputBuffer<>::removeFileAndIndex(Vfn[i]);
				}
			};
		}
	}
}
#endif
