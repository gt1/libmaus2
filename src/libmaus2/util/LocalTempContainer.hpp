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
#if ! defined(LIBMAUS2_UTIL_LOCALTEMPCONTAINER_HPP)
#define LIBMAUS2_UTIL_LOCALTEMPCONTAINER_HPP

#include <vector>
#include <string>
#include <iomanip>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/parallel/PosixSpinLock.hpp>

namespace libmaus2
{
	namespace util
	{
		struct LocalTempContainer
		{
			typedef LocalTempContainer this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			std::string const prefix;
			uint64_t volatile next;
			std::vector<std::string> V;
			libmaus2::parallel::PosixSpinLock lock;

			LocalTempContainer & operator=(LocalTempContainer const & O);
			LocalTempContainer(LocalTempContainer const & O);

			public:
			LocalTempContainer(std::string const & rprefix)
			: prefix(rprefix), next(0), V(), lock()
			{

			}

			virtual ~LocalTempContainer()
			{
				for ( uint64_t i = 0; i < V.size(); ++i )
					libmaus2::aio::FileRemoval::removeFile(V[i]);
			}

			std::string get()
			{
				std::ostringstream fnstr;
				{
					libmaus2::parallel::ScopePosixSpinLock slock(lock);
					fnstr << prefix << "_" << std::setw(8) << std::setfill('0') << next++;
				}
				std::string const fn = fnstr.str();
				{
					libmaus2::parallel::ScopePosixSpinLock slock(lock);
					V.push_back(fn);
				}
				return fn;
			}
		};
	}
}
#endif
