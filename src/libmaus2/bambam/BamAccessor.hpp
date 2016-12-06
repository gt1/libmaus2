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
#if ! defined(LIBMAUS2_BAMBAM_BAMACCESSOR_HPP)
#define LIBMAUS2_BAMBAM_BAMACCESSOR_HPP

#include <libmaus2/bambam/BamNumericalIndexDecoder.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamAccessor
		{
			typedef BamAccessor this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string const bamfn;
			libmaus2::bambam::BamNumericalIndexDecoder::unique_ptr_type Pbamindexdec;
			libmaus2::bambam::BamNumericalIndexDecoder & bamindexdec;
			int64_t rank;
			libmaus2::lz::BgzfInflateFile::unique_ptr_type file;
			libmaus2::bambam::BamAlignment algn;

			BamAccessor(std::string const & rbamfn, libmaus2::bambam::BamNumericalIndexDecoder & rbamindexdec, uint64_t const at)
			: bamfn(rbamfn), bamindexdec(rbamindexdec), rank(static_cast<int64_t>(at)-1), file(bamindexdec.getStreamAt(bamfn,at))
			{

			}

			BamAccessor(std::string const & rbamfn, uint64_t const at)
			: bamfn(rbamfn), Pbamindexdec(new libmaus2::bambam::BamNumericalIndexDecoder(libmaus2::bambam::BamNumericalIndexBase::getIndexName(bamfn))),
			  bamindexdec(*Pbamindexdec), rank(static_cast<int64_t>(at)-1), file(bamindexdec.getStreamAt(bamfn,at))
			{

			}

			libmaus2::bambam::BamAlignment & operator[](uint64_t const r)
			{
				while ( rank < static_cast<int64_t>(r) )
				{
					libmaus2::bambam::BamAlignmentDecoder::readAlignmentGz(*file,algn);
					rank += 1;
				}
				assert ( rank == static_cast<int64_t>(r) );
				return algn;
			}
		};
	}
}
#endif
