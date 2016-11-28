/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_BAMBAM_OPTNAME_HPP)
#define LIBMAUS2_BAMBAM_OPTNAME_HPP

#include <istream>
#include <ostream>
#include <libmaus2/util/StringSerialisation.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct OptName
		{
			typedef OptName this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t rank;
			std::string refreadname;

			OptName()
			{
			}

			OptName(uint64_t const rrank, std::string const & rrefreadname) : rank(rrank), refreadname(rrefreadname) {}
			OptName(std::istream & in)
			: rank(libmaus2::util::NumberSerialisation::deserialiseNumber(in)), refreadname(libmaus2::util::StringSerialisation::deserialiseString(in))
			{

			}

			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(out,rank);
				libmaus2::util::StringSerialisation::serialiseString(out,refreadname);
				return out;
			}

			std::istream & deserialise(std::istream & in)
			{
				*this = OptName(in);
				return in;
			}

			bool operator<(OptName const & O) const
			{
				return rank < O.rank;
			}
		};
	}
}
#endif
