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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTNAMECOMPARATOR_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTNAMECOMPARATOR_HPP

#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/StrCmpNum.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * comparator for BAM alignments by name. names are split up into character and number string components:
		 * name = c_0 n_0 c_1 n_1 ... . The string parts are compared lexicographically. The number parts
		 * are decoded as numbers and compared numerically. For equivalent names read 1 is smaller than read 2.
		 **/
		struct BamAlignmentNameComparator : public StrCmpNum
		{
			//! data pointer
			uint8_t const * data;
			
			/**
			 * constructor from data pointer
			 *
			 * @param rdata container data 
			 **/
			BamAlignmentNameComparator(uint8_t const * rdata)
			: data(rdata)
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
			static bool compare(uint8_t const * da, uint8_t const * db)
			{
				char const * namea = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(da);
				char const * nameb = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(db);

				int const r = strcmpnum(namea,nameb);
				bool res;
				
				if ( r < 0 )
				{
					res = true;
				}
				else if ( r == 0 )
				{
					// read 1 before read 2
					uint32_t const flagsa = ::libmaus::bambam::BamAlignmentDecoderBase::getFlags(da);
					uint32_t const flagsb = ::libmaus::bambam::BamAlignmentDecoderBase::getFlags(db);
					
					int const r1a = (flagsa & ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1) != 0;
					int const r1b = (flagsb & ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1) != 0;
					int const r2a = (flagsa & ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD2) != 0;
					int const r2b = (flagsb & ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD2) != 0;
					
					int const r1 = ((1-r1a) << 1) | (1-r2a);
					int const r2 = ((1-r1b) << 1) | (1-r2b);
					
					return (r1-r2) < 0;					
				}
				else
				{
					res = false;
				}
				
				return res;
			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then
			 * da < db iff da is read 1
			 *
			 * @param da first alignment
			 * @param db second alignment
			 * @return true iff da < db (alignments referenced by da and db, not pointers)
			 **/
			static bool compare(libmaus::bambam::BamAlignment const & A, libmaus::bambam::BamAlignment const & B)
			{
				return compare(A.D.begin(),B.D.begin());
			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then compare by read1 flag (read1 < !read1)
			 *
			 * @param da first string
			 * @param db second string
			 * @return -1 if da<db, 0 if da == db, 1 if da > db  (alignments referenced by da and db, not pointers)
			 **/
			static int compareInt(uint8_t const * da, uint8_t const * db)
			{
				char const * namea = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(da);
				char const * nameb = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(db);

				int const r = strcmpnum(namea,nameb);
				
				if ( r != 0 )
					return r;

				// read 1 before read 2
				int const r1 = ::libmaus::bambam::BamAlignmentDecoderBase::getFlags(da) & ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1;
				int const r2 = ::libmaus::bambam::BamAlignmentDecoderBase::getFlags(db) & ::libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREAD1;
				
				if ( r1 == r2 )
					return 0;
				else if ( r1 )
					return -1;
				else
					return 1;
			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then
			 * da < db iff da is read 1
			 *
			 * @param da first alignment
			 * @param db second alignment
			 * @return true iff da < db (alignments referenced by da and db, not pointers)
			 **/
			static bool compareInt(libmaus::bambam::BamAlignment const & A, libmaus::bambam::BamAlignment const & B)
			{
				return compareInt(A.D.begin(),B.D.begin());
			}

			/**
			 * compare alignment blocks by name only
			 *
			 * @param da alignment block a
			 * @param db alignment block b
			 * @return -1 if da < db, 0 if da == db, 1 if da > db by name (alignments referenced by da and db, not pointers)
			 **/
			static int compareIntNameOnly(uint8_t const * da, uint8_t const * db)
			{
				char const * namea = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(da);
				char const * nameb = ::libmaus::bambam::BamAlignmentDecoderBase::getReadName(db);

				return strcmpnum(namea,nameb);
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
				uint8_t const * da = data + a + sizeof(uint32_t);
				uint8_t const * db = data + b + sizeof(uint32_t);
				
				return compare(da,db);
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
				uint8_t const * da = data + a + sizeof(uint32_t);
				uint8_t const * db = data + b + sizeof(uint32_t);
				return compareInt(da,db);
			}

			/**
			 * compare alignments at offsets a and b in the data block by name only (ignore flags)
			 *
			 * @param a first offset into data block
			 * @param b second offset into data block
			 * @return -1 for a < b, 0 for a == b, 1 for a > b (alignments at offset a and b, not offsets and b)
			 **/
			int compareIntNameOnly(uint64_t const a, uint64_t const b) const
			{
				uint8_t const * da = data + a + sizeof(uint32_t);
				uint8_t const * db = data + b + sizeof(uint32_t);
				return compareIntNameOnly(da,db);
			}
		};
		
		struct BamAlignmentNameObject
		{
			uint8_t const * D;
			uint64_t * P;
			
			BamAlignmentNameObject(uint8_t const * rD, uint64_t * rP) : D(rD), P(rP) {}
			
			uint8_t const * operator[](uint64_t const i) const
			{
				return D + P[i] + sizeof(uint32_t);
			}
		};

		struct BamAlignmentNameProjector
		{
			typedef uint8_t const * value_type;
			
			#if defined(QUICKSORT_DEBUG)
			static value_type proj(BamAlignmentNameObject const & A, unsigned int i)
			{
				return A[i];
			}
			#endif
			
			static void swap(BamAlignmentNameObject & A, unsigned int i, unsigned int j)
			{
				std::swap(A.P[i],A.P[j]);
			}

			static bool comp(BamAlignmentNameObject & A, unsigned int i, unsigned int j)
			{
				return BamAlignmentNameComparator::compare(A[i],A[j]);
			}
		};
	}
}
#endif
