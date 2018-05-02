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
#if ! defined(LIBMAUS2_UTIL_CONTAINERDESCRIPTIONLIST_HPP)
#define LIBMAUS2_UTIL_CONTAINERDESCRIPTIONLIST_HPP

#include <libmaus2/util/ContainerDescription.hpp>

namespace libmaus2
{
	namespace util
	{
		struct ContainerDescriptionList
		{
			std::vector < ContainerDescription > V;
			std::vector < uint64_t > O;

			ContainerDescriptionList()
			{

			}

			ContainerDescriptionList(std::istream & in)
			{
				deserialise(in);
			}

			void replace(uint64_t const i, ContainerDescription const & CD, std::ostream & out)
			{
				uint64_t const s = CD.serialisedSize();
				assert ( O[i+1]-O[i] == s );
				out.clear();
				out.seekp(O[i]);
				CD.serialise(out);
			}

			std::istream & deserialise(std::istream & in)
			{
				uint64_t const n = libmaus2::util::NumberSerialisation::deserialiseNumber(in);

				for ( uint64_t i = 0; i < n; ++i )
					V.push_back(ContainerDescription(in));

				O.resize(n+1);
				for ( uint64_t i = 0; i < n+1; ++i )
					O[i] = libmaus2::util::NumberSerialisation::deserialiseNumber(in);

				return in;
			}

			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(out,V.size());
				std::vector < uint64_t > O(V.size()+1);
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					O[i] = out.tellp();
					V[i].serialise(out);
				}
				O[V.size()] = out.tellp();

				for ( uint64_t i = 0; i < O.size(); ++i )
					libmaus2::util::NumberSerialisation::serialiseNumber(out,V.size());

				return out;
			}
		};
	}
}
#endif
