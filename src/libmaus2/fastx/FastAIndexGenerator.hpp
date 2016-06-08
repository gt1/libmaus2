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
#if ! defined(LIBMAUS2_FASTX_FASTAINDEXGENERATOR_HPP)
#define LIBMAUS2_FASTX_FASTAINDEXGENERATOR_HPP

#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/fastx/FastAIndex.hpp>
#include <libmaus2/fastx/SpaceTable.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/util/LineBuffer.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct FastAIndexGenerator
		{
			static void generate(std::string const & fastaname, std::string const & fainame, int const verbose)
			{
				if ( (! libmaus2::util::GetFileSize::fileExists(fainame)) || (libmaus2::util::GetFileSize::isOlder(fainame,fastaname) ) )
				{
					if ( verbose > 0 )
						std::cerr << "[V] generating " << fainame << "...";

					libmaus2::aio::InputStreamInstance ISI(fastaname);
					libmaus2::aio::OutputStreamInstance OSI(fainame);
					libmaus2::util::LineBuffer LB(ISI, 8*1024);
					libmaus2::fastx::SpaceTable ST;

					char const * a = 0;
					char const * e = 0;

					std::string seqname;
					int64_t linewidth = -1;
					int64_t linelength = -1;
					uint64_t seqlength = 0;
					uint64_t seqoffset = 0;
					uint64_t o = 0;

					while ( LB.getline(&a,&e) )
					{
						if ( e-a && *a == '>' )
						{
							if ( seqname.size() )
								OSI << seqname << "\t" << seqlength << "\t" << seqoffset << "\t" << linewidth << "\t" << linelength << std::endl;

							seqname = libmaus2::fastx::FastAIndex::computeShortName(std::string(a+1,e));
							linewidth = -1;
							linelength = -1;
							seqlength = 0;
						}
						else
						{
							uint64_t nonspace = 0;
							for ( char const * c = a; c != e && !ST.spacetable[static_cast<uint8_t>(static_cast<unsigned char>(*c))]; ++c )
								++nonspace;

							if ( linewidth < 0 )
							{

								linewidth = nonspace;
								linelength = e-a + 1 /* newline */;
								seqoffset = o;
							}
							else
							{
								// todo: check line width consistency
							}

							seqlength += nonspace;
						}

						o += (e-a)+1;
					}
					if ( seqname.size() )
						OSI << seqname << "\t" << seqlength << "\t" << seqoffset << "\t" << linewidth << "\t" << linelength << std::endl;

					if ( verbose > 0 )
						std::cerr << "done." << std::endl;
				}
			}
		};
	}
}
#endif
