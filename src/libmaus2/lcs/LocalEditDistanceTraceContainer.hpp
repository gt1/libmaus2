/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_LCS_LOCALEDITDISTANCETRACECONTAINER_HPP)
#define LIBMAUS2_LCS_LOCALEDITDISTANCETRACECONTAINER_HPP

#include <libmaus2/lcs/LocalAlignmentTraceContainer.hpp>
#include <libmaus2/lcs/LocalAlignmentPrint.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct LocalEditDistanceTraceContainer : public LocalAlignmentTraceContainer, public LocalAlignmentPrint
		{
			LocalEditDistanceTraceContainer(uint64_t const tracelen = 0) : LocalAlignmentTraceContainer(tracelen) {}

			template<typename iterator>
			std::ostream & printAlignment(
				std::ostream & out,
				iterator ita,
				iterator itb
			) const
			{
				LocalAlignmentPrint::printAlignment(out,ita,itb,ta,te);
				return out;
			}
			std::ostream & printAlignmentLines(
				std::ostream & out,
				std::string const & a,
				std::string const & b,
				uint64_t const rlinewidth
			) const
			{
				LocalAlignmentPrint::printAlignmentLines(out,a.begin(),a.end(),b.begin(),b.end(),rlinewidth,ta,te);
				return out;
			}

			void resize(uint64_t const tracelen)
			{
				LocalAlignmentTraceContainer::resize(tracelen);
			}
			uint64_t capacity() const
			{
				return LocalAlignmentTraceContainer::capacity();
			}

			LocalAlignmentTraceContainer const & getTrace() const
			{
				return static_cast<LocalAlignmentTraceContainer const &>(*this);
			}
		};
	}
}
#endif
