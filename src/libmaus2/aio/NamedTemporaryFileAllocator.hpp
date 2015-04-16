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
#if ! defined(LIBMAUS_AIO_NAMEDTEMPORARYFILEALLOCATOR_HPP)
#define LIBMAUS_AIO_NAMEDTEMPORARYFILEALLOCATOR_HPP

#include <libmaus2/aio/NamedTemporaryFile.hpp>
#include <libmaus2/parallel/SynchronousCounter.hpp>
#include <iomanip>
#include <sstream>

namespace libmaus2
{
	namespace aio
	{
		template<typename _stream_type>
		struct NamedTemporaryFileAllocator
		{
			typedef _stream_type stream_type;
			
			std::string prefix;
			libmaus2::parallel::SynchronousCounter<uint64_t> * S;
			
			NamedTemporaryFileAllocator() : prefix(), S(0) {}
			NamedTemporaryFileAllocator(
				std::string const & rprefix,
				libmaus2::parallel::SynchronousCounter<uint64_t> * const rS
			) : prefix(rprefix), S(rS)
			{
			
			}
			
			typename libmaus2::aio::NamedTemporaryFile<stream_type>::shared_ptr_type operator()()
			{
				uint64_t const lid = static_cast<uint64_t>((*S)++);
				std::ostringstream fnostr;
				fnostr << prefix << "_" << std::setw(6) << std::setfill('0') << lid;
				std::string const fn = fnostr.str();
				typename libmaus2::aio::NamedTemporaryFile<stream_type>::shared_ptr_type ptr =
					libmaus2::aio::NamedTemporaryFile<stream_type>::sconstruct(fn,lid);
				return ptr;
			}
		};
	}
}
#endif
