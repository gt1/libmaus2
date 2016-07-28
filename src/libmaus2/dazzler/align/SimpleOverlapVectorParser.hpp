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

#if ! defined(LIBMAUS2_DAZZLER_ALIGN_SIMPLEOVERLAPVECTORPARSER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_SIMPLEOVERLAPVECTORPARSER_HPP

#include <libmaus2/dazzler/align/SimpleOverlapParser.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct SimpleOverlapVectorParser
			{
				typedef SimpleOverlapVectorParser this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::vector<std::string> const Vfn;
				std::vector<std::string>::const_iterator it;
				uint64_t const bufsize;
				SimpleOverlapParser::unique_ptr_type Pparser;
				OverlapParser::split_type const splittype;

				void openFile(std::string const & fn)
				{
					uint64_t const lasheadersize = libmaus2::dazzler::align::AlignmentFile::getSerialisedHeaderSize();
					uint64_t const lasfilesize = libmaus2::util::GetFileSize::getFileSize(fn);
					SimpleOverlapParser::unique_ptr_type Tparser(new SimpleOverlapParser(fn,bufsize,splittype,lasfilesize-lasheadersize));
					Pparser = UNIQUE_PTR_MOVE(Tparser);
				}

				SimpleOverlapVectorParser(
					std::vector<std::string> const & rVfn,
					uint64_t const rbufsize = 32*1024,
					OverlapParser::split_type const rsplittype = OverlapParser::overlapparser_do_split
				)
				: Vfn(rVfn), it(Vfn.begin()), bufsize(rbufsize), splittype(rsplittype)
				{
				}

				OverlapData & getData()
				{
					return Pparser->getData();
				}

				bool parseNextBlock()
				{
					while ( true )
					{
						if ( ! Pparser )
						{
							if ( it == Vfn.end() )
								return false;
							else
								openFile(*(it++));
						}
						else
						{
							if ( Pparser->parseNextBlock() )
							{
								return true;
							}
							else
							{
								Pparser.reset();
							}
						}
					}
				}
			};
		}
	}
}
#endif
