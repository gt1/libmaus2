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

#if ! defined(LIBMAUS2_DAZZLER_ALIGN_SIMPLEOVERLAPPARSER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_SIMPLEOVERLAPPARSER_HPP

#include <libmaus2/dazzler/align/OverlapParser.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct SimpleOverlapParser
			{
				typedef SimpleOverlapParser this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
				std::istream & in;
				libmaus2::dazzler::align::AlignmentFile AF;
				OverlapParser parser;
				libmaus2::autoarray::AutoArray<char> Abuffer;

				SimpleOverlapParser(std::istream & rin, uint64_t const bufsize = 32*1024)
				: PISI(), in(rin), AF(in), parser(AF.tspace), Abuffer(bufsize) {}

				SimpleOverlapParser(std::string const & fn, uint64_t const bufsize = 32*1024)
				: PISI(new libmaus2::aio::InputStreamInstance(fn)), in(*PISI), AF(in), parser(AF.tspace), Abuffer(bufsize)
				{}

				std::istream & getStream()
				{
					return in;
				}

				OverlapData & getData()
				{
					return parser.getData();
				}

				bool parseNextBlock()
				{
					in.read(Abuffer.begin(),Abuffer.size());

					if ( in.bad() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "SimpleOverlapParser::parseNextBlock(): stream error" << std::endl;
						lme.finish();
						throw lme;
					}

					std::streamsize const r = in.gcount();

					if ( ! r )
					{
						bool const ok = parser.isIdle();
						if ( ! ok )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "SimpleOverlapParser::parseNextBlock(): unexpected EOF while reading record" << std::endl;
							lme.finish();
							throw lme;
						}

						return false;
					}
					else
					{
						parser.parseBlock(
							reinterpret_cast<uint8_t const *>(Abuffer.begin()),
							reinterpret_cast<uint8_t const *>(Abuffer.begin()+r)
						);

						return true;
					}
				}
			};
		}
	}
}
#endif
