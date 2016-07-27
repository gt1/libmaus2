/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_LCS_ALIGNMENTSTATISTICS_HPP)
#define LIBMAUS2_LCS_ALIGNMENTSTATISTICS_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <ostream>

namespace libmaus2
{
	namespace lcs
	{
		struct AlignmentStatistics
		{
			uint64_t matches;
			uint64_t mismatches;
			uint64_t insertions;
			uint64_t deletions;

			AlignmentStatistics()
			: matches(0), mismatches(0), insertions(0), deletions(0)
			{

			}

			AlignmentStatistics(
				uint64_t const rmatches,
				uint64_t const rmismatches,
				uint64_t const rinsertions,
				uint64_t const rdeletions
			)
			: matches(rmatches), mismatches(rmismatches), insertions(rinsertions), deletions(rdeletions)
			{

			}

			AlignmentStatistics(std::istream & in)
			{
				deserialise(in);
			}

			AlignmentStatistics(std::string const & fn)
			{
				deserialise(fn);
			}

			AlignmentStatistics & operator+=(AlignmentStatistics const & O)
			{
				matches += O.matches;
				mismatches += O.mismatches;
				insertions += O.insertions;
				deletions += O.deletions;
				return *this;
			}

			double getErrorRate() const
			{
				return
					static_cast<double>(mismatches + insertions + deletions) /
					static_cast<double>(matches + mismatches + insertions + deletions);
			}

			uint64_t getEditDistance() const
			{
				return mismatches + insertions + deletions;
			}

			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(out,matches);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,mismatches);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,insertions);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,deletions);
				return out;
			}

			void serialise(std::string const & fn) const
			{
				libmaus2::aio::OutputStreamInstance OSI(fn);
				serialise(OSI);
			}

			std::istream & deserialise(std::istream & in)
			{
				matches = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				mismatches = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				insertions = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				deletions = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				return in;
			}

			void deserialise(std::string const & fn)
			{
				libmaus2::aio::InputStreamInstance ISI(fn);
				deserialise(ISI);
			}
		};

		std::ostream & operator<<(std::ostream & out, AlignmentStatistics const & A);
	}
}
#endif
