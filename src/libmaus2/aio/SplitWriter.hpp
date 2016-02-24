/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_AIO_SPLITWRITER_HPP)
#define LIBMAUS2_AIO_SPLITWRITER_HPP

#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <vector>
#include <iomanip>

namespace libmaus2
{
	namespace aio
	{
		struct SplitWriter
		{
			typedef SplitWriter this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			std::string const basename;
			uint64_t id;
			uint64_t limit;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type OSI;
			std::vector<std::string> Vfn;
			uint64_t current;
			bool const registertmp;

			void closeFile()
			{
				if ( OSI )
				{
					OSI->flush();
					OSI.reset();
				}
			}

			void openNextFile()
			{
				std::ostringstream fnostr;
				fnostr << basename << "_" << std::setw(6) << std::setfill('0') << id++ << std::setw(0);
				std::string const fn = fnostr.str();
				if ( registertmp )
					libmaus2::util::TempFileRemovalContainer::addTempFile(fn);
				libmaus2::aio::OutputStreamInstance::unique_ptr_type TOSI(new libmaus2::aio::OutputStreamInstance(fn));
				OSI = UNIQUE_PTR_MOVE(TOSI);
				Vfn.push_back(fn);
				current = 0;
			}

			SplitWriter(std::string const & rbasename, uint64_t const rlimit, bool const rregistertmp = false)
			: basename(rbasename), id(0), limit(rlimit), current(0), registertmp(rregistertmp)
			{

			}

			void write(char const * c, size_t n)
			{
				while ( n )
				{
					if ( current == limit || (!OSI) )
						openNextFile();

					assert ( current != limit );

					uint64_t const towrite = std::min(n, limit - current);
					OSI->write(c,towrite);

					n -= towrite;
					c += towrite;
					current += towrite;
				}
			}

			void put(char c)
			{
				write(&c,1);
			}

			~SplitWriter()
			{
				closeFile();
			}

			static unique_ptr_type construct(std::string const & rbasename, uint64_t const rlimit)
			{
				unique_ptr_type ptr(new this_type(rbasename,rlimit));
				return UNIQUE_PTR_MOVE(ptr);
			}
		};
	}
}
#endif
