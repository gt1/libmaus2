/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS2_UTIL_TEMPFILECONTAINER_HPP)
#define LIBMAUS2_UTIL_TEMPFILECONTAINER_HPP

#include <libmaus2/types/types.hpp>
#include <istream>
#include <ostream>

namespace libmaus2
{
	namespace util
	{
		struct TempFileContainer
		{
			virtual ~TempFileContainer() {}
			
			virtual std::ostream & openOutputTempFile(uint64_t) = 0;
			virtual std::ostream & getOutputTempFile(uint64_t) = 0;
			virtual void closeOutputTempFile(uint64_t) = 0;
			virtual std::istream & openInputTempFile(uint64_t) = 0;
			virtual void closeInputTempFile(uint64_t) = 0;
		};
	}
}
#endif
