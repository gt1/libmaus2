/**
    libmaus2
    Copyright (C) 2009-2016 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
**/
#include <libmaus2/suffixsort/bwtb3m/BwtMergeSort.hpp>

libmaus2::suffixsort::bwtb3m::BwtMergeSortResult libmaus2::suffixsort::bwtb3m::BwtMergeSort::computeBwt(BwtMergeSortOptions const & options, std::ostream * logstr)
{
	if ( options.inputtype == BwtMergeSortOptions::bwt_merge_input_type_utf_8 )
	{
		// compute index of file for random access, if it does not already exist
		std::string const fn = options.fn;
		std::string const idxfn = fn + ".idx";
		if (
			(! ::libmaus2::util::GetFileSize::fileExists(idxfn))
			||
			::libmaus2::util::GetFileSize::isOlder(fn,idxfn)
		)
		{
			::libmaus2::util::Utf8BlockIndex::unique_ptr_type index(::libmaus2::util::Utf8BlockIndex::constructFromUtf8File(fn,16*1024 /* block size */,options.numthreads));
			::libmaus2::aio::OutputStreamInstance COS(idxfn);
			index->serialise(COS);
			COS.flush();
		}

	}

	switch ( options.inputtype )
	{
		case BwtMergeSortOptions::bwt_merge_input_type_bytestream:
			return BwtMergeSortTemplate<libmaus2::suffixsort::ByteInputTypes>::computeBwt(options,logstr);
		case BwtMergeSortOptions::bwt_merge_input_type_compactstream:
			return BwtMergeSortTemplate<libmaus2::suffixsort::CompactInputTypes>::computeBwt(options,logstr);
		case BwtMergeSortOptions::bwt_merge_input_type_pac:
			return BwtMergeSortTemplate<libmaus2::suffixsort::PacInputTypes>::computeBwt(options,logstr);
		case BwtMergeSortOptions::bwt_merge_input_type_pacterm:
			return BwtMergeSortTemplate<libmaus2::suffixsort::PacTermInputTypes>::computeBwt(options,logstr);
		case BwtMergeSortOptions::bwt_merge_input_type_lz4:
			return BwtMergeSortTemplate<libmaus2::suffixsort::Lz4InputTypes>::computeBwt(options,logstr);
		case BwtMergeSortOptions::bwt_merge_input_type_utf_8:
			return BwtMergeSortTemplate<libmaus2::suffixsort::Utf8InputTypes>::computeBwt(options,logstr);
		default:
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "libmaus2::suffixsort::bwtb3m::BwtMergeSort::computeBwt: unknown/unsupported input type" << std::endl;
			lme.finish();
			throw lme;
		}
			break;
	}
}
