/*
    libmaus2
    Copyright (C) 2009-2016 German Tischler

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
#if ! defined(LIBMAUS2_AIO_DEBUGLINEOUTPUTSTREAMBUFFER_HPP)
#define LIBMAUS2_AIO_DEBUGLINEOUTPUTSTREAMBUFFER_HPP

#include <libmaus2/LibMausConfig.hpp>

#include <ostream>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/StreamLock.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct DebugLineOutputStreamBuffer : public ::std::streambuf
		{
			private:
			std::ostream & out;
			libmaus2::parallel::PosixSpinLock & lock;
			libmaus2::autoarray::AutoArray<char> A;
			uint64_t o;

			public:
			DebugLineOutputStreamBuffer(
				std::ostream & rout,
				libmaus2::parallel::PosixSpinLock & rlock
			)
			: out(rout), lock(rlock), o(0)
			{
				setp(NULL,NULL);
			}

			~DebugLineOutputStreamBuffer()
			{
				sync();
			}

			int_type overflow(int_type c = traits_type::eof())
			{
				if ( c != traits_type::eof() )
				{
					A.push(o,c);
					if ( c == '\n' )
						sync();
				}

				return c;
			}

			int sync()
			{
				{
					libmaus2::parallel::ScopePosixSpinLock slock(lock);
					out.write(A.begin(),o);
					o = 0;
				}
				return 0; // no error, -1 for error
			}
		};
	}
}
#endif
