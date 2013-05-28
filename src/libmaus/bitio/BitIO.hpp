/*
    libmaus
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


#if ! defined(BITIO_HPP)
#define BITIO_HPP

#include <cstdlib>
#include <libmaus/types/types.hpp>

namespace libmaus
{
	namespace bitio
	{
		inline uint8_t revertOctet(uint8_t v) {
			return
				((v & 0x80) >> 7) |
				((v & 0x40) >> 5) |
				((v & 0x20) >> 3) |
				((v & 0x10) >> 1) |
				((v & 0x08) << 1) |
				((v & 0x04) << 3) |
				((v & 0x02) << 5) |
				((v & 0x01) << 7);
		}
	}
}

#include <iostream>
#include <libmaus/bitio/BitIOInput.hpp>
#include <libmaus/bitio/BitIOOutput.hpp>

#endif
