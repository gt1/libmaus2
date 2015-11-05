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
#if ! defined(LIBMAUS2_FASTX_COMPACTFASTQHEADER_HPP)
#define LIBMAUS2_FASTX_COMPACTFASTQHEADER_HPP

#include <libmaus2/fastx/FqWeightQuantiser.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct CompactFastQHeader
		{
			typedef CompactFastQHeader this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint64_t const blocklen;
			uint64_t const numreads;
			unsigned int const qbits;
			::libmaus2::fastx::FqWeightQuantiser const quant;

			uint64_t byteSize() const
			{
				return
					2*sizeof(uint64_t)+1*sizeof(unsigned int)+quant.byteSize();
			}

			template<typename stream_type>
			CompactFastQHeader(stream_type & stream)
			:
			  blocklen(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
			  numreads(::libmaus2::util::NumberSerialisation::deserialiseNumber(stream)),
			  qbits(stream.get()),
			  quant(stream)
			{

			}

			static uint64_t getEmptyBlockHeaderSize()
			{
				::libmaus2::fastx::FqWeightQuantiser quant;
				std::ostringstream ostr;
				quant.serialise(ostr);
				return 2*sizeof(uint64_t) + 1 * sizeof(uint8_t) + ostr.str().size();
			}
		};
	}
}
#endif
