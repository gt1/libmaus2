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

#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

void libmaus2::util::GetFileSize::copy(std::string const & from, std::string const & to)
{
	::libmaus2::aio::InputStreamInstance istr(from);
	::libmaus2::aio::OutputStreamInstance ostr(to);
	copy(istr,ostr,getFileSize(from));
	ostr.flush();
}

int libmaus2::util::GetFileSize::getSymbolAtPosition(std::string const & filename, uint64_t const pos)
{
	::libmaus2::aio::InputStreamInstance CIS(filename);
	CIS.seekg(pos,std::ios::beg);
	int const c = CIS.get();
	if ( c < 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed to get symbol at position " << pos << " of " << filename << std::endl;
		se.finish();
		throw se;
	}

	return c;
}

::libmaus2::autoarray::AutoArray<uint8_t> libmaus2::util::GetFileSize::readFile(std::string const & filename)
{
	::libmaus2::autoarray::AutoArray<uint8_t> A;

	if ( ! fileExists(filename) )
		return A;

	uint64_t const fs = getFileSize(filename);
	A = ::libmaus2::autoarray::AutoArray<uint8_t>(fs);

	libmaus2::aio::InputStreamInstance istr(filename);
	istr.read ( reinterpret_cast<char *>(A.get()), fs );
	assert ( istr );
	assert ( istr.gcount() == static_cast<int64_t>(fs) );

	return A;
}

bool libmaus2::util::GetFileSize::fileExists(std::string const & filename)
{
	return libmaus2::aio::InputStreamFactoryContainer::tryOpen(filename);
}

uint64_t libmaus2::util::GetFileSize::getFileSize(std::istream & istr)
{
	uint64_t const cur = istr.tellg();
	istr.seekg(0,std::ios::end);
	uint64_t const l = istr.tellg();
	istr.seekg(cur,std::ios::beg);
	istr.clear();
	return l;
}

uint64_t libmaus2::util::GetFileSize::getFileSize(std::wistream & istr)
{
	uint64_t const cur = istr.tellg();
	istr.seekg(0,std::ios::end);
	uint64_t const l = istr.tellg();
	istr.seekg(cur,std::ios::beg);
	istr.clear();
	return l;
}

uint64_t libmaus2::util::GetFileSize::getFileSize(std::string const & filename)
{
	libmaus2::aio::InputStreamInstance istr(filename);
	istr.seekg(0,std::ios::end);
	uint64_t const l = istr.tellg();
	return l;
}

uint64_t libmaus2::util::GetFileSize::getFileSize(std::vector<std::string> const & filenames)
{
	uint64_t s = 0;
	for ( uint64_t i = 0; i < filenames.size(); ++i )
		s += getFileSize(filenames[i]);
	return s;
}

uint64_t libmaus2::util::GetFileSize::getFileSize(std::vector< std::vector<std::string> > const & filenames)
{
	uint64_t s = 0;
	for ( uint64_t i = 0; i < filenames.size(); ++i )
		s += getFileSize(filenames[i]);
	return s;
}

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

bool libmaus2::util::GetFileSize::isOlder(std::string const & fn_A, std::string const & fn_B)
{
	struct stat stat_a;
	struct stat stat_b;

	int const ret_a = stat(fn_A.c_str(),&stat_a);

	if ( ret_a < 0 )
	{
		int const error = errno;
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "libmaus2::util::GetFileSize::isOlder: stat failed for file " << fn_A << ": " << strerror(error) << std::endl;
		lme.finish();
		throw lme;
	}

	int const ret_b = stat(fn_B.c_str(),&stat_b);

	if ( ret_b < 0 )
	{
		int const error = errno;
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "libmaus2::util::GetFileSize::isOlder: stat failed for file " << fn_B << ": " << strerror(error) << std::endl;
		lme.finish();
		throw lme;
	}

	return stat_a.st_mtime < stat_b.st_mtime;
}

#include <deque>
#include <sys/types.h>
#include <dirent.h>

enum file_type { file_type_dir, file_type_regular, file_type_other };

std::ostream & operator<<(std::ostream & out, file_type const & F)
{
	switch ( F )
	{
		case file_type_dir:
			out << "file_type_dir";
			break;
		case file_type_regular:
			out << "file_type_regular";
			break;
		default:
			out << "file_type_other";
			break;
	}

	return out;
}

static std::pair<file_type,uint64_t> getType(std::string const & fn)
{
	struct stat sb;
	if ( ::stat(fn.c_str(),&sb) < 0 )
	{
		return std::pair<file_type,uint64_t>(file_type_other,0);
	}

	if ( S_ISREG(sb.st_mode) )
		return std::pair<file_type,uint64_t>(file_type_regular,sb.st_size);
	else if ( S_ISDIR(sb.st_mode) )
		return std::pair<file_type,uint64_t>(file_type_dir,0);
	else
		return std::pair<file_type,uint64_t>(file_type_other,0);
}

struct DirHandleScope
{
	DIR * handle;

	DirHandleScope(std::string const & fn) : handle(::opendir(fn.c_str())) {}
	~DirHandleScope()
	{
		if ( handle )
		{
			::closedir(handle);
			handle = 0;
		}
	}

	struct dirent * getNext()
	{
		if ( handle )
			return readdir(handle);
		else
			return 0;
	}
};

uint64_t libmaus2::util::GetFileSize::getDirSize(std::string const & fn)
{
	std::deque<std::string> Q;
	Q.push_back(fn);
	uint64_t s = 0;

	while ( Q.size() )
	{
		std::string const D = Q.front();
		Q.pop_front();

		std::pair<file_type,uint64_t> type = getType(D);

		// std::cerr << D << " has type " << type.first << " size " << type.second << std::endl;

		if ( type.first == file_type_dir )
		{
			DirHandleScope DHS(D);

			struct dirent * entry;
			while ( (entry=DHS.getNext()) )
			{
				std::string const sname = entry->d_name;

				if ( sname != std::string(".") && sname != std::string("..") )
					Q.push_back(D + std::string("/") + sname);
			}
		}
		else if ( type.first == file_type_regular )
		{
			s += type.second;
		}
	}

	return s;
}
