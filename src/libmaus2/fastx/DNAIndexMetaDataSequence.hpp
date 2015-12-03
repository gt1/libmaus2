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
#if ! defined(LIBMAUS2_FASTX_DNAINDEXMETADATASEQUENCE_HPP)
#define LIBMAUS2_FASTX_DNAINDEXMETADATASEQUENCE_HPP

#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct DNAIndexMetaDataSequence
		{
			uint64_t l;
			std::vector< std::pair<uint64_t,uint64_t> > nblocks;

			DNAIndexMetaDataSequence() : l(0), nblocks() {}
			DNAIndexMetaDataSequence(std::istream & in)
			{
				l = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				uint64_t const numnblocks = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				nblocks.resize(numnblocks);
				for ( uint64_t i = 0; i < numnblocks; ++i )
				{
					uint64_t const from = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					uint64_t const to = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					nblocks[i] = std::pair<uint64_t,uint64_t>(from,to);
				}
			}
		};

		std::ostream & operator<<(std::ostream & out, DNAIndexMetaDataSequence const & D);
	}
}
#endif
