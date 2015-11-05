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
#if ! defined(LIBMAUS2_AIO_BUFFER_HPP)
#define LIBMAUS2_AIO_BUFFER_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace aio
	{
		template<typename _element_type>
		struct Buffer
		{
			typedef _element_type element_type;
			libmaus2::autoarray::AutoArray<element_type> A;
			element_type * const pa;
			element_type * pc;
			element_type * const pe;

			Buffer(uint64_t const bufsize = 8*1024)
			: A(bufsize,false), pa(A.begin()), pc(pa), pe(A.end())
			{

			}

			bool put(element_type const & B)
			{
				*(pc++) = B;
				return pc == pe;
			}

			template<typename stream_type>
			std::pair<uint64_t,uint64_t> flush(stream_type & out)
			{
				uint64_t const start = out.tellp();

				if ( pc != pa )
				{
					std::sort(pa,pc);
					out.write(reinterpret_cast<char const *>(pa),(pc-pa)*sizeof(element_type));
					pc = pa;
				}

				uint64_t const end = out.tellp();

				return std::pair<uint64_t,uint64_t>(start,end);
			}

			bool empty() const
			{
				return pc == pa;
			}
		};
	}
}
#endif
