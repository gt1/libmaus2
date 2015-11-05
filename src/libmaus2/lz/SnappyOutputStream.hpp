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
#if ! defined(LIBMAUS2_LZ_SNAPPYOUTPUTSTREAM_HPP)
#define LIBMAUS2_LZ_SNAPPYOUTPUTSTREAM_HPP

#include <libmaus2/lz/SnappyCompress.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/utf8.hpp>
#include <utility>

namespace libmaus2
{
	namespace lz
	{
		template<typename _stream_type>
		struct SnappyOutputStream
		{
			typedef _stream_type stream_type;
			typedef SnappyOutputStream<stream_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			::std::ostream & out;
			::libmaus2::autoarray::AutoArray<char> B;
			char * const pa;
			char *       pc;
			char * const pe;

			uint64_t compressedbyteswritten;

			size_t byteSize() const
			{
				return
					sizeof(out) +
					B.byteSize() +
					sizeof(pa) +
					sizeof(pc) +
					sizeof(pe);
			}

			SnappyOutputStream(stream_type & rout, uint64_t const bufsize = 64*1024)
			: out(rout), B(bufsize), pa(B.begin()), pc(pa), pe(B.end()), compressedbyteswritten(0)
			{
			}
			~SnappyOutputStream()
			{
				flush();
			}

			std::pair<uint64_t,uint64_t> getOffset() const
			{
				return std::pair<uint64_t,uint64_t>(compressedbyteswritten,pc-pa);
			}

			operator bool()
			{
				return true;
			}

			void flush()
			{
				if ( pc != pa )
				{
					// compress data
					std::string const cdata = SnappyCompress::compress(std::string(pa,pc));

					// store size of uncompressed buffer
					compressedbyteswritten += ::libmaus2::util::NumberSerialisation::serialiseNumber(out,pc-pa);

					// store size of compressed buffer
					compressedbyteswritten += ::libmaus2::util::NumberSerialisation::serialiseNumber(out,cdata.size());

					// write compressed data
					out.write(cdata.c_str(),cdata.size());
					compressedbyteswritten += cdata.size();

					if ( ! out )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Failed to flush SnappyOutputStream." << std::endl;
						se.finish();
						throw se;
					}
				}

				pc = pa;
			}

			void put(int const c)
			{
				*(pc++) = c;

				if ( pc == pe )
					flush();
			}

			void write(char const * c, uint64_t n)
			{
				while ( n )
				{
					uint64_t const sp = (pe-pc);
					uint64_t const tocopy = std::min(n,sp);
					std::copy(c,c+tocopy,pc);

					c += tocopy;
					pc += tocopy;
					n -= tocopy;

					if ( pc == pe )
						flush();
				}
			}
		};
	}
}
#endif
