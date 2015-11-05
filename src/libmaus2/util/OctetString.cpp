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
#include <libmaus2/util/OctetString.hpp>
#include <libmaus2/util/GetFileSize.hpp>

uint64_t libmaus2::util::OctetString::computeOctetLength(std::istream &, uint64_t const len)
{
	return len;
}

libmaus2::util::OctetString::shared_ptr_type libmaus2::util::OctetString::constructRaw(
	std::string const & filename,
	uint64_t const offset,
	uint64_t const blength
)
{
	return shared_ptr_type(new this_type(filename, offset, blength));
}

libmaus2::util::OctetString::OctetString(
	std::string const & filename,
	uint64_t offset,
	uint64_t blength)
{
	::libmaus2::aio::InputStreamInstance CIS(filename);
	uint64_t const fs = ::libmaus2::util::GetFileSize::getFileSize(CIS);
	offset = std::min(offset,fs);
	blength = std::min(blength,fs-offset);

	CIS.seekg(offset);
	A = ::libmaus2::autoarray::AutoArray<uint8_t>(blength,false);
	CIS.read(reinterpret_cast<char *>(A.begin()),blength);
}

libmaus2::util::OctetString::OctetString(std::istream & CIS, uint64_t blength)
: A(blength,false)
{
	CIS.read(reinterpret_cast<char *>(A.begin()),blength);
}


libmaus2::util::OctetString::OctetString(std::istream & CIS, uint64_t const octetlength, uint64_t const symlength)
: A(octetlength,false)
{
	assert ( octetlength == symlength );
	CIS.read(reinterpret_cast<char *>(A.begin()),symlength);
}

::libmaus2::util::Histogram::unique_ptr_type libmaus2::util::OctetString::getHistogram() const
{
	::libmaus2::util::Histogram::unique_ptr_type hist(new ::libmaus2::util::Histogram);

	for ( uint64_t i = 0; i < A.size(); ++i )
		(*hist)(A[i]);

	return UNIQUE_PTR_MOVE(hist);
}

std::map<int64_t,uint64_t> libmaus2::util::OctetString::getHistogramAsMap() const
{
	::libmaus2::util::Histogram::unique_ptr_type hist(getHistogram());
	return hist->getByType<int64_t>();
}

::libmaus2::autoarray::AutoArray<libmaus2::util::OctetString::saidx_t,::libmaus2::autoarray::alloc_type_c>
	libmaus2::util::OctetString::computeSuffixArray32(bool const parallel) const
{
	if ( A.size() > static_cast<uint64_t>(::std::numeric_limits<saidx_t>::max()) )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "computeSuffixArray32: input is too large for data type." << std::endl;
		se.finish();
		throw se;
	}

	::libmaus2::autoarray::AutoArray<saidx_t,::libmaus2::autoarray::alloc_type_c> SA(A.size());
	if ( parallel )
		sort_type_parallel::divsufsort ( A.begin() , SA.begin() , A.size() );
	else
		sort_type_serial::divsufsort ( A.begin() , SA.begin() , A.size() );

	return SA;
}
