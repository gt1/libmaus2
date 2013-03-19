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

#if ! defined(STRINGSERIALISATION_HPP)
#define STRINGSERIALISATION_HPP

#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <deque>
#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/util/utf8.hpp>

namespace libmaus
{
	namespace util
	{
		struct StringSerialisation : public NumberSerialisation
		{
			static void serialiseString(std::ostream & out, std::string const & s)
			{
				::libmaus::util::UTF8::encodeUTF8(s.size(),out);
				out.write ( s.c_str(), s.size() );

				if ( ! out )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "EOF/failure in ::libmaus::util::StringSerialisation::serialiseStdString()";
					se.finish();
					throw se;
				}
			}

			static std::string deserialiseString(std::istream & in)
			{
				uint64_t const u = ::libmaus::util::UTF8::decodeUTF8(in);
				
				::libmaus::autoarray::AutoArray<char> A(u,false);
				in.read ( A.get(), u );
				
				if ( (!in) || (in.gcount() != static_cast<int64_t>(u)) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "EOF/failure in ::libmaus::util::StringSerialisation::deserialiseStdString()";
					se.finish();
					throw se;
				}

				return std::string ( A.get(), A.get()+u );		
			}
			
			static void serialiseStringVector ( std::ostream & out, std::vector < std::string > const & V )
			{
				serialiseNumber ( out,V.size() );
				for ( uint64_t i = 0; i < V.size(); ++i )
					serialiseString(out,V[i]);
			}
			static std::vector < std::string > deserialiseStringVector ( std::istream & in )
			{
				uint64_t const numstrings = deserialiseNumber(in);
				std::vector < std::string > strings;
				
				for ( uint64_t i = 0; i < numstrings; ++i )
					strings.push_back ( deserialiseString(in) );
				
				return strings;
			}

			static void serialiseStringVectorVector ( std::ostream & out, std::vector < std::vector < std::string > > const & V )
			{
				serialiseNumber ( out,V.size() );
				for ( uint64_t i = 0; i < V.size(); ++i )
					serialiseStringVector(out,V[i]);
			}

			static void serialiseStringVectorDeque ( std::ostream & out, std::deque < std::vector < std::string > > const & V )
			{
				serialiseNumber ( out,V.size() );
				for ( uint64_t i = 0; i < V.size(); ++i )
					serialiseStringVector(out,V[i]);
			}

			static std::vector < std::vector < std::string > > deserialiseStringVectorVector ( std::istream & in )
			{
				uint64_t const numvectors = deserialiseNumber(in);
				std::vector < std::vector < std::string > > vectors;
				
				for ( uint64_t i = 0; i < numvectors; ++i )
					vectors.push_back ( deserialiseStringVector(in) );
				
				return vectors;
			}

			static std::deque < std::vector < std::string > > deserialiseStringVectorDeque ( std::istream & in )
			{
				uint64_t const numvectors = deserialiseNumber(in);
				std::deque < std::vector < std::string > > vectors;
				
				for ( uint64_t i = 0; i < numvectors; ++i )
					vectors.push_back ( deserialiseStringVector(in) );
				
				return vectors;
			}
			
			static void serialiseDouble(std::ostream & out, double const v)
			{
				std::ostringstream ostr; ostr << v;
				serialiseString(out,ostr.str());
			}
			static std::string serialiseDouble(double const v)
			{
				std::ostringstream ostr;
				serialiseDouble(ostr,v);
				return ostr.str();
			}
			
			static double deserialiseDouble(std::istream & in)
			{
				std::string const s = deserialiseString(in);
				std::istringstream istr(s);
				double v;
				istr >> v;
				return v;
			}
			static double deserialiseDouble(std::string const & s)
			{
				std::istringstream istr(s);
				return deserialiseDouble(istr);
			}
		};
	}
}
#endif
