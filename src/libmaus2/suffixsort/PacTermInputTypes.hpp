/**
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
**/
#if ! defined(LIBMAUS2_SUFFIXSORT_PACTERMINPUTTYPES_HPP)
#define LIBMAUS2_SUFFIXSORT_PACTERMINPUTTYPES_HPP

#include <libmaus2/suffixsort/BwtMergeEnumBase.hpp>
#include <libmaus2/bitio/CompactDecoderBuffer.hpp>
#include <libmaus2/aio/CircularWrapper.hpp>
#include <libmaus2/suffixsort/CircularSuffixComparator.hpp>
#include <libmaus2/util/OctetString.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		struct PacTermInputTypes : public libmaus2::suffixsort::BwtMergeEnumBase
		{
			typedef ::std::istream base_input_stream;
			typedef ::libmaus2::bitio::PacDecoderTermWrapper linear_wrapper;
			typedef ::libmaus2::aio::PacTermCircularWrapper circular_wrapper;
			typedef ::libmaus2::aio::PacTermCircularReverseWrapper circular_reverse_wrapper;
			typedef ::libmaus2::suffixsort::PacTermCircularSuffixComparator circular_suffix_comparator;
			typedef ::libmaus2::util::OctetString string_type;
			typedef ::libmaus2::aio::CircularWrapper octet_circular_wrapper;
			
			static bwt_merge_sort_input_type getType()
			{
				return bwt_merge_input_type_pac_term;
			}
			static bool utf8Wavelet()
			{
				return false;
			}

			static uint64_t getSourceLengthBits(
				std::string const & /* fn */,
				uint64_t const low,
				uint64_t const high,
				std::map<int64_t,uint64_t> const & /* freqs */)
			{
				return 2 * (high-low);
			}


			static uint64_t getSourceLengthBytes(
				std::string const & /* fn */,
				uint64_t const low,
				uint64_t const high,
				std::map<int64_t,uint64_t> const & /* freqs */)
			{
				return (high-low);
			}

			static uint64_t getSourceTextIndexBits(
				std::string const & /* fn */,
				uint64_t const /* low */,
				uint64_t const /* high */,
				std::map<int64_t,uint64_t> const & /* freqs */)
			{
				return 0;
			}	
		};
	}
}
#endif
