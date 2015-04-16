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

#if ! defined(LIBMAUS_UTIL_GENERICINTERVALTREE_HPP)
#define LIBMAUS_UTIL_GENERICINTERVALTREE_HPP

#include <libmaus/util/IntervalTree.hpp>

namespace libmaus
{
	namespace util
	{
		/**
		 * generalised interval tree allowing empty intervals
		 **/
		struct GenericIntervalTree
		{
			//! this type
			typedef GenericIntervalTree this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			/**
			 * compute array of non empty intervals
			 *
			 * @param V input array
			 * @return output array without empty intervals
			 **/
			static ::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > computeNonEmpty(::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & V);
			/**
			 * compute bit vector with 1 bits for non empty and 0 bits for empty intervals
			 *
			 * @param V interval array
			 * @return bit vector with 1s for non empty and 0s for empty intervals
			 **/
			static ::libmaus::bitio::IndexedBitVector::unique_ptr_type computeNonEmptyBV(::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & V);
			
			//! array of non empty intervals
			::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > nonempty;
			//! bit vector for mapping non empty to empty intervals
			::libmaus::bitio::IndexedBitVector::unique_ptr_type BV;
			//! interval tree on non empty interval array
			IntervalTree::unique_ptr_type IT;
			
			/**
			 * constructor by interval array [a_0,a_1),[a_1,a_2],... where a_i <= a_{i+1}
			 *
			 * @param H interval array
			 **/
			GenericIntervalTree(::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const & H);
			/**
			 * find interval containing value v
			 *
			 * @param v
			 * @return interval index containing value v
			 **/
			uint64_t find(uint64_t const v) const;
		};
	}
}
#endif
