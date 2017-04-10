/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if !defined(LIBMAUS2_UTIL_COMMAND_HPP)
#define LIBMAUS2_UTIL_COMMAND_HPP

#include <libmaus2/util/StringSerialisation.hpp>

namespace libmaus2
{
	namespace util
	{
		struct Command
		{
			std::string in;
			std::string out;
			std::string err;
			std::vector<std::string> args;

			Command()
			{

			}
			Command(std::string const & rin, std::string const & rout, std::string const & rerr, std::vector<std::string> const & rargs)
			: in(rin), out(rout), err(rerr), args(rargs)
			{
			}
			Command(std::istream & istr)
			{
				deserialise(istr);
			}

			void serialise(std::ostream & ostr) const
			{
				libmaus2::util::StringSerialisation::serialiseString(ostr,in);
				libmaus2::util::StringSerialisation::serialiseString(ostr,out);
				libmaus2::util::StringSerialisation::serialiseString(ostr,err);
				libmaus2::util::StringSerialisation::serialiseStringVector(ostr,args);
			}

			void deserialise(std::istream & istr)
			{
				in = libmaus2::util::StringSerialisation::deserialiseString(istr);
				out = libmaus2::util::StringSerialisation::deserialiseString(istr);
				err = libmaus2::util::StringSerialisation::deserialiseString(istr);
				args = libmaus2::util::StringSerialisation::deserialiseStringVector(istr);
			}

			int dispatch() const;
		};
	}
}

#endif
