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

#if ! defined(LIBMAUS_UTIL_BITLIST_HPP)
#define LIBMAUS_UTIL_BITLIST_HPP

#include <cassert>
#include <list>
#include <ostream>
#include <sys/types.h>

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * bit vector stored in form of a linked list
		 **/
		struct BitList
		{
			//! stored bit vector a linked list
			std::list< bool > B;
			
			/**
			 * construct bit vector containing words*64 zero bits
			 *
			 * @param initial length of list in 64 bit units
			 **/
			BitList(uint64_t words);
			/**
			 * @param rank selector for 1 bit (zero based)
			 * @return index of rank'th 1
			 **/
			uint64_t select1(uint64_t rank) const;
			/**
			 * @param rank selector for 0 bit (zero based)
			 * @return index of rank'th 0
			 **/
			uint64_t select0(uint64_t rank) const;
			/**
			 * @param pos position
			 * @return umber of 1 bits up to and including position pos
			 **/
			uint64_t rank1(uint64_t pos) const;
			/**
			 * insert bit b at position pos
			 *
			 * @param pos position for insertion
			 * @param b bit to be inserted
			 **/
			void insertBit(uint64_t pos, bool b);
			/**
			 * delete bit a position pos
			 *
			 * @param pos position of bit to be deleted
			 **/
			void deleteBit(uint64_t pos);
			/**
			 * set bit at position pos to b
			 *
			 * @param pos position of bit to be set
			 * @param b new value for bit
			 **/
			void setBit(uint64_t pos, bool b);
		};

		/**
		 * print bit list B on stream out
		 *
		 * @param out output stream
		 * @param B bit list
		 * @return out
		 **/
		std::ostream & operator<<(std::ostream & out, BitList const & B);
	}
}
#endif
