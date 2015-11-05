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
#if ! defined(LIBMAUS2_DAZZLER_DB_READ_HPP)
#define LIBMAUS2_DAZZLER_DB_READ_HPP

#include <libmaus2/dazzler/db/InputBase.hpp>
#include <libmaus2/dazzler/db/OutputBase.hpp>
#include <libmaus2/dazzler/db/GetByteCounter.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/fastx/acgtnMap.hpp>


namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct Read : public libmaus2::dazzler::db::InputBase, public libmaus2::dazzler::db::OutputBase
			{
				static int32_t const DB_QV = 0x3ff;
				static int32_t const DB_CSS = 0x400;
				static int32_t const DB_BEST = 0x800;
				static size_t const serialisedSize;

				int32_t origin;
				int32_t rlen;
				int32_t fpulse;
				int64_t boff;
				int64_t coff;
				int32_t flags;

				static size_t computeSerialisedSize()
				{
					GetByteCounter GBC;
					Read R;
					R.deserialise(GBC);
					return GBC.c;
				}

				template<typename stream_type>
				void deserialise(stream_type & in)
				{
					uint64_t offset = 0;
					origin = getLittleEndianInteger4(in,offset);
					rlen   = getLittleEndianInteger4(in,offset);
					fpulse = getLittleEndianInteger4(in,offset);
					boff   = getLittleEndianInteger8(in,offset);
					coff   = getLittleEndianInteger8(in,offset);
					flags  = getLittleEndianInteger4(in,offset);
					libmaus2::dazzler::db::InputBase::align(in,sizeof(int64_t),offset);
				}

				uint64_t serialise(std::ostream & out) const
				{
					uint64_t offset = 0;
					putLittleEndianInteger4(out,origin,offset);
					putLittleEndianInteger4(out,rlen,offset);
					putLittleEndianInteger4(out,fpulse,offset);
					putLittleEndianInteger8(out,boff,offset);
					putLittleEndianInteger8(out,coff,offset);
					putLittleEndianInteger4(out,flags,offset);
					libmaus2::dazzler::db::OutputBase::align(out,sizeof(int64_t),offset);
					return offset;
				}

				Read()
				{

				}

				Read(std::istream & in)
				{
					deserialise(in);
				}

				template<typename iterator>
				static size_t encode(std::ostream & out, iterator ita, iterator ite)
				{
					uint64_t s = 0;

					while ( ita != ite )
					{
						unsigned int c = 0;
						char C[4] = {0,0,0,0};
						while ( ita != ite && c < 4 )
							C[c++] = libmaus2::fastx::mapChar(*(ita++)) & 3;
						uint8_t const v =
							(C[0] << 6) |
							(C[1] << 4) |
							(C[2] << 2) |
							(C[3] << 0);
						out.put(v);
						++s;
					}

					return s;
				}

				size_t decode(std::istream & in, libmaus2::autoarray::AutoArray<char> & C, bool const seek = true)
				{
					if ( seek )
						in.seekg(boff,std::ios::beg);
					if ( static_cast<int32_t>(C.size()) < rlen )
						C = libmaus2::autoarray::AutoArray<char>(rlen,false);

					in.read(C.end() - (rlen+3)/4,(rlen+3)/4);
					if ( in.gcount() != (rlen+3)/4 )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::dazzler::db::Read::decode(): input failure" << std::endl;
						lme.finish();
						throw lme;
					}

					unsigned char * p = reinterpret_cast<unsigned char *>(C.end() - ((rlen+3)>>2));
					char * o = C.begin();
					for ( int32_t i = 0; i < (rlen>>2); ++i )
					{
						unsigned char v = *(p++);

						*(o++) = libmaus2::fastx::remapChar((v >> 6)&3);
						*(o++) = libmaus2::fastx::remapChar((v >> 4)&3);
						*(o++) = libmaus2::fastx::remapChar((v >> 2)&3);
						*(o++) = libmaus2::fastx::remapChar((v >> 0)&3);
					}
					if ( rlen & 3 )
					{
						unsigned char v = *(p++);
						size_t rest = rlen - ((rlen>>2)<<2);

						if ( rest )
						{
							*(o++) = libmaus2::fastx::remapChar((v >> 6)&3);
							rest--;
						}
						if ( rest )
						{
							*(o++) = libmaus2::fastx::remapChar((v >> 4)&3);
							rest--;
						}
						if ( rest )
						{
							*(o++) = libmaus2::fastx::remapChar((v >> 2)&3);
							rest--;
						}
					}

					return rlen;
				}
			};

			std::ostream & operator<<(std::ostream & out, Read const & R);
		}
	}
}
#endif
