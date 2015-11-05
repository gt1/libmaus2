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
#if ! defined(LIBMAUS2_LZ_SIMPLECOMPRESSEDINPUTBLOCK_HPP)
#define LIBMAUS2_LZ_SIMPLECOMPRESSEDINPUTBLOCK_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lz/DecompressorObjectFactory.hpp>
#include <libmaus2/util/CountPutObject.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/utf8.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct SimpleCompressedInputBlock
		{
			typedef SimpleCompressedInputBlock this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::lz::DecompressorObject::unique_ptr_type Pdecompressor;
			libmaus2::lz::DecompressorObject & decompressor;

			libmaus2::autoarray::AutoArray<uint8_t> I;
			libmaus2::autoarray::AutoArray<uint8_t> O;
			uint64_t metasize;
			uint64_t compsize;
			uint64_t uncompsize;
			bool eof;

			SimpleCompressedInputBlock(libmaus2::lz::DecompressorObjectFactory & factory)
			: Pdecompressor(factory()), decompressor(*Pdecompressor), I(), O(), compsize(0), uncompsize(0), eof(false)
			{

			}

			template<typename stream_type>
			bool readBlock(stream_type & stream)
			{
				if ( stream.peek() == stream_type::traits_type::eof() )
				{
					eof = true;
					return true;
				}

				libmaus2::util::CountPutObject CPO;
				uncompsize = libmaus2::util::UTF8::decodeUTF8(stream);
				::libmaus2::util::UTF8::encodeUTF8(uncompsize,CPO);

				compsize = ::libmaus2::util::NumberSerialisation::deserialiseNumber(stream);
				::libmaus2::util::NumberSerialisation::serialiseNumber(CPO,compsize);

				metasize = CPO.c;

				if ( compsize > I.size() )
					I = libmaus2::autoarray::AutoArray<uint8_t>(compsize,false);

				stream.read(reinterpret_cast<char *>(I.begin()),compsize);

				if ( stream.gcount() == static_cast<int64_t>(compsize) )
					return true;
				else
					return false;
			}

			bool uncompressBlock()
			{
				if ( uncompsize > O.size() )
					O = libmaus2::autoarray::AutoArray<uint8_t>(uncompsize,false);

				bool const ok = decompressor.rawuncompress(
					reinterpret_cast<char const *>(I.begin()),compsize,
					reinterpret_cast<char *>(O.begin()),uncompsize
				);

				return ok;
			}
		};
	}
}
#endif
