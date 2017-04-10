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
#if !defined(LIBMAUS2_UTIL_COMMANDCONTAINER_HPP)
#define LIBMAUS2_UTIL_COMMANDCONTAINER_HPP

#include <libmaus2/util/Command.hpp>

namespace libmaus2
{
	namespace util
	{
		struct CommandContainer
		{
			std::vector<Command> V;

			CommandContainer()
			{}
			CommandContainer(std::istream & in)
			{
				deserialise(in);
			}

			void serialise(std::ostream & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(out,V.size());
				for ( uint64_t i = 0; i < V.size(); ++i )
					V[i].serialise(out);
			}

			void deserialise(std::istream & in)
			{
				V.resize(libmaus2::util::NumberSerialisation::deserialiseNumber(in));
				for ( uint64_t i = 0; i < V.size(); ++i )
					V[i].deserialise(in);
			}

			void dispatch(uint64_t const i) const
			{
				V.at(i).dispatch();
			}
		};
	}
}

#endif
