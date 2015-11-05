/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_FASTX_FASTALINEPARSER_HPP)
#define LIBMAUS2_FASTX_FASTALINEPARSER_HPP

#include <libmaus2/fastx/FastAMapTable.hpp>
#include <libmaus2/fastx/CharTermTable.hpp>
#include <libmaus2/fastx/SpaceTable.hpp>
#include <libmaus2/fastx/FastALineParserLineInfo.hpp>
#include <istream>

namespace libmaus2
{
	namespace fastx
	{
		struct FastALineParser
		{
			typedef std::istream stream_type;
			stream_type & stream;
			libmaus2::fastx::CharTermTable const newlineterm;
			libmaus2::fastx::SpaceTable const spacetable;
			libmaus2::fastx::FastAMapTable const FMT;
			libmaus2::autoarray::AutoArray<uint8_t> data;
			libmaus2::autoarray::AutoArray<uint8_t> id;
			int64_t idlen;

			bool haveputback;
			::libmaus2::fastx::FastALineParserLineInfo putbackinfo;

			FastALineParser(stream_type & rstream)
			: stream(rstream), newlineterm('\n'), idlen(-1), haveputback(false)
			{
				int c = stream_type::traits_type::eof();

				// look for start marker
				while ( (c=stream.peek()) != stream_type::traits_type::eof() )
					if ( c == '>' )
						break;
					else
						stream.get();
			}

			void putback(::libmaus2::fastx::FastALineParserLineInfo const & info)
			{
				putbackinfo = info;
				haveputback = true;
			}

			bool getNextLine(::libmaus2::fastx::FastALineParserLineInfo & info)
			{
				if ( haveputback )
				{
					haveputback = false;
					info = putbackinfo;
					return info.linetype != ::libmaus2::fastx::FastALineParserLineInfo::libmaus2_fastx_fasta_id_line_eof;
				}

				uint8_t * pa = data.begin();
				uint8_t * pc = pa;
				uint8_t * pe = data.end();
				int c = stream_type::traits_type::eof();

				while ( ! newlineterm[c=stream.get()] )
				{
					if ( pc == pe )
					{
						libmaus2::autoarray::AutoArray<uint8_t> ndata(
							std::max(static_cast<uint64_t>(1),static_cast<uint64_t>(2*data.size())),false);
						std::copy(data.begin(),data.end(),ndata.begin());

						pa = ndata.begin();
						pc = pa + data.size();
						pe = ndata.end();

						data = ndata;
					}

					*(pc++) = c;
				}

				while ( pa != pc && spacetable.spacetable[*pa] )
					++pa;

				info.line = pa;
				info.linelen = pc - pa;

				if ( (info.linelen == 0) && (c == stream_type::traits_type::eof()) )
				{
					info.linetype = ::libmaus2::fastx::FastALineParserLineInfo::libmaus2_fastx_fasta_id_line_eof;
					return false;
				}
				else if ( info.linelen > 0 && *pa == '>' )
				{
					info.line++;
					info.linelen--;
					info.linetype = ::libmaus2::fastx::FastALineParserLineInfo::libmaus2_fastx_fasta_id_line;

					if ( id.size() < info.linelen )
						id = libmaus2::autoarray::AutoArray<uint8_t>(info.linelen);

					idlen = info.linelen;
					memcpy(id.begin(),info.line,info.linelen);
					return true;
				}
				else
				{
					info.linetype = ::libmaus2::fastx::FastALineParserLineInfo::libmaus2_fastx_fasta_base_line;

					uint8_t * op = pa;

					for ( uint8_t * p = pa; p != pc; ++p )
						if ( spacetable.nospacetable[*p] )
							*(op++) = FMT[(*p)];

					pc = op;
					info.linelen = pc-pa;

					return true;
				}
			}
		};
	}
}
#endif
