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
#include <libmaus2/dazzler/align/LasFileRange.hpp>

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
				OverlapParser::split_type splittype;

				bool const limitset;
				uint64_t const limit;
				uint64_t tr;

				int64_t seekpos;
				bool seekposenable;

				SimpleOverlapParser(
					std::istream & rin,
					int64_t const rtspace,
					uint64_t const bufsize = 32*1024,
					OverlapParser::split_type const rsplittype = OverlapParser::overlapparser_do_split,
					int64_t const rlimit = -1,
					int64_t const rseekpos = std::numeric_limits<int64_t>::min(),
					bool const rseekposenable = false
				)
				: PISI(), in(rin), AF(), parser(rtspace), Abuffer(bufsize), splittype(rsplittype), limitset(rlimit != -1), limit(rlimit), tr(0), seekpos(rseekpos), seekposenable(rseekposenable) {}

				SimpleOverlapParser(std::string const & fn, uint64_t const bufsize = 32*1024, OverlapParser::split_type const rsplittype = OverlapParser::overlapparser_do_split, int64_t const rlimit = -1)
				: PISI(new libmaus2::aio::InputStreamInstance(fn)), in(*PISI), AF(in), parser(AF.tspace), Abuffer(bufsize), splittype(rsplittype),
				  limitset(rlimit != -1), limit(rlimit), tr(0), seekpos(std::numeric_limits<int64_t>::min()), seekposenable(false)
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
					if ( seekposenable )
					{
						in.clear();
						in.seekg(seekpos);
					}

					if ( limitset )
					{
						uint64_t const rest = limit - tr;
						uint64_t const toread = std::min(static_cast<uint64_t>(Abuffer.size()),rest);
						in.read(Abuffer.begin(),toread);
					}
					else
					{
						in.read(Abuffer.begin(),Abuffer.size());
					}

					tr += in.gcount();

					if ( in.bad() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "SimpleOverlapParser::parseNextBlock(): stream error" << std::endl;
						lme.finish();
						throw lme;
					}

					std::streamsize const r = in.gcount();

					if ( seekposenable )
						seekpos = in.tellg();

					if ( ! r )
					{
						if ( parser.pushbackfill )
						{
							parser.parseBlock(
								reinterpret_cast<uint8_t const *>(Abuffer.begin()),
								reinterpret_cast<uint8_t const *>(Abuffer.begin()+r),
								OverlapParser::overlapparser_do_split
							);
							assert ( ! parser.pushbackfill );
							return true;
						}

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
							reinterpret_cast<uint8_t const *>(Abuffer.begin()+r),
							splittype
						);

						return true;
					}
				}
			};

			struct SimpleOverlapParserConcat
			{
				typedef SimpleOverlapParserConcat this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
				SimpleOverlapParser::unique_ptr_type Pparser;
				int64_t const tspace;
				OverlapParser::split_type const splittype;
				std::vector < LasFileRange > ranges;
				std::vector < std::string > Vfn;
				uint64_t index;
				uint64_t bufsize;

				bool openNext()
				{
					if ( index == ranges.size() )
					{
						Pparser.reset();
						PISI.reset();
						return false;
					}

					LasFileRange const & FR = ranges[index++];

					PISI.reset();

					libmaus2::aio::InputStreamInstance::unique_ptr_type TISI(new libmaus2::aio::InputStreamInstance(Vfn[FR.id]));
					PISI = UNIQUE_PTR_MOVE(TISI);
					PISI->clear();
					PISI->seekg(FR.startoffset);

					Pparser.reset();

					SimpleOverlapParser::unique_ptr_type Tparser(
						new SimpleOverlapParser(
							*PISI,
							tspace,
							bufsize,
							splittype,
							FR.endoffset - FR.startoffset
						)
					);

					Pparser = UNIQUE_PTR_MOVE(Tparser);

					return true;
				}

				SimpleOverlapParserConcat(
					int64_t const rtspace,
					std::vector < LasFileRange > const & rranges,
					std::vector < std::string > const & rVfn,
					uint64_t const rbufsize,
					OverlapParser::split_type const rsplittype = OverlapParser::overlapparser_do_split
				) : PISI(), Pparser(), tspace(rtspace), splittype(rsplittype), ranges(rranges), Vfn(rVfn), index(0), bufsize(rbufsize)
				{
					openNext();
				}

				OverlapData & getData()
				{
					return Pparser->getData();
				}

				bool parseNextBlock()
				{
					while ( Pparser )
					{
						bool const ok = Pparser->parseNextBlock();

						if ( ok )
							return true;
						else
							openNext();
					}

					return false;
				}
			};

			struct SimpleOverlapParserGet
			{
				typedef SimpleOverlapParserGet this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				SimpleOverlapParser & parser;
				uint64_t i;

				SimpleOverlapParserGet(SimpleOverlapParser & rparser) : parser(rparser), i(0) {}

				bool getNext(std::pair<uint8_t const *, uint8_t const *> & P)
				{
					while ( true )
					{
						OverlapData & data = parser.getData();
						if ( i < data.size() )
						{
							P = data.getData(i++);
							return true;
						}
						else
						{
							bool const ok = parser.parseNextBlock();

							if ( ok )
							{
								i = 0;
							}
							else
							{
								return false;
							}
						}
					}
				}
			};
		}
	}
}
#endif
