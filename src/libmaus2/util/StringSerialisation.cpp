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

#include <libmaus2/util/StringSerialisation.hpp>
#include <libmaus2/util/CountPutObject.hpp>

uint64_t libmaus2::util::StringSerialisation::serialiseString(std::ostream & out, std::string const & s)
{
	::libmaus2::util::CountPutObject CPO;
	::libmaus2::util::UTF8::encodeUTF8(s.size(),CPO);

	::libmaus2::util::UTF8::encodeUTF8(s.size(),out);
	out.write ( s.c_str(), s.size() );

	if ( ! out )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "EOF/failure in ::libmaus2::util::StringSerialisation::serialiseStdString()";
		se.finish();
		throw se;
	}

	return CPO.c + s.size();
}

std::string libmaus2::util::StringSerialisation::deserialiseString(std::istream & in)
{
	uint64_t const u = ::libmaus2::util::UTF8::decodeUTF8(in);

	::libmaus2::autoarray::AutoArray<char> A(u,false);
	in.read ( A.get(), u );

	if ( (!in) || (in.gcount() != static_cast<int64_t>(u)) )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "EOF/failure in ::libmaus2::util::StringSerialisation::deserialiseStdString()";
		se.finish();
		throw se;
	}

	return std::string ( A.get(), A.get()+u );
}

void libmaus2::util::StringSerialisation::serialiseStringVector ( std::ostream & out, std::vector < std::string > const & V )
{
	serialiseNumber ( out,V.size() );
	for ( uint64_t i = 0; i < V.size(); ++i )
		serialiseString(out,V[i]);
}
std::vector < std::string > libmaus2::util::StringSerialisation::deserialiseStringVector ( std::istream & in )
{
	uint64_t const numstrings = deserialiseNumber(in);
	std::vector < std::string > strings;

	for ( uint64_t i = 0; i < numstrings; ++i )
		strings.push_back ( deserialiseString(in) );

	return strings;
}

std::vector < std::string > libmaus2::util::StringSerialisation::deserialiseStringVector ( std::string const & in )
{
	std::istringstream istr(in);
	return deserialiseStringVector(istr);
}

void libmaus2::util::StringSerialisation::serialiseStringVectorVector ( std::ostream & out, std::vector < std::vector < std::string > > const & V )
{
	serialiseNumber ( out,V.size() );
	for ( uint64_t i = 0; i < V.size(); ++i )
		serialiseStringVector(out,V[i]);
}

void libmaus2::util::StringSerialisation::serialiseStringVectorDeque ( std::ostream & out, std::deque < std::vector < std::string > > const & V )
{
	serialiseNumber ( out,V.size() );
	for ( uint64_t i = 0; i < V.size(); ++i )
		serialiseStringVector(out,V[i]);
}

std::vector < std::vector < std::string > > libmaus2::util::StringSerialisation::deserialiseStringVectorVector ( std::istream & in )
{
	uint64_t const numvectors = deserialiseNumber(in);
	std::vector < std::vector < std::string > > vectors;

	for ( uint64_t i = 0; i < numvectors; ++i )
		vectors.push_back ( deserialiseStringVector(in) );

	return vectors;
}

std::deque < std::vector < std::string > > libmaus2::util::StringSerialisation::deserialiseStringVectorDeque ( std::istream & in )
{
	uint64_t const numvectors = deserialiseNumber(in);
	std::deque < std::vector < std::string > > vectors;

	for ( uint64_t i = 0; i < numvectors; ++i )
		vectors.push_back ( deserialiseStringVector(in) );

	return vectors;
}

void libmaus2::util::StringSerialisation::serialiseDouble(std::ostream & out, double const v)
{
	std::ostringstream ostr; ostr << v;
	serialiseString(out,ostr.str());
}
std::string libmaus2::util::StringSerialisation::serialiseDouble(double const v)
{
	std::ostringstream ostr;
	serialiseDouble(ostr,v);
	return ostr.str();
}

double libmaus2::util::StringSerialisation::deserialiseDouble(std::istream & in)
{
	std::string const s = deserialiseString(in);
	std::istringstream istr(s);
	double v;
	istr >> v;
	return v;
}
double libmaus2::util::StringSerialisation::deserialiseDouble(std::string const & s)
{
	std::istringstream istr(s);
	return deserialiseDouble(istr);
}
