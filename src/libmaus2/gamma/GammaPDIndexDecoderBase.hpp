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
#if ! defined(LIBMAUS2_GAMMA_GAMMAPDINDEXDECODERBASE_HPP)
#define LIBMAUS2_GAMMA_GAMMAPDINDEXDECODERBASE_HPP

#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/GetFileSize.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct GammaPDIndexDecoderBase
		{
			static uint64_t getIndexOffset(std::istream & ISI)
			{
				ISI.clear();
				ISI.seekg(-static_cast<int64_t>(1*sizeof(uint64_t)),std::ios::end);
				return libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
			}

			static uint64_t getNumIndexEntries(std::istream & ISI)
			{
				uint64_t const indexoffset = getIndexOffset(ISI);
				uint64_t const fs = ::libmaus2::util::GetFileSize::getFileSize(ISI);
				assert ( fs > indexoffset );
				assert ( (fs-sizeof(uint64_t)) >= indexoffset );
				uint64_t const indexbytes = (fs-sizeof(uint64_t))-indexoffset;
				assert ( indexbytes % sizeof(uint64_t) == 0 );
				uint64_t const indexwords = indexbytes / sizeof(uint64_t);
				assert ( indexwords % 2 == 0 );
				uint64_t const indexentries = indexwords / 2;
				return indexentries;
			}

			static uint64_t getNumBlocks(std::istream & ISI)
			{
				uint64_t const indexentries = getNumIndexEntries(ISI);
				assert ( indexentries > 0 );
				return indexentries-1;
			}

			static uint64_t getNumValues(std::istream & ISI)
			{
				ISI.clear();
				ISI.seekg(-static_cast<int64_t>(2*sizeof(uint64_t)),std::ios::end);
				return libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
			}

			static uint64_t getNumValues(std::string const & fn)
			{
				libmaus2::aio::InputStreamInstance ISI(fn);
				return getNumValues(ISI);
			}
		};
	}
}
#endif
