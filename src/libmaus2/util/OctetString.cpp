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
#include <libmaus2/aio/StreamLock.hpp>

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
	uint64_t blength,
	int rverbose
) : verbose(rverbose)
{
	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] loading OctectString from " << filename << " offset " << offset << " blength=" << blength << std::endl;
	}
	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString opening: " << filename << " offset " << offset << " blength=" << blength << std::endl;
	}
	::libmaus2::aio::InputStreamInstance CIS(filename);
	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString getFileSize: " << filename << " offset " << offset << " blength=" << blength << std::endl;
	}
	uint64_t const fs = ::libmaus2::util::GetFileSize::getFileSize(CIS);
	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString got file size: " << filename << " offset " << offset << " blength=" << blength << fs << std::endl;
	}
	offset = std::min(offset,fs);
	blength = std::min(blength,fs-offset);

	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString updated: " << filename << " offset " << offset << " blength=" << blength << fs << std::endl;
	}

	CIS.seekg(offset);

	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString seeked: " << filename << " offset " << offset << " blength=" << blength << fs << std::endl;
	}

	A = A_type(blength,false);

	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString allocated: " << filename << " offset " << offset << " blength=" << blength << fs << std::endl;
	}

	CIS.read(reinterpret_cast<char *>(A.begin()),blength);

	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString read: " << filename << " offset " << offset << " blength=" << blength << fs << std::endl;
	}
}

libmaus2::util::OctetString::OctetString(std::istream & CIS, uint64_t blength, int const rverbose)
: A(blength,false), verbose(rverbose)
{
	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString reading stream blength=" << blength << std::endl;
	}
	CIS.read(reinterpret_cast<char *>(A.begin()),blength);
	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString reading stream blength=" << blength << " finished" << std::endl;
	}
}


libmaus2::util::OctetString::OctetString(std::istream & CIS, uint64_t const octetlength, uint64_t const symlength, int const rverbose)
: A(octetlength,false), verbose(rverbose)
{
	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString reading stream octetlength=" << octetlength << " symlength=" << symlength << std::endl;
	}
	assert ( octetlength == symlength );
	CIS.read(reinterpret_cast<char *>(A.begin()),symlength);
	if ( verbose >= 5 )
	{
		libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
		std::cerr << "[V] OctectString reading stream octetlength=" << octetlength << " symlength=" << symlength << " finished" << std::endl;
	}
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

::libmaus2::autoarray::AutoArray<libmaus2::util::OctetString::saidx_t,static_cast<libmaus2::autoarray::alloc_type>(libmaus2::util::StringAllocTypes::sa_atype)>
	libmaus2::util::OctetString::computeSuffixArray32(bool const parallel) const
{
	if ( A.size() > static_cast<uint64_t>(::std::numeric_limits<saidx_t>::max()) )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "computeSuffixArray32: input is too large for data type." << std::endl;
		se.finish();
		throw se;
	}

	::libmaus2::autoarray::AutoArray<saidx_t,static_cast<libmaus2::autoarray::alloc_type>(libmaus2::util::StringAllocTypes::sa_atype)> SA(A.size());
	if ( parallel )
		sort_type_parallel::divsufsort ( A.begin() , SA.begin() , A.size() );
	else
		sort_type_serial::divsufsort ( A.begin() , SA.begin() , A.size() );

	return SA;
}
