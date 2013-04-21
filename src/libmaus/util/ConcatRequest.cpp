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

#include <libmaus/util/ConcatRequest.hpp>

libmaus::util::ConcatRequest::ConcatRequest(
	std::vector<std::string> const & rinfilenames,
	std::string const & routfilename)
: infilenames(rinfilenames), outfilename(routfilename)
{

}

libmaus::util::ConcatRequest::ConcatRequest(std::istream & in)
: 
infilenames ( ::libmaus::util::StringSerialisation::deserialiseStringVector(in) ),
outfilename ( ::libmaus::util::StringSerialisation::deserialiseString(in) )
{
	
}

void libmaus::util::ConcatRequest::serialise(std::ostream & out) const
{
	::libmaus::util::StringSerialisation::serialiseStringVector(out,infilenames);
	::libmaus::util::StringSerialisation::serialiseString(out,outfilename);
}

void libmaus::util::ConcatRequest::serialise(std::string const & filename) const
{
	std::ofstream ostr(filename.c_str(), std::ios::binary);
	serialise(ostr);
	ostr.flush();
	assert ( ostr );
	ostr.close();
}

void libmaus::util::ConcatRequest::serialise(
	std::vector<std::string> const & infilenames,
	std::string const & outfilename,
	std::string const & requestfilename)
{
	ConcatRequest CR(infilenames,outfilename);
	CR.serialise(requestfilename);
}

void libmaus::util::ConcatRequest::serialise(
	std::string const & infilename,
	std::string const & outfilename,
	std::string const & requestfilename)
{
	ConcatRequest CR(std::vector<std::string>(1,infilename),outfilename);
	CR.serialise(requestfilename);
}

void libmaus::util::ConcatRequest::execute(bool const removeFiles) const
{
	::libmaus::util::Concat::concat(infilenames,outfilename,removeFiles);
}

libmaus::util::ConcatRequest::unique_ptr_type libmaus::util::ConcatRequest::load(std::string const & requestfilename)
{
	std::ifstream istr(requestfilename.c_str(),std::ios::binary);
	if ( ! istr.is_open() )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "Failed to open file " << requestfilename << std::endl;
		se.finish();
		throw se;
	}
	::libmaus::util::ConcatRequest::unique_ptr_type req ( new ::libmaus::util::ConcatRequest(istr) );
	if ( !istr )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "Failed to deserialise from file " << requestfilename << std::endl;
		se.finish();
		throw se;
	}

	return UNIQUE_PTR_MOVE(req);
}
