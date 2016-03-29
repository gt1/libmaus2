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
#if ! defined(LIBMAUS2_FASTX_FASTATOCOMPACT4BIGBAND)
#define LIBMAUS2_FASTX_FASTATOCOMPACT4BIGBAND

#include <vector>
#include <string>
#include <ostream>

namespace libmaus2
{
	namespace fastx
	{
		struct FastAToCompact4BigBand
		{
			static int fagzToCompact4BigBand(
				std::vector<std::string> const & inputfilenames,
				bool const rc,
				bool const replrc,
				std::ostream * logstr,
				std::string outputfilename = std::string()
			);
		};
	}
}
#endif
