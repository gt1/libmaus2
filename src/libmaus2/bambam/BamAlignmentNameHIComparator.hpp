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
#if ! defined(LIBMAUS2_BAMBAM_BAMALIGNMENTNAMECOMPARATOR_HPP)
#define LIBMAUS2_BAMBAM_BAMALIGNMENTNAMECOMPARATOR_HPP

#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/bambam/StrCmpNum.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * comparator for BAM alignments by name. names are split up into character and number string components:
		 * name = c_0 n_0 c_1 n_1 ... . The string parts are compared lexicographically. The number parts
		 * are decoded as numbers and compared numerically. For equivalent names read 1 is smaller than read 2.
		 **/
		struct BamAlignmentNameHIComparator : public StrCmpNum, public DecoderBase
		{
			//! data pointer
			uint8_t const * data;

			/**
			 * constructor from data pointer
			 *
			 * @param rdata container data
			 **/
			BamAlignmentNameHIComparator(uint8_t const * rdata)
			: data(rdata)
			{

			}

			static uint64_t getSecSupFlags()
			{
				return
					libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSECONDARY |
					libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FSUPPLEMENTARY;
			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then
			 * da < db iff da is read 1
			 *
			 * @param da first string
			 * @param db second string
			 * @return true iff da < db (alignments referenced by da and db, not pointers)
			 **/
			static bool compare(uint8_t const * da, uint64_t const /* la */, uint8_t const * db, uint64_t const /* lb */)
			{
				char const * namea = ::libmaus2::bambam::BamAlignmentDecoderBase::getReadName(da);
				char const * nameb = ::libmaus2::bambam::BamAlignmentDecoderBase::getReadName(db);

				int const r = strcmpnum(namea,nameb);
				bool res;

				if ( r < 0 )
				{
					res = true;
				}
				else if ( r == 0 )
				{
					int64_t const HIa =
						::libmaus2::bambam::BamAlignmentDecoderBase::hasAux(da,la,"HI") ?
						::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<int64_t>(da,la,"HI") :
						std::numeric_limits<int64_t>::min();
					int64_t const HIb =
						::libmaus2::bambam::BamAlignmentDecoderBase::hasAux(db,lb,"HI") ?
						::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<int64_t>(db,lb,"HI") :
						std::numeric_limits<int64_t>::min();

					if ( HIa != HIb )
					{
						if ( HIa < HIb )
							return true;
						else
							return false;
					}

					// read 1 before read 2
					uint32_t const flagsa = ::libmaus2::bambam::BamAlignmentDecoderBase::getFlags(da);
					uint32_t const flagsb = ::libmaus2::bambam::BamAlignmentDecoderBase::getFlags(db);

					int const r1a = (flagsa & ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1) != 0;
					int const r1b = (flagsb & ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1) != 0;
					int const r2a = (flagsa & ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2) != 0;
					int const r2b = (flagsb & ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2) != 0;

					int const r1 = ((1-r1a) << 1) | (1-r2a);
					int const r2 = ((1-r1b) << 1) | (1-r2b);

					if ( r1 != r2 )
					{
						return (r1-r2) < 0;
					}
					else
					{
						// primary < secondary < supplementary
						return
							(flagsa & getSecSupFlags()) < (flagsb & getSecSupFlags());
					}
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
			static bool compare(libmaus2::bambam::BamAlignment const & A, libmaus2::bambam::BamAlignment const & B)
			{
				return compare(A.D.begin(),A.blocksize,B.D.begin(),B.blocksize);
			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then compare by read1 flag (read1 < !read1)
			 *
			 * @param da first string
			 * @param db second string
			 * @return -1 if da<db, 0 if da == db, 1 if da > db  (alignments referenced by da and db, not pointers)
			 **/
			static int compareInt(uint8_t const * da, uint64_t const la, uint8_t const * db, uint64_t const lb)
			{
				char const * namea = ::libmaus2::bambam::BamAlignmentDecoderBase::getReadName(da);
				char const * nameb = ::libmaus2::bambam::BamAlignmentDecoderBase::getReadName(db);

				int const r = strcmpnum(namea,nameb);

				if ( r != 0 )
					return r;

				int64_t const HIa =
					::libmaus2::bambam::BamAlignmentDecoderBase::hasAux(da,la,"HI") ?
					::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<int64_t>(da,la,"HI") :
					std::numeric_limits<int64_t>::min();
				int64_t const HIb =
					::libmaus2::bambam::BamAlignmentDecoderBase::hasAux(db,lb,"HI") ?
					::libmaus2::bambam::BamAlignmentDecoderBase::getAuxAsNumber<int64_t>(db,lb,"HI") :
					std::numeric_limits<int64_t>::min();

				if ( HIa != HIb )
				{
					if ( HIa < HIb )
						return -1;
					else
						return 1;
				}

				// read 1 before read 2
				uint32_t const flagsa = ::libmaus2::bambam::BamAlignmentDecoderBase::getFlags(da);
				uint32_t const flagsb = ::libmaus2::bambam::BamAlignmentDecoderBase::getFlags(db);

				int const r1a = (flagsa & ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1) != 0;
				int const r1b = (flagsb & ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1) != 0;
				int const r2a = (flagsa & ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2) != 0;
				int const r2b = (flagsb & ::libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD2) != 0;

				int const r1 = ((1-r1a) << 1) | (1-r2a);
				int const r2 = ((1-r1b) << 1) | (1-r2b);

				if ( r1 == r2 )
				{
					uint64_t const fa = flagsa & getSecSupFlags();
					uint64_t const fb = flagsb & getSecSupFlags();

					if ( fa < fb )
						return -1;
					else if ( fb < fa )
						return 1;
					else
						return 0;
				}
				else if ( (r1-r2) < 0 )
				{
					return -1;
				}
				else
				{
					return 1;
				}
			}

			/**
			 * compare alignment blocks da and db by name as described in class description. if names are equal then
			 * da < db iff da is read 1
			 *
			 * @param da first alignment
			 * @param db second alignment
			 * @return true iff da < db (alignments referenced by da and db, not pointers)
			 **/
			static bool compareInt(libmaus2::bambam::BamAlignment const & A, libmaus2::bambam::BamAlignment const & B)
			{
				return compareInt(A.D.begin(),A.blocksize,B.D.begin(),B.blocksize);
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
				return compare(da+sizeof(uint32_t),la,db+sizeof(uint32_t),lb);
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
				return compareInt(da+sizeof(uint32_t),la,db+sizeof(uint32_t),lb);
			}
		};
	}
}
#endif
