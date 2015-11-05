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

#if ! defined(TEMPFILENAMEGENERATOR_HPP)
#define TEMPFILENAMEGENERATOR_HPP

#include <libmaus2/math/Log.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/util/TempFileNameGeneratorState.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <vector>
#include <string>
#include <cassert>
#include <iomanip>
#include <algorithm>

/* for mkdir */
#include <sys/stat.h>
#include <sys/types.h>

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

namespace libmaus2
{
	namespace util
	{
		struct TempFileNameGenerator
		{
			typedef TempFileNameGenerator this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			::libmaus2::parallel::OMPLock lock;

			TempFileNameGeneratorState state;
			TempFileNameGeneratorState const startstate;

			TempFileNameGenerator(std::string const rprefix, unsigned int const rdepth);
			~TempFileNameGenerator();

			std::string getFileName();
			void cleanupDirs();
		};
	}
}
#endif
