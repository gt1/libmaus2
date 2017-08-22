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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPINFOINDEXER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPINFOINDEXER_HPP

#include <libmaus2/dazzler/align/OverlapInfo.hpp>
#include <libmaus2/util/GetFileSize.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapInfoIndexer
			{
				static void createInfoIndex(std::string const & fn, uint64_t const n)
				{
					uint64_t const fs = libmaus2::util::GetFileSize::getFileSize(fn);

					std::vector<uint64_t> O(n,fs);

					libmaus2::aio::InputStreamInstance ISI(fn);
					int64_t prevaread = -1;

					while ( ISI.peek() != std::istream::traits_type::eof() )
					{
						uint64_t const p = ISI.tellg();
						libmaus2::dazzler::align::OverlapInfo I(ISI);
						int64_t const aread = (I.aread>>1);

						if ( aread != prevaread )
						{
							O [ aread ] = p;
							assert ( aread < static_cast<int64_t>(n) );
							prevaread = aread;
						}
					}

					uint64_t next = fs;
					for ( uint64_t ii = 0; ii < O.size(); ++ii )
					{
						uint64_t const i = O.size()-ii-1;

						if ( O[i] == fs )
							O[i] = next;
						else
							next = O[i];
					}

					std::string const indexfn = fn + ".index";
					libmaus2::aio::OutputStreamInstance OSI(indexfn);
					for ( uint64_t i = 0; i < O.size(); ++i )
						libmaus2::util::NumberSerialisation::serialiseNumber(OSI,O[i]);
					OSI.flush();
				}
			};
		}
	}
}
#endif
