/**
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
**/
#if ! defined(LIBMAUS_BAMBAM_ALIGNMENTVALIDITY_HPP)
#define LIBMAUS_BAMBAM_ALIGNMENTVALIDITY_HPP

#include <ostream>

namespace libmaus
{
	namespace bambam
	{
		enum libmaus_bambam_alignment_validity {
			libmaus_bambam_alignment_validity_ok = 0,
			libmaus_bambam_alignment_validity_block_too_small = 1,
			libmaus_bambam_alignment_validity_queryname_extends_over_block = 2,
			libmaus_bambam_alignment_validity_queryname_length_inconsistent = 3,
			libmaus_bambam_alignment_validity_cigar_extends_over_block = 4,
			libmaus_bambam_alignment_validity_sequence_extends_over_block = 5,
			libmaus_bambam_alignment_validity_quality_extends_over_block = 6,
			libmaus_bambam_alignment_validity_cigar_is_inconsistent_with_sequence_length = 7,
			libmaus_bambam_alignment_validity_unknown_cigar_op = 8,
			libmaus_bambam_alignment_validity_queryname_contains_illegal_symbols = 9,
			libmaus_bambam_alignment_validity_queryname_empty = 10,
			libmaus_bambam_alignment_validity_invalid_mapping_position = 11,
			libmaus_bambam_alignment_validity_invalid_next_mapping_position = 12,
			libmaus_bambam_alignment_validity_invalid_tlen = 13,
			libmaus_bambam_alignment_validity_invalid_quality_value = 14,
			libmaus_bambam_alignment_validity_invalid_refseq = 15,
			libmaus_bambam_alignment_validity_invalid_next_refseq = 16,
			libmaus_bambam_alignment_validity_invalid_auxiliary_data = 17
		};

		inline ::std::ostream & operator<<(std::ostream & out, libmaus_bambam_alignment_validity const v)
		{
			switch ( v )
			{
				case libmaus_bambam_alignment_validity_ok:
					out << "Alignment valid";
					break;
				case libmaus_bambam_alignment_validity_block_too_small:
					out << "Alignment block is too small to hold fixed size data";
					break;
				case libmaus_bambam_alignment_validity_queryname_extends_over_block:
					out << "Null terminated query name extends beyond block boundary";
					break;
				case libmaus_bambam_alignment_validity_queryname_length_inconsistent:
					out << "Length of null terminated query name is inconsistent with alignment header";
					break;
				case libmaus_bambam_alignment_validity_cigar_extends_over_block:
					out << "Cigar data extends beyond block boundary";
					break;
				case libmaus_bambam_alignment_validity_sequence_extends_over_block:
					out << "Sequence data extends beyond block boundary";
					break;
				case libmaus_bambam_alignment_validity_quality_extends_over_block:
					out << "Quality data extends beyond block boundary";
					break;
				case libmaus_bambam_alignment_validity_cigar_is_inconsistent_with_sequence_length:
					out << "Cigar operations are inconsistent with length of query sequence";
					break;
				case libmaus_bambam_alignment_validity_unknown_cigar_op:
					out << "Unknown/invalid cigar operator";
					break;
				case libmaus_bambam_alignment_validity_queryname_contains_illegal_symbols:
					out << "Query name contains illegal symbols";
					break;
				case libmaus_bambam_alignment_validity_queryname_empty:
					out << "Query name is the empty string";
					break;
				case libmaus_bambam_alignment_validity_invalid_mapping_position:
					out << "Invalid leftmost mapping position";
					break;
				case libmaus_bambam_alignment_validity_invalid_next_mapping_position:
					out << "Invalid next segment mapping position";
					break;
				case libmaus_bambam_alignment_validity_invalid_tlen:
					out << "Invalid observed template length";
					break;
				case libmaus_bambam_alignment_validity_invalid_quality_value:
					out << "Quality string contains invalid quality value";
					break;
				case libmaus_bambam_alignment_validity_invalid_refseq:
					out << "Invalid/unknown reference sequence identifier";
					break;
				case libmaus_bambam_alignment_validity_invalid_next_refseq:
					out << "Invalid/unknown next segment reference sequence identifier";
					break;
				case libmaus_bambam_alignment_validity_invalid_auxiliary_data:
					out << "Invalid auxiliary tag data";
					break;
				default:
					out << "Unknown alignment validity value.";
					break;
			};
			
			return out;
		}
	}
}
#endif
