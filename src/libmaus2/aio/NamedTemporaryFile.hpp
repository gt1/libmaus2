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
#if ! defined(LIBMAUS2_AIO_NAMEDTEMPORARYFILE_HPP)
#define LIBMAUS2_AIO_NAMEDTEMPORARYFILE_HPP

#include <string>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct NamedTemporaryFile
		{
			typedef NamedTemporaryFile this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string const name;
			uint64_t const id;
			libmaus2::aio::OutputStream::unique_ptr_type stream;

			NamedTemporaryFile(std::string const & rname, uint64_t const rid)
			: name(rname), id(rid), stream()
			{
				libmaus2::aio::OutputStream::unique_ptr_type tstream(libmaus2::aio::OutputStreamFactoryContainer::constructUnique(name));
				stream = UNIQUE_PTR_MOVE(tstream);
			}

			static unique_ptr_type uconstruct(std::string const & rname, uint64_t const rid)
			{
				unique_ptr_type tptr(new this_type(rname,rid));
				return UNIQUE_PTR_MOVE(tptr);
			}

			static shared_ptr_type sconstruct(std::string const & rname, uint64_t const rid)
			{
				shared_ptr_type tptr(new this_type(rname,rid));
				return tptr;
			}

			uint64_t getId() const
			{
				return id;
			}

			std::string const & getName() const
			{
				return name;
			}

			libmaus2::aio::OutputStream & getStream()
			{
				return *stream;
			}
		};
	}
}
#endif
