/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if !defined(LIBMAUS2_LZ_LZ4COMPRESS_HPP)
#define LIBMAUS2_LZ_LZ4COMPRESS_HPP

#include <ostream>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct Lz4Compress
		{
			private:
			uint64_t const inputblocksize;
			uint64_t const outputblocksize;

			libmaus2::autoarray::AutoArray<char> outputblock;

			std::ostream & out;

			uint64_t outputbyteswritten;
			uint64_t payloadbyteswritten;

			std::ostream * indexstream;

			public:
			Lz4Compress(std::ostream & rout, uint64_t const inputblocksize, std::ostream * rindexstream = 0);
			~Lz4Compress();

			void setIndexStream(std::ostream * rindexstream = 0)  { indexstream = rindexstream; }
			void writeUncompressed(char const * input, int const inputsize);
			void write(char const * input, int const inputsize);
			void flush();
			uint64_t align(uint64_t const mod);
			uint64_t getPayloadBytesWritten() const;
		};
	}
}
#endif
