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
#if ! defined(LIBMAUS2_FASTX_LINEBUFFERFASTAREADER_HPP)
#define LIBMAUS2_FASTX_LINEBUFFERFASTAREADER_HPP

#include <libmaus2/fastx/acgtnMap.hpp>
#include <libmaus2/util/LineBuffer.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct LineBufferFastAReader
		{
			typedef LineBufferFastAReader this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::util::LineBuffer LB;
			libmaus2::autoarray::AutoArray<char> M;

			LineBufferFastAReader(std::istream & in)
			: LB(in,1024*1024), M(static_cast<uint64_t>(std::numeric_limits<unsigned char>::max())+1ull)
			{
				for ( uint64_t i = 0; i < M.size(); ++i )
					M[i] = libmaus2::fastx::mapChar(i);
			}

			inline char mapChar(char c) const
			{
				return M [ static_cast<unsigned char>(c) ];
			}

			static void increaseSize(libmaus2::autoarray::AutoArray<char> & data)
			{
				uint64_t const oldsize = data.size();
				uint64_t const one = 1;
				uint64_t const newsize = std::max(oldsize << 1,one);
				data.resize(newsize);
			}

			static void ensureSize(libmaus2::autoarray::AutoArray<char> & data, uint64_t const s)
			{
				while ( data.size() < s )
					increaseSize(data);
			}

			static void pushSymbol(libmaus2::autoarray::AutoArray<char> & data, uint64_t & o, char const c)
			{
				ensureSize(data,o+1);
				data[o++] = c;
			}

			static void pushString(libmaus2::autoarray::AutoArray<char> & data, uint64_t & o, char const * linea, char const * linee)
			{
				size_t const l = linee-linea;
				ensureSize(data,o+l);
				std::copy(linea,linee,data.begin()+o);
				o += l;
			}

			void pushMappedString(libmaus2::autoarray::AutoArray<char> & data, uint64_t & o, char const * linea, char const * linee)
			{
				size_t const l = linee-linea;
				ensureSize(data,o+l);
				while ( linea != linee )
					data[o++] = libmaus2::fastx::mapChar(*(linea++));
			}

			bool getNext(
				libmaus2::autoarray::AutoArray<char> & data,
				uint64_t & o,
				uint64_t & o_name,
				uint64_t & o_data,
				uint64_t & rlen,
				bool const termdata = true,
				bool const mapdata = false,
				bool const paddata = false,
				char const padsym = 4,
				bool const addrc = false
			)
			{
				char const * linea = 0, *linee = 0;

				if ( LB.getline(&linea,&linee) )
				{
					ptrdiff_t const len = linee-linea;
					if ( ! len )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "LineBufferFastAReader: unexpected empty line" << std::endl;
						lme.finish();
						throw lme;
					}
					else if ( linea[0] != '>' )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "LineBufferFastAReader: unexpected symbol " << linea[0] << std::endl;
						lme.finish();
						throw lme;
					}

					o_name = o;
					pushString(data,o,linea+1,linee);
					pushSymbol(data,o,0);

					bool ok;

					if ( paddata )
						pushSymbol(data,o,padsym);

					o_data = o;
					rlen = 0;
					while ( (ok=LB.getline(&linea,&linee)) && (linee-linea) && linea[0] != '>' )
					{
						rlen += (linee-linea);
						if ( mapdata )
							pushMappedString(data,o,linea,linee);
						else
							pushString(data,o,linea,linee);
					}

					if ( ok )
						LB.putback(linea);

					if ( paddata )
						pushSymbol(data,o,padsym);

					if ( termdata )
						pushSymbol(data,o,0);

					if ( addrc )
					{
						ensureSize(data,o + rlen + (paddata ? 2 : 0) + (termdata ? 1 : 0) );

						if ( paddata )
							data[o++] = padsym;

						char const * datastart = data.begin() + o_data;
						char const * dataend = datastart + rlen;

						if ( mapdata )
							while ( dataend != datastart )
								data[o++] = *(--dataend) ^ 3;
						else
							while ( dataend != datastart )
								data[o++] = libmaus2::fastx::invertUnmapped(*(--dataend));

						if ( paddata )
							data[o++] = padsym;
						if ( termdata )
							data[o++] = 0;
					}

					return true;
				}
				else
				{
					return false;
				}
			}

			struct ReadMeta
			{
				uint64_t nameoff;
				uint64_t dataoff;
				uint64_t len;

				ReadMeta(
					uint64_t rnameoff = 0,
					uint64_t rdataoff = 0,
					uint64_t rlen = 0
				) : nameoff(rnameoff), dataoff(rdataoff), len(rlen) {}
			};

			bool getNext(
				libmaus2::autoarray::AutoArray<char> & name,
				uint64_t & o_name,
				libmaus2::autoarray::AutoArray<char> & data,
				uint64_t & o_data,
				uint64_t & rlen,
				bool const termdata = true,
				bool const mapdata = false,
				bool const paddata = false,
				char const padsym = 4
			)
			{
				char const * linea = 0, *linee = 0;

				if ( LB.getline(&linea,&linee) )
				{
					ptrdiff_t const len = linee-linea;
					if ( ! len )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "LineBufferFastAReader: unexpected empty line" << std::endl;
						lme.finish();
						throw lme;
					}
					else if ( linea[0] != '>' )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "LineBufferFastAReader: unexpected symbol " << linea[0] << std::endl;
						lme.finish();
						throw lme;
					}

					pushString(name,o_name,linea+1,linee);
					pushSymbol(name,o_name,0);

					bool ok;
					rlen = 0;
					if ( paddata )
						pushSymbol(data,o_data,padsym);
					while ( (ok=LB.getline(&linea,&linee)) && (linee-linea) && linea[0] != '>' )
					{
						rlen += (linee-linea);
						if ( mapdata )
							pushMappedString(data,o_data,linea,linee);
						else
							pushString(data,o_data,linea,linee);
					}

					if ( ok )
						LB.putback(linea);

					if ( paddata )
						pushSymbol(data,o_data,padsym);

					if ( termdata )
						pushSymbol(data,o_data,0);

					return true;
				}
				else
				{
					return false;
				}
			}

			void pushReverseComplement(
				libmaus2::autoarray::AutoArray<char> & data, uint64_t & o_data,
				libmaus2::autoarray::AutoArray<ReadMeta> & readmeta,
				uint64_t const numreads,
				bool const termdata = true,
				bool const mapdata = false,
				bool const paddata = false,
				char const padsym = 4
			)
			{
				uint64_t onumreads = numreads;

				for ( uint64_t i = 0; i < numreads; ++i )
				{
					ReadMeta const RM = readmeta[i];
					uint64_t const readoffset = RM.dataoff;
					uint64_t const readlen = RM.len;
					uint64_t const nameoffset = RM.nameoff;

					if ( paddata )
						pushSymbol(data,o_data,padsym);

					uint64_t const nextreadoffset = o_data;

					// make sure we do not need to reallocate while pushing the base symbols
					ensureSize(data,o_data + readlen);

					char const * p = data.begin() + readoffset + readlen;

					if ( mapdata )
						for ( uint64_t j = 0; j < readlen; ++j )
							pushSymbol(data,o_data,(*(--p))^3);
					else
						for ( uint64_t j = 0; j < readlen; ++j )
							pushSymbol(data,o_data, libmaus2::fastx::invertUnmapped(*(--p)) );

					if ( paddata )
						pushSymbol(data,o_data,padsym);

					if ( termdata )
						pushSymbol(data,o_data,0);

					ReadMeta NRM;
					NRM.dataoff = nextreadoffset;
					NRM.len = readlen;
					NRM.nameoff = nameoffset;
					readmeta.push(onumreads,NRM);
				}
			}
		};
	}
}
#endif
