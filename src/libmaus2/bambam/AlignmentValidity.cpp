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
#include <libmaus2/bambam/AlignmentValidity.hpp>

::std::ostream & libmaus2::bambam::operator<<(::std::ostream & out, libmaus2::bambam::libmaus2_bambam_alignment_validity const v)
{
	switch ( v )
	{
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_ok:
			out << "Alignment valid";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_block_too_small:
			out << "Alignment block is too small to hold fixed size data";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_queryname_extends_over_block:
			out << "Null terminated query name extends beyond block boundary";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_queryname_length_inconsistent:
			out << "Length of null terminated query name is inconsistent with alignment header";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_cigar_extends_over_block:
			out << "Cigar data extends beyond block boundary";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_sequence_extends_over_block:
			out << "Sequence data extends beyond block boundary";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_quality_extends_over_block:
			out << "Quality data extends beyond block boundary";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_cigar_is_inconsistent_with_sequence_length:
			out << "Cigar operations are inconsistent with length of query sequence";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_unknown_cigar_op:
			out << "Unknown/invalid cigar operator";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_queryname_contains_illegal_symbols:
			out << "Query name contains illegal symbols";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_queryname_empty:
			out << "Query name is the empty string";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_invalid_mapping_position:
			out << "Invalid leftmost mapping position";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_invalid_next_mapping_position:
			out << "Invalid next segment mapping position";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_invalid_tlen:
			out << "Invalid observed template length";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_invalid_quality_value:
			out << "Quality string contains invalid quality value";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_invalid_refseq:
			out << "Invalid/unknown reference sequence identifier";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_invalid_next_refseq:
			out << "Invalid/unknown next segment reference sequence identifier";
			break;
		case libmaus2::bambam::libmaus2_bambam_alignment_validity_invalid_auxiliary_data:
			out << "Invalid auxiliary tag data";
			break;
		default:
			out << "Unknown alignment validity value.";
			break;
	};

	return out;
}
