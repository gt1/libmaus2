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
#if ! defined(LIBMAUS2_UTIL_CONTAINERDESCRIPTION_HPP)
#define LIBMAUS2_UTIL_CONTAINERDESCRIPTION_HPP

#include <libmaus2/util/StringSerialisation.hpp>

namespace libmaus2
{
	namespace util
	{
		struct ContainerDescription
		{
			std::string fn;
			bool started;
			uint64_t missingdep;
			
			ContainerDescription() {}
			ContainerDescription(std::string const & rfn, bool const rstarted, uint64_t const rmissingdep) : fn(rfn), started(rstarted), missingdep(rmissingdep) {}
			ContainerDescription(std::istream & in)
			{
				deserialise(in);
			}
			
			std::istream & deserialise(std::istream & in)
			{
				fn = libmaus2::util::StringSerialisation::deserialiseString(in);
				started = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				missingdep = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				return in;
			}
			
			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::StringSerialisation::serialiseString(out,fn);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,started);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,missingdep);
				return out;
			}
		};
	}
}
#endif
