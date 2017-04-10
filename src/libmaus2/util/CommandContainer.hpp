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
				std::vector<uint64_t> O;
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					uint64_t o = out.tellp();
					O.push_back(o);
					V[i].serialise(out);
				}

				for ( uint64_t i = 0; i < O.size(); ++i )
					libmaus2::util::NumberSerialisation::serialiseNumber(out,O[i]);
			}

			void deserialise(std::istream & in)
			{
				V.resize(libmaus2::util::NumberSerialisation::deserialiseNumber(in));
				for ( uint64_t i = 0; i < V.size(); ++i )
					V[i].deserialise(in);
			}

			int dispatch(uint64_t const i) const
			{
				return V.at(i).dispatch();
			}

			static int dispatch(std::istream & istr, uint64_t const i)
			{
				istr.clear();
				istr.seekg(0,std::ios::beg);
				uint64_t const n = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				assert ( i < n );

				istr.clear();
				istr.seekg(
					- static_cast<int64_t>( n * sizeof(uint64_t) ) + static_cast<int64_t>( i * sizeof(uint64_t) ),
					std::ios::end
				);

				uint64_t const p = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);

				istr.clear();
				istr.seekg(p);

				Command const C(istr);
				return C.dispatch();
			}
		};
	}
}

#endif
