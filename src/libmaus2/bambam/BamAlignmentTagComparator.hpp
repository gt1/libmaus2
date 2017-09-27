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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENTTAGCOMPARATOR_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENTTAGCOMPARATOR_HPP

#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/bambam/BamAlignmentNameComparator.hpp>
#include <libmaus2/bambam/BamAlignmentPosComparator.hpp>
#include <libmaus2/bambam/StrCmpNum.hpp>
#include <libmaus2/digest/Digests.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * comparator for BAM alignments by tag.
		 **/
		struct BamAlignmentTagComparator : public StrCmpNum, public DecoderBase
		{
			//! data pointer
			uint8_t const * data;
			char const * tagname;

			void setup(char const * rtagname)
			{
				tagname = rtagname;
			}

			/**
			 * constructor from data pointer
			 *
			 * @param rdata container data
			 **/
			BamAlignmentTagComparator(uint8_t const * rdata)
			:
				data(rdata), tagname(0)
			{

			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then
			 * da < db iff da is read 1
			 *
			 * @param da first string
			 * @param db second string
			 * @return true iff da < db (alignments referenced by da and db, not pointers)
			 **/
			static bool compare(uint8_t const * da, uint64_t const al, uint8_t const * db, uint64_t const bl, char const * tagname)
			{
				return compareInt(da,al,db,bl,tagname) < 0;
			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then
			 * da < db iff da is read 1
			 *
			 * @param da first alignment
			 * @param db second alignment
			 * @return true iff da < db (alignments referenced by da and db, not pointers)
			 **/
			static bool compare(libmaus2::bambam::BamAlignment const & A, libmaus2::bambam::BamAlignment const & B, char const * tagname)
			{
				return compare(A.D.begin(),A.blocksize,B.D.begin(),B.blocksize,tagname);
			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then compare by read1 flag (read1 < !read1)
			 *
			 * @param da first string
			 * @param db second string
			 * @return -1 if da<db, 0 if da == db, 1 if da > db  (alignments referenced by da and db, not pointers)
			 **/
			static int compareInt(uint8_t const * da, uint64_t const al, uint8_t const * db, uint64_t const bl, char const * tagname)
			{
				char const * taga = ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(da,al,tagname);
				char const * tagb = ::libmaus2::bambam::BamAlignmentDecoderBase::getAuxString(db,bl,tagname);

				if ( ! taga || ! tagb )
				{
					if ( taga )
						return -1;
					else if ( tagb )
						return 1;
					// both with no tag
					else
						return libmaus2::bambam::BamAlignmentPosComparator::compareInt(da,al,db,bl);
				}

				int const tagcomp = strcmpnum(taga,tagb);

				if ( tagcomp )
					return tagcomp;
				else
					return libmaus2::bambam::BamAlignmentPosComparator::compareInt(da,al,db,bl);
			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then compare by read1 flag (read1 < !read1)
			 *
			 * @param da first string
			 * @param db second string
			 * @return -1 if da<db, 0 if da == db, 1 if da > db  (alignments referenced by da and db, not pointers)
			 **/
			int compareInt(uint8_t const * da, uint64_t const al, uint8_t const * db, uint64_t const bl) const
			{
				return compareInt(da,al,db,bl,tagname);
			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then
			 * da < db iff da is read 1
			 *
			 * @param da first alignment
			 * @param db second alignment
			 * @return true iff da < db (alignments referenced by da and db, not pointers)
			 **/
			static bool compareInt(libmaus2::bambam::BamAlignment const & A, libmaus2::bambam::BamAlignment const & B, char const * tagname)
			{
				return compareInt(A.D.begin(),A.blocksize,B.D.begin(),B.blocksize,tagname);
			}

			/**
			 * compare alignments at offsets a and b in the data block
			 *
			 * @param a first offset into data block
			 * @param b second offset into data block
			 * @return true iff alignment at a < alignment at b (alignments at offset a and b, not offsets and b)
			 **/
			bool operator()(uint64_t const a, uint64_t const b) const
			{
				uint8_t const * da = data + a;
				uint64_t const la = static_cast<int32_t>(getLEInteger(da,4));
				uint8_t const * db = data + b;
				uint64_t const lb = static_cast<int32_t>(getLEInteger(db,4));
				return compare(da+sizeof(uint32_t),la,db+sizeof(uint32_t),lb,tagname);
			}

			/**
			 * compare alignments at offsets a and b in the data block
			 *
			 * @param a first offset into data block
			 * @param b second offset into data block
			 * @return -1 for a < b, 0 for a == b, 1 for a > b (alignments at offset a and b, not offsets and b)
			 **/
			int compareInt(uint64_t const a, uint64_t const b) const
			{
				uint8_t const * da = data + a;
				uint64_t const la = static_cast<int32_t>(getLEInteger(da,4));
				uint8_t const * db = data + b;
				uint64_t const lb = static_cast<int32_t>(getLEInteger(db,4));
				return compareInt(da+sizeof(uint32_t),la,db+sizeof(uint32_t),lb,tagname);
			}
		};
	}
}
#endif
