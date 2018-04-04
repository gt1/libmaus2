/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if !defined(LIBMAUS2_AIO_INPUTSTREAMOBJECT_HPP)
#define LIBMAUS2_AIO_INPUTSTREAMOBJECT_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/types/types.hpp>
#include <istream>

namespace libmaus2
{
	namespace aio
	{
		struct InputStreamObject
		{
			typedef InputStreamObject this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::istream * istr;
			uint64_t streamid;
			uint64_t volatile blockid;
			bool volatile eof;

			InputStreamObject(
				std::istream * ristr,
				uint64_t const rstreamid,
				uint64_t const rblockid = 0
			) : istr(ristr), streamid(rstreamid), blockid(rblockid), eof(false) {}
		};
	}
}
#endif
