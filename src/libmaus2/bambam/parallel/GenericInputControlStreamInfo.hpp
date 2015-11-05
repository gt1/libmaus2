/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTCONTROLSTRAMINFO_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTCONTROLSTRAMINFO_HPP

#include <string>
#include <libmaus2/types/types.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputControlStreamInfo
			{
				typedef GenericInputControlStreamInfo this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::string path;
				bool finite;
				uint64_t start;
				uint64_t end;
				bool hasheader;

				GenericInputControlStreamInfo() : path(), finite(true), start(0), end(0), hasheader(false) {}
				GenericInputControlStreamInfo(
					std::string const & rpath,
					bool const rfinite,
					uint64_t const rstart,
					uint64_t const rend,
					bool const rhasheader
				) : path(rpath), finite(rfinite), start(rstart), end(rend), hasheader(rhasheader) {}

				libmaus2::aio::InputStream::unique_ptr_type openStream() const
				{
					libmaus2::aio::InputStream::unique_ptr_type tptr(libmaus2::aio::InputStreamFactoryContainer::constructUnique(path));

					if ( start != 0 )
					{
						tptr->clear();
						tptr->seekg(start,std::ios::beg);
					}

					return UNIQUE_PTR_MOVE(tptr);
				}

				size_t byteSize() const
				{
					return
						path.size() +
						sizeof(finite) +
						sizeof(start) +
						sizeof(end) +
						sizeof(hasheader);
				}
			};

			std::ostream & operator<<(std::ostream & out, GenericInputControlStreamInfo const & G);
		}
	}
}
#endif
