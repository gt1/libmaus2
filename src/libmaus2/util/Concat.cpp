/*
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
*/

#include <libmaus/util/Concat.hpp>

uint64_t libmaus::util::Concat::concat(std::istream & in, std::ostream & out)
{
	::libmaus::autoarray::AutoArray < char > buf(16*1024,false);
	uint64_t c = 0;
	
	while ( in )
	{
		in.read(buf.get(), buf.getN());
		out.write ( buf.get(), in.gcount() );
		c += in.gcount();
	}
	
	return c;
}

uint64_t libmaus::util::Concat::concat(std::string const & filename, std::ostream & out)
{
	uint64_t n = ::libmaus::util::GetFileSize::getFileSize(filename);
	::libmaus::autoarray::AutoArray < char > buf(16*1024,false);
	libmaus::aio::CheckedInputStream in(filename);
	uint64_t c = 0;
	
	while ( n )
	{
		uint64_t const toread = std::min(n,buf.getN());
		in.read(buf.get(), toread);
		assert ( static_cast<int64_t>(toread) == static_cast<int64_t>(in.gcount()) );
		out.write ( buf.get(), toread );
		n -= toread;
		c += toread;
	}
	
	in.close();
	
	return c;
}

uint64_t libmaus::util::Concat::concat(std::vector < std::string > const & files, std::ostream & out, bool const rem)
{
	uint64_t c = 0;
	
	for ( uint64_t i = 0; i < files.size(); ++i )
	{
		c += concat ( files[i] , out );
		if ( rem )
			::remove ( files[i].c_str() );
	}
	
	return c;
}

uint64_t libmaus::util::Concat::concatParallel(
	std::vector < std::string > const & files, 
	std::string const & outputfilename, 
	bool const rem
)
{
	::libmaus::autoarray::AutoArray<uint64_t> P(files.size()+1);
	for ( uint64_t i = 0; i < files.size(); ++i )
		P[i] = ::libmaus::util::GetFileSize::getFileSize(files[i]);
	P.prefixSums();

	std::ofstream ostr(outputfilename.c_str(),std::ios::binary);
	ostr.flush();
	ostr.close();

	#if defined(_OPENMP)
	#pragma omp parallel for schedule(dynamic,1)
	#endif			
	for ( int64_t i = 0; i < static_cast<int64_t>(files.size()); ++i )
	{
		std::fstream out(outputfilename.c_str(), std::ios::in|std::ios::out|std::ios::binary|std::ios::ate);
		out.seekp(P[i],std::ios::beg);
		concat ( files[i] , out );
		if ( rem )
			::remove ( files[i].c_str() );
		out.flush();
		out.close();
	}
	
	return P[files.size()];
}

uint64_t libmaus::util::Concat::concatParallel(
	std::vector < std::vector < std::string > > const & files, 
	std::string const & outputfilename, 
	bool const rem
)
{
	std::vector<std::string> sfiles;
	
	for ( uint64_t i = 0; i < files.size(); ++i )
		for ( uint64_t j = 0; j < files[i].size(); ++j )
			sfiles.push_back(files[i][j]);

	return concatParallel(sfiles,outputfilename,rem);
}

uint64_t libmaus::util::Concat::concat(std::vector < std::string > const & files, std::string const & outputfile, bool const rem)
{
	libmaus::aio::CheckedOutputStream out(outputfile.c_str(),std::ios::binary);
	uint64_t const c = concat(files,out,rem);
	out.flush();
	out.close();
	return c;
}			

uint64_t libmaus::util::Concat::concat(
	std::vector < std::vector < std::string > > const & files, 
	std::string const & outputfilename, 
	bool const rem
)
{
	std::vector<std::string> sfiles;
	
	for ( uint64_t i = 0; i < files.size(); ++i )
		for ( uint64_t j = 0; j < files[i].size(); ++j )
			sfiles.push_back(files[i][j]);

	return concat(sfiles,outputfilename,rem);
}
