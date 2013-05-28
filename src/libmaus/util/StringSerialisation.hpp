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
			static void serialiseString(std::ostream & out, std::string const & s);
			static std::string deserialiseString(std::istream & in);
			static void serialiseStringVector ( std::ostream & out, std::vector < std::string > const & V );
			static std::vector < std::string > deserialiseStringVector ( std::istream & in );
			static void serialiseStringVectorVector ( std::ostream & out, std::vector < std::vector < std::string > > const & V );
			static void serialiseStringVectorDeque ( std::ostream & out, std::deque < std::vector < std::string > > const & V );
			static std::vector < std::vector < std::string > > deserialiseStringVectorVector ( std::istream & in );
			static std::deque < std::vector < std::string > > deserialiseStringVectorDeque ( std::istream & in );
			static void serialiseDouble(std::ostream & out, double const v);
			static std::string serialiseDouble(double const v);
			static double deserialiseDouble(std::istream & in);
			static double deserialiseDouble(std::string const & s);
		};
	}
}
#endif
