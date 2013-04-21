/**
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
**/

#include <libmaus/util/GetFileSize.hpp>

void libmaus::util::GetFileSize::copy(std::string const & from, std::string const & to)
{
	::libmaus::aio::CheckedInputStream istr(from);
	::libmaus::aio::CheckedOutputStream ostr(to);
	copy(istr,ostr,getFileSize(from));
	ostr.flush();
	ostr.close();
}

int libmaus::util::GetFileSize::getSymbolAtPosition(std::string const & filename, uint64_t const pos)
{
	::libmaus::aio::CheckedInputStream CIS(filename);
	CIS.seekg(pos,std::ios::beg);
	int const c = CIS.get();
	if ( c < 0 )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "Failed to get symbol at position " << pos << " of " << filename << std::endl;
		se.finish();
		throw se;
	}
	
	return c;
}

::libmaus::autoarray::AutoArray<uint8_t> libmaus::util::GetFileSize::readFile(std::string const & filename)
{
	::libmaus::autoarray::AutoArray<uint8_t> A;
	
	if ( ! fileExists(filename) )
		return A;
	
	uint64_t const fs = getFileSize(filename);
	A = ::libmaus::autoarray::AutoArray<uint8_t>(fs);
	
	::std::ifstream istr(filename.c_str(), std::ios::binary);
	assert ( istr.is_open() );
	istr.read ( reinterpret_cast<char *>(A.get()), fs );
	assert ( istr );
	assert ( istr.gcount() == static_cast<int64_t>(fs) );
	istr.close();
	
	return A;
}

bool libmaus::util::GetFileSize::fileExists(std::string const & filename)
{
	std::ifstream istr(filename.c_str(), std::ios::binary);
	if ( istr.is_open() )
	{
		istr.close();
		return true;
	}
	else
	{
		return false;
	}
}

uint64_t libmaus::util::GetFileSize::getFileSize(std::istream & istr)
{
	uint64_t const cur = istr.tellg();
	istr.seekg(0,std::ios::end);
	uint64_t const l = istr.tellg();
	istr.seekg(cur,std::ios::beg);
	istr.clear();
	return l;
}

uint64_t libmaus::util::GetFileSize::getFileSize(std::wistream & istr)
{
	uint64_t const cur = istr.tellg();
	istr.seekg(0,std::ios::end);
	uint64_t const l = istr.tellg();
	istr.seekg(cur,std::ios::beg);
	istr.clear();
	return l;
}

uint64_t libmaus::util::GetFileSize::getFileSize(std::string const & filename)
{
	std::ifstream istr(filename.c_str(),std::ios::binary);
	istr.seekg(0,std::ios::end);
	uint64_t const l = istr.tellg();
	istr.close();
	return l;
}

uint64_t libmaus::util::GetFileSize::getFileSize(std::vector<std::string> const & filenames)
{
	uint64_t s = 0;
	for ( uint64_t i = 0; i < filenames.size(); ++i )
		s += getFileSize(filenames[i]);
	return s;
}

uint64_t libmaus::util::GetFileSize::getFileSize(std::vector< std::vector<std::string> > const & filenames)
{
	uint64_t s = 0;
	for ( uint64_t i = 0; i < filenames.size(); ++i )
		s += getFileSize(filenames[i]);
	return s;
}
