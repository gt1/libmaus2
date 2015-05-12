/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_DB_HPP)
#define LIBMAUS2_DAZZLER_DB_HPP

#include <libmaus2/types/types.hpp>
#include <string>
#include <ostream>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct FastaInfo
			{
				uint64_t fnumreads;
				std::string const fastaprolog;
				std::string fastafn;
				
				FastaInfo()
				: fnumreads(0)
				{
				
				}

				FastaInfo(uint64_t const rfnumreads, std::string const & rfastaprolog, std::string const & rfastafn)
				: fnumreads(rfnumreads), fastaprolog(rfastaprolog), fastafn(rfastafn)
				{
				
				}
			};

			std::ostream & operator<<(std::ostream & out, FastaInfo const & info);
		}
	}
}
#endif
