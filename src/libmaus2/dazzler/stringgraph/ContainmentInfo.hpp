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
#if !defined(LIBMAUS2_DAZZLER_STRINGGRAPH_CONTAINMENTINFO_HPP)
#define LIBMAUS2_DAZZLER_STRINGGRAPH_CONTAINMENTINFO_HPP

#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace stringgraph
		{

			struct ContainmentInfo
			{
				uint64_t bread;
				uint64_t aread;
				uint64_t abpos;
				uint64_t aepos;
				uint64_t flags;
				uint64_t bbpos;
				uint64_t bepos;

				ContainmentInfo() {}
				ContainmentInfo(uint64_t const raread) : aread(raread) {}
				ContainmentInfo(std::istream & in)
				:
					bread(libmaus2::util::NumberSerialisation::deserialiseNumber(in,4)),
					aread(libmaus2::util::NumberSerialisation::deserialiseNumber(in,4)),
					abpos(libmaus2::util::NumberSerialisation::deserialiseNumber(in,4)),
					aepos(libmaus2::util::NumberSerialisation::deserialiseNumber(in,4)),
					flags(libmaus2::util::NumberSerialisation::deserialiseNumber(in,1)),
					bbpos(libmaus2::util::NumberSerialisation::deserialiseNumber(in,4)),
					bepos(libmaus2::util::NumberSerialisation::deserialiseNumber(in,4))
				{}

				void serialise(std::ostream & out) const
				{
					libmaus2::util::NumberSerialisation::serialiseNumber(out,bread,4);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,aread,4);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,abpos,4);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,aepos,4);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,flags,1);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,bbpos,4);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,bepos,4);
				}

				void deserialise(std::istream & in)
				{
					bread = libmaus2::util::NumberSerialisation::deserialiseNumber(in,4);
					aread = libmaus2::util::NumberSerialisation::deserialiseNumber(in,4);
					abpos = libmaus2::util::NumberSerialisation::deserialiseNumber(in,4);
					aepos = libmaus2::util::NumberSerialisation::deserialiseNumber(in,4);
					flags = libmaus2::util::NumberSerialisation::deserialiseNumber(in,1);
					bbpos = libmaus2::util::NumberSerialisation::deserialiseNumber(in,4);
					bepos = libmaus2::util::NumberSerialisation::deserialiseNumber(in,4);
				}

				bool operator<(ContainmentInfo const & O) const
				{
					if ( aread != O.aread )
						return aread < O.aread;
					else if ( bread != O.bread )
						return bread < O.bread;
					else if ( abpos != O.abpos )
						return abpos < O.abpos;
					else if ( aepos != O.aepos )
						return aepos < O.aepos;
					else if ( flags != O.flags )
						return flags < O.flags;
					else
						return false;
				}
			};

			struct ContainmentInfoAReadComparator
			{
				bool operator()(ContainmentInfo const & A, ContainmentInfo const & B) const
				{
					return A.aread < B.aread;
				}
			};

			inline std::ostream & operator<<(std::ostream & out, ContainmentInfo const & C)
			{
				out << "ContainmentInfo("
					<< "aread=" << C.aread
					<< ",bread=" << C.bread
					<< ",abpos=" << C.abpos
					<< ",aepos=" << C.aepos
					<< ",flags=" << C.flags
					<< ",bbpos=" << C.bbpos
					<< ",bepos=" << C.bepos
					<< ")";
				return out;
			}

		}
	}
}
#endif
