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

#include <libmaus/bitio/CompactArray.hpp>

unsigned int ::libmaus::bitio::CompactArrayBase::globalBitsInFirstWord[4160];
unsigned int ::libmaus::bitio::CompactArrayBase::globalFirstShift[4160];
uint64_t libmaus::bitio::CompactArrayBase::globalFirstKeepMask[4160];
uint64_t libmaus::bitio::CompactArrayBase::globalFirstValueKeepMask[4160];
unsigned int ::libmaus::bitio::CompactArrayBase::globalLastShift[4160];
uint64_t libmaus::bitio::CompactArrayBase::globalLastMask[4160];
uint64_t libmaus::bitio::CompactArrayBase::globalGetFirstMask[4160];
uint64_t libmaus::bitio::CompactArrayBase::globalvmask[65];
bool ::libmaus::bitio::CompactArrayBase::globalinit = false;
