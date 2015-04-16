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
#if !defined(LIBMAUS2_LZ_LZ4DECODER_HPP)
#define LIBMAUS2_LZ_LZ4DECODER_HPP

#include <libmaus2/lz/Lz4DecoderBuffer.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <string>

namespace libmaus2
{
	namespace lz
	{
		struct Lz4Decoder : public Lz4DecoderBuffer, public ::std::istream
		{
			typedef Lz4Decoder this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			Lz4Decoder(std::string const & filename)
			: Lz4DecoderBuffer(filename), ::std::istream(this)
			{
				
			}
			
			Lz4Decoder(std::istream & in)
			: Lz4DecoderBuffer(in), ::std::istream(this)
			{
			
			}

			static int getSymbolAtPosition(std::string const & filename, uint64_t const offset)
			{
				this_type I(filename);
				I.seekg(offset);
				return I.get();
			}

			static uint64_t getFileSize(std::string const & filename)
			{
				this_type I(filename);
				I.seekg(0,std::ios::end);
				return I.tellg();
			}
		};

	}
}
#endif
