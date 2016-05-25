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
#if ! defined(LIBMAUS2_LCP_COMPUTESUCCINCTLCPRESULT_HPP)
#define LIBMAUS2_LCP_COMPUTESUCCINCTLCPRESULT_HPP

#include <libmaus2/util/StringSerialisation.hpp>

namespace libmaus2
{
	namespace lcp
	{
		struct ComputeSuccinctLCPResult
		{
			std::vector<std::string> Vfn;
			uint64_t sa_0;
			uint64_t circularshift;

			ComputeSuccinctLCPResult() {}
			ComputeSuccinctLCPResult(std::vector<std::string> const & rVfn, uint64_t const rsa_0, uint64_t const rcircularshift)
			: Vfn(rVfn), sa_0(rsa_0), circularshift(rcircularshift) {}

			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::StringSerialisation::serialiseStringVector(out,Vfn);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,sa_0);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,circularshift);
				return out;
			}

			void serialise(std::string const & fn) const
			{
				libmaus2::aio::OutputStreamInstance OSI(fn);
				serialise(OSI);
			}

			void deserialise(std::istream & in)
			{
				Vfn = libmaus2::util::StringSerialisation::deserialiseStringVector(in);
				sa_0 = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				circularshift = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
			}
		};
	}
}
#endif
