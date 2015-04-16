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
#if ! defined(LIBMAUS2_BAMBAM_ALIGNMENTVALIDITY_HPP)
#define LIBMAUS2_BAMBAM_ALIGNMENTVALIDITY_HPP

#include <ostream>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * bam validation error codes
		 **/
		enum libmaus2_bambam_alignment_validity {
			libmaus2_bambam_alignment_validity_ok = 0,
			libmaus2_bambam_alignment_validity_block_too_small = 1,
			libmaus2_bambam_alignment_validity_queryname_extends_over_block = 2,
			libmaus2_bambam_alignment_validity_queryname_length_inconsistent = 3,
			libmaus2_bambam_alignment_validity_cigar_extends_over_block = 4,
			libmaus2_bambam_alignment_validity_sequence_extends_over_block = 5,
			libmaus2_bambam_alignment_validity_quality_extends_over_block = 6,
			libmaus2_bambam_alignment_validity_cigar_is_inconsistent_with_sequence_length = 7,
			libmaus2_bambam_alignment_validity_unknown_cigar_op = 8,
			libmaus2_bambam_alignment_validity_queryname_contains_illegal_symbols = 9,
			libmaus2_bambam_alignment_validity_queryname_empty = 10,
			libmaus2_bambam_alignment_validity_invalid_mapping_position = 11,
			libmaus2_bambam_alignment_validity_invalid_next_mapping_position = 12,
			libmaus2_bambam_alignment_validity_invalid_tlen = 13,
			libmaus2_bambam_alignment_validity_invalid_quality_value = 14,
			libmaus2_bambam_alignment_validity_invalid_refseq = 15,
			libmaus2_bambam_alignment_validity_invalid_next_refseq = 16,
			libmaus2_bambam_alignment_validity_invalid_auxiliary_data = 17
		};


		/**
		 * format string descrption of bam validation error code
		 *
		 * @param out output stream
		 * @param v error code
		 * @return output stream
		 **/
		::std::ostream & operator<<(::std::ostream & out, libmaus2_bambam_alignment_validity const v);
	}
}

#endif
