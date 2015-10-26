/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_READENDSBASE_HPP)
#define LIBMAUS2_BAMBAM_READENDSBASE_HPP

#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/math/UnsignedInteger.hpp>
#include <libmaus2/util/DigitTable.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/utf8.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct OpticalComparator;
	}
}

namespace libmaus2
{
	namespace bambam
	{
		struct ReadEndsBase;
	}
}

namespace libmaus2
{
	namespace bambam
	{
		struct ReadEnds;
	}
}

namespace libmaus2
{
	namespace bambam
	{
		std::ostream & operator<<(std::ostream & out, libmaus2::bambam::ReadEndsBase const & RE);
	}
}

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * base class for ReadEnds
		 **/
		struct ReadEndsBase
		{
			//! this type
			typedef ReadEndsBase this_type;
			//! digit table
			static ::libmaus2::util::DigitTable const D;
			
			//! friend output iterator
			friend std::ostream & ::libmaus2::bambam::operator<<(std::ostream & out, ::libmaus2::bambam::ReadEndsBase const & RE);
			//! friend comparator
			friend struct OpticalComparator;
			
			//! orientation enum
			enum read_end_orientation { F=0, R=1, FF=2, FR=3, RF=4, RR=5 };
			
			template<typename stream_type>
			uint64_t serialise(stream_type & stream) const
			{
				uint64_t l = 0;
				l += libmaus2::util::NumberSerialisation::serialiseNumber(stream,libraryId,2);
				l += libmaus2::util::NumberSerialisation::serialiseNumber(stream,tagId,8);
				l += libmaus2::util::NumberSerialisation::serialiseNumber(stream,read1Sequence,4);
				l += libmaus2::util::NumberSerialisation::serialiseNumber(stream,read1Coordinate,4);
				l += libmaus2::util::NumberSerialisation::serialiseNumber(stream,orientation,1);
				l += libmaus2::util::NumberSerialisation::serialiseNumber(stream,read2Sequence,4);
				l += libmaus2::util::NumberSerialisation::serialiseNumber(stream,read2Coordinate,4);
				l += libmaus2::util::NumberSerialisation::serialiseNumber(stream,read1IndexInFile,8);
				l += libmaus2::util::NumberSerialisation::serialiseNumber(stream,read2IndexInFile,8);
				return l;
			}
			
			template<typename stream_type>
			void deserialise(stream_type & stream)
			{
				libraryId = libmaus2::util::NumberSerialisation::deserialiseNumber(stream,2);
				tagId = libmaus2::util::NumberSerialisation::deserialiseNumber(stream,8);
				read1Sequence = libmaus2::util::NumberSerialisation::deserialiseNumber(stream,4);
				read1Coordinate = libmaus2::util::NumberSerialisation::deserialiseNumber(stream,4);
				orientation = static_cast<read_end_orientation>(libmaus2::util::NumberSerialisation::deserialiseNumber(stream,1));
				read2Sequence = libmaus2::util::NumberSerialisation::deserialiseNumber(stream,4);
				read2Coordinate = libmaus2::util::NumberSerialisation::deserialiseNumber(stream,4);
				read1IndexInFile = libmaus2::util::NumberSerialisation::deserialiseNumber(stream,8);
				read2IndexInFile = libmaus2::util::NumberSerialisation::deserialiseNumber(stream,8);
			}
			
			static uint64_t getSerialisedObjectSize()
			{
				ReadEndsBase REB;
				libmaus2::util::CountPutObject CPO;
				REB.serialise(CPO);
				return CPO.c;
			}
			
			private:
			//! library id
			uint16_t libraryId;
			//! tag id
			uint64_t tagId;
			//! sequence id for end 1
			uint32_t read1Sequence;
			//! coordinate for end 1
			uint32_t read1Coordinate;
			//! orientation
			read_end_orientation orientation;
			//! sequence id for end 2
			uint32_t read2Sequence;
			//! coordinate for end 2
			uint32_t read2Coordinate;

			//! line number of end 1
			uint64_t read1IndexInFile;
			//! line number of end 2
			uint64_t read2IndexInFile;

			//! score
			uint32_t score;
			//! read group id
			uint16_t readGroup;
			//! optical tile number
			uint16_t tile;
			//! optical x coordinate
			uint32_t x;
			//! optical y coordinate
			uint32_t y;
			
			//! shift to make signed numbers non-negative
			static int64_t const signshift = (-(1l<<29))+1;

			public:
			/**
			 * shift n to make it non-negative
			 *
			 * @param n number
			 * @return n - signshift
			 **/
			static uint32_t signedEncode(int32_t const n)
			{
				return static_cast<uint32_t>(static_cast<int64_t>(n) - signshift);
			}
			
			/**
			 * shift n generated by signedEncode back to its original value
			 *
			 * @param n number
			 * @return n + signshift
			 **/
			static int32_t signedDecode(uint32_t const n)
			{
				return static_cast<int32_t>(static_cast<int64_t>(n) + signshift);
			}

			/**
			 * @return true if object is paired (i.e. read2Sequence != 0)
			 **/
			bool isPaired() const
			{
				return read2Sequence != 0;
			}
			
			/**
			 * @return coordinates for read 1
			 **/
			std::pair<int64_t,int64_t> getCoord1() const
			{
				return std::pair<int64_t,int64_t>(read1Sequence,getRead1Coordinate());
			}
			/**
			 * @return coordinates for read 2
			 **/
			std::pair<int64_t,int64_t> getCoord2() const
			{
				return std::pair<int64_t,int64_t>(read2Sequence,getRead2Coordinate());
			}
			
			/**
			 * @return library id
			 **/
			uint16_t getLibraryId() const { return libraryId; }
			/**
			 * @return tag id
			 **/
			uint64_t getTagId() const { return tagId; }
			/**
			 * @return orientation
			 **/
			read_end_orientation getOrientation() const { return orientation; }
			/**
			 * @return sequence id for read 1
			 **/
			uint32_t getRead1Sequence() const { return read1Sequence; }
			/**
			 * @return coordinate for read 1
			 **/
			int32_t getRead1Coordinate() const { return signedDecode(read1Coordinate); }
			/**
			 * @return sequence id for read 2
			 **/
			uint32_t getRead2Sequence() const { return read2Sequence; }
			/**
			 * @return coordinate for read 2
			 **/
			int32_t getRead2Coordinate() const { return signedDecode(read2Coordinate); }
			/**
			 * @return original line number of read 1
			 **/
			uint64_t getRead1IndexInFile() const { return read1IndexInFile; }
			/**
			 * @return original line number of read 2
			 **/
			uint64_t getRead2IndexInFile() const { return read2IndexInFile; }
			/**
			 * @return score
			 **/
			uint32_t getScore() const { return score; }
			/**
			 * @return read group
			 **/
			uint16_t getReadGroup() const { return readGroup; }
			/**
			 * @return tile
			 **/
			uint16_t getTile() const { return tile; }
			/**
			 * @return x coordinate
			 **/
			uint32_t getX() const { return x; }
			/**
			 * @return y coordinate
			 **/
			uint32_t getY() const { return y; }
			
			/**
			 * constructor for invalid/empty object
			 **/
			ReadEndsBase()
			{
				reset();
			}
			
			/**
			 * reset/invalidate object
			 **/
			void reset()
			{
				memset(this,0,sizeof(*this));	
			}

			static unsigned int const hash_value_words = 7;			
			typedef libmaus2::math::UnsignedInteger<hash_value_words> hash_value_type;

			bool compareLongHashAttributes(ReadEndsBase const & O) const
			{
				return
					libraryId == O.libraryId
					&&
					tagId == O.tagId
					&&
					read1Sequence == O.read1Sequence
					&&
					read1Coordinate == O.read1Coordinate
					&&
					orientation == O.orientation
					&&
					read2Sequence == O.read2Sequence
					&&
					read2Coordinate == O.read2Coordinate;
			}

			bool compareLongHashAttributesSmaller(ReadEndsBase const & O) const
			{
				if ( libraryId != O.libraryId )
					return libraryId < O.libraryId;
				else if ( tagId != O.tagId )
					return tagId < O.tagId;
				else if ( read1Sequence != O.read1Sequence )
					return read1Sequence < O.read1Sequence;
				else if ( read1Coordinate != O.read1Coordinate )
					return read1Coordinate < O.read1Coordinate;
				else if ( orientation != O.orientation )
					return orientation < O.orientation;
				else if ( read2Sequence != O.read2Sequence )
					return read2Sequence < O.read2Sequence;
				else
					return read2Coordinate < O.read2Coordinate;
			}
			
			hash_value_type encodeLongHash() const
			{
				hash_value_type H;
				H <<= 16; H |= hash_value_type(libraryId);
				H <<= 64; H |= hash_value_type(tagId);
				H <<= 32; H |= hash_value_type(read1Sequence);
				H <<= 32; H |= hash_value_type(read1Coordinate);
				H <<=  8; H |= hash_value_type(orientation);
				H <<= 32; H |= hash_value_type(read2Sequence);
				H <<= 32; H |= hash_value_type(read2Coordinate);
				return H;
			}
			
			void decodeLongHash(hash_value_type H)
			{
				read2Coordinate = (H[0] & 0xFFFFFFFFUL); H >>= 32;
				read2Sequence   = (H[0] & 0xFFFFFFFFUL); H >>= 32;
				orientation     = static_cast<read_end_orientation>(H[0] & 0xFF); H >>= 8;
				read1Coordinate = (H[0] & 0xFFFFFFFFUL); H >>= 32;
				read1Sequence   = (H[0] & 0xFFFFFFFFUL); H >>= 32;
				tagId           = (static_cast<uint64_t>(H[1]) << 32) | (static_cast<uint64_t>(H[0]) <<  0); H >>= 64;
				libraryId       = (H[0] & 0xFFFF); H >>= 8;
			}

			bool compareShortHashAttributes(ReadEndsBase const & O) const
			{
				return
					libraryId == O.libraryId
					&&
					tagId == O.tagId
					&&
					read1Sequence == O.read1Sequence
					&&
					read1Coordinate == O.read1Coordinate
					&&
					orientation == O.orientation;
			}

			bool compareShortHashAttributesSmaller(ReadEndsBase const & O) const
			{
				if ( libraryId != O.libraryId )
					return libraryId < O.libraryId;
				else if ( tagId != O.tagId )
					return tagId < O.tagId;
				else if ( read1Sequence != O.read1Sequence )
					return read1Sequence < O.read1Sequence;
				else if ( read1Coordinate != O.read1Coordinate )
					return read1Coordinate < O.read1Coordinate;
				else
					return orientation < O.orientation;
			}
			
			hash_value_type encodeShortHash() const
			{
				hash_value_type H;
				H <<= 16; H |= hash_value_type(libraryId);
				H <<= 64; H |= hash_value_type(tagId);
				H <<= 32; H |= hash_value_type(read1Sequence);
				H <<= 32; H |= hash_value_type(read1Coordinate);
				H <<=  8; H |= hash_value_type(orientation);
				return H;
			}
			
			void decodeShortHash(hash_value_type H)
			{
				orientation     = static_cast<read_end_orientation>(H[0] & 0xFF); H >>= 8;
				read1Coordinate = (H[0] & 0xFFFFFFFFUL); H >>= 32;
				read1Sequence   = (H[0] & 0xFFFFFFFFUL); H >>= 32;
				tagId           = (static_cast<uint64_t>(H[1]) << 32) | (static_cast<uint64_t>(H[0]) <<  0); H >>= 64;
				libraryId       = (H[0] & 0xFFFF); H >>= 8;
			}
			
			/**
			 * comparator smaller
			 *
			 * @param o other ReadEndsBase object
			 * @result *this < o lexicographically along the attributes as ordered in memory (see above)
			 **/
			bool operator<(ReadEndsBase const & o) const 
			{
				if ( libraryId != o.libraryId ) return libraryId < o.libraryId;
				if ( tagId != o.tagId ) return tagId < o.tagId;
				if ( read1Sequence != o.read1Sequence ) return read1Sequence < o.read1Sequence;
				if ( read1Coordinate != o.read1Coordinate ) return read1Coordinate < o.read1Coordinate;
				if ( orientation != o.orientation ) return orientation < o.orientation;
				if ( read2Sequence != o.read2Sequence ) return read2Sequence < o.read2Sequence;
				if ( read2Coordinate != o.read2Coordinate ) return read2Coordinate < o.read2Coordinate;
				if ( read1IndexInFile != o.read1IndexInFile ) return read1IndexInFile < o.read1IndexInFile;
				if ( read2IndexInFile != o.read2IndexInFile ) return read2IndexInFile < o.read2IndexInFile;

				return false;
			}

			/**
			 * comparator greater
			 *
			 * @param o other ReadEndsBase object
			 * @result *this > o lexicographically along the attributes as ordered in memory (see above)
			 **/
			bool operator>(ReadEndsBase const & o) const 
			{
				if ( libraryId != o.libraryId ) return libraryId > o.libraryId;
				if ( tagId != o.tagId ) return tagId > o.tagId;
				if ( read1Sequence != o.read1Sequence ) return read1Sequence > o.read1Sequence;
				if ( read1Coordinate != o.read1Coordinate ) return read1Coordinate > o.read1Coordinate;
				if ( orientation != o.orientation ) return orientation > o.orientation;
				if ( read2Sequence != o.read2Sequence ) return read2Sequence > o.read2Sequence;
				if ( read2Coordinate != o.read2Coordinate ) return read2Coordinate > o.read2Coordinate;
				if ( read1IndexInFile != o.read1IndexInFile ) return read1IndexInFile > o.read1IndexInFile;
				if ( read2IndexInFile != o.read2IndexInFile ) return read2IndexInFile > o.read2IndexInFile;

				return false;
			}
			
			/**
			 * comparator equals
			 *
			 * @param o other ReadEndsBase object
			 * @result *this == o lexicographically along the attributes as ordered in memory (see above) up to the line numbers
			 **/
			bool operator==(ReadEndsBase const & o) const
			{
				if ( libraryId != o.libraryId ) return false;
				if ( tagId != o.tagId ) return false;
				if ( read1Sequence != o.read1Sequence ) return false;
				if ( read1Coordinate != o.read1Coordinate ) return false;
				if ( orientation != o.orientation ) return false;
				if ( read2Sequence != o.read2Sequence ) return false;
				if ( read2Coordinate != o.read2Coordinate ) return false;
				if ( read1IndexInFile != o.read1IndexInFile ) return false;
				if ( read2IndexInFile != o.read2IndexInFile ) return false;
				
				return true;
			}

			/**
			 * comparator not equals
			 *
			 * @param o other ReadEndsBase object
			 * @result *this != o lexicographically along the attributes as ordered in memory (see above) up to the line numbers
			 **/
			bool operator!=(ReadEndsBase const & o) const
			{
				return ! (*this == o);
			}

			/**
			 * parse optical information (if any) into tile, x and y
			 **/
			static bool parseOptical(uint8_t const * readname, uint16_t & tile, uint32_t & x, uint32_t & y)
			{
				size_t const l = strlen(reinterpret_cast<char const *>(readname));
				
				if ( parseReadNameValid(readname,readname+l) )
				{
					parseReadNameTile(readname,readname+l,tile,x,y);
					return tile != 0;
				}
				else
				{
					return false;
				}
			}

			private:
			/**
			 * determine if readname contains optical parameters
			 *
			 * @param readname name start pointer
			 * @param readnamee name end pointer
			 * @return true iff name has optical fields (number of ":" in name is >= 2 and <= 4)
			 **/
			static bool parseReadNameValid(uint8_t const * readname, uint8_t const * readnamee)
			{
				int cnt[2] = { 0,0 };

				for ( uint8_t const * c = readname; c != readnamee; ++c )
					cnt [ (static_cast<int>(*c) - ':') == 0 ] ++;

				bool const rnparseok = (cnt[1] <= 4) && (cnt[1] >= 2);
				
				return rnparseok;
			}

			/**
			 * parse optical parameters from read name
			 *
			 * assumes tile, x and y are separated by the last 2 ":" in the read name
			 *
			 * @param readname name start pointer
			 * @param readnamee name end pointer
			 * @param RE ReadEndsBase object to be filled
			 **/
			static void parseReadNameTile(uint8_t const * readname, uint8_t const * readnamee, ::libmaus2::bambam::ReadEndsBase & RE)
			{
				uint8_t const * sem[4];
				sem[2] = readname;
				uint8_t const ** psem = &sem[0];
				uint8_t const * c = readnamee;
				for ( --c; c >= readname; --c )
					if ( *c == ':' )
						*(psem++) = c+1;
				
				uint8_t const * t = sem[2];
				RE.tile = 0;
				while ( D[*t] )
				{
					RE.tile *= 10;
					RE.tile += *(t++)-'0';
				}
				RE.tile += 1;

				t = sem[1];
				RE.x = 0;
				while ( D[*t] )
				{
					RE.x *= 10;
					RE.x += *(t++)-'0';
				}

				t = sem[0];
				RE.y = 0;
				while ( D[*t] )
				{
					RE.y *= 10;
					RE.y += *(t++)-'0';
				}			
			}

			/**
			 * parse optical parameters from read name
			 *
			 * assumes tile, x and y are separated by the last 2 ":" in the read name
			 *
			 * @param readname name start pointer
			 * @param readnamee name end pointer
			 * @param RE ReadEndsBase object to be filled
			 **/
			static void parseReadNameTile(
				uint8_t const * readname, 
				uint8_t const * readnamee, 
				uint16_t & tile,
				uint32_t & x,
				uint32_t & y
			)
			{
				uint8_t const * sem[4];
				sem[2] = readname;
				uint8_t const ** psem = &sem[0];
				uint8_t const * c = readnamee;
				for ( --c; c >= readname; --c )
					if ( *c == ':' )
						*(psem++) = c+1;
				
				uint8_t const * t = sem[2];
				while ( D[*t] )
				{
					tile *= 10;
					tile += *(t++)-'0';
				}
				tile += 1;

				t = sem[1];
				while ( D[*t] )
				{
					x *= 10;
					x += *(t++)-'0';
				}

				t = sem[0];
				while ( D[*t] )
				{
					y *= 10;
					y += *(t++)-'0';
				}			
			}
			
			/**
			 * fill common parts between fragment and pair ReadEndsBase objects
			 *
			 * @param p alignment
			 * @param RE ReadEndsBase object to be filled
			 **/
			static void fillCommon(
				::libmaus2::bambam::BamAlignment const & p, 
				::libmaus2::bambam::ReadEndsBase & RE
			)
			{
				RE.read1Sequence = p.getRefIDChecked() + 1;
				RE.read1Coordinate = signedEncode(p.getCoordinate() + 1);
				RE.read1IndexInFile = p.getRank();

				uint8_t const * const readname = reinterpret_cast<uint8_t const *>(p.getName());
				uint8_t const * const readnamee = readname + (p.getLReadName()-1);
				
				// parse tile, x, y
				if ( parseReadNameValid(readname,readnamee) )
					parseReadNameTile(readname,readnamee,RE);			
			}

			/**
			 * fill common parts between fragment and pair ReadEndsBase objects
			 *
			 * @param pD alignment block
			 * @param pblocksize alignment block length
			 * @param RE ReadEndsBase object to be filled
			 **/
			static void fillCommon(
				uint8_t const * pD, 
				uint64_t const pblocksize,
				::libmaus2::bambam::ReadEndsBase & RE
			)
			{
				RE.read1Sequence = libmaus2::bambam::BamAlignmentDecoderBase::getRefIDChecked(pD) + 1;
				RE.read1Coordinate = signedEncode(libmaus2::bambam::BamAlignmentDecoderBase::getCoordinate(pD) + 1);
				RE.read1IndexInFile = libmaus2::bambam::BamAlignmentDecoderBase::getRank(pD,pblocksize);

				uint8_t const * const readname = reinterpret_cast<uint8_t const *>(libmaus2::bambam::BamAlignmentDecoderBase::getReadName(pD));
				uint8_t const * const readnamee = readname + (libmaus2::bambam::BamAlignmentDecoderBase::getLReadName(pD)-1);

				// parse tile, x, y
				if ( parseReadNameValid(readname,readnamee) )
					parseReadNameTile(readname,readnamee,RE);			
			}

			public:
			/**
			 * fill fragment type ReadEndsBase object
			 *
			 * @param p alignment
			 * @param header BAM header
			 * @param RE ReadEndsBase object to be filled
			 * @param tagId tag id for object
			 **/
			template<typename header_type>
			static void fillFrag(
				uint8_t const * pD, 
				uint64_t const pblocksize,
				header_type const & header,
				::libmaus2::bambam::ReadEndsBase & RE,
				uint64_t const rtagid = 0
			)
			{
				fillCommon(pD,pblocksize,RE);
				
				uint32_t const pflags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(pD);
				
				RE.orientation =
					(pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE)
					?
					::libmaus2::bambam::ReadEndsBase::R : ::libmaus2::bambam::ReadEndsBase::F;

				RE.score = libmaus2::bambam::BamAlignmentDecoderBase::getScore(pD);
				
				if ( 
					(pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FPAIRED) &&
					(!(pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMUNMAP))
				)
					RE.read2Sequence = libmaus2::bambam::BamAlignmentDecoderBase::getNextRefIDChecked(pD) + 1;
					
				int64_t const rg = libmaus2::bambam::BamAlignmentDecoderBase::getReadGroupId(pD,pblocksize,header);
				RE.readGroup = rg + 1;
				RE.libraryId = header.getLibraryId(rg);
				RE.tagId = rtagid;
			}

			/**
			 * fill fragment type ReadEndsBase object
			 *
			 * @param p alignment
			 * @param header BAM header
			 * @param RE ReadEndsBase object to be filled
			 * @param tagId tag id for object
			 **/
			template<typename header_type>
			static void fillFrag(
				::libmaus2::bambam::BamAlignment const & p, 
				header_type const & header,
				::libmaus2::bambam::ReadEndsBase & RE,
				uint64_t const rtagid = 0
			)
			{
				fillCommon(p,RE);

				RE.orientation = p.isReverse() ? ::libmaus2::bambam::ReadEndsBase::R : ::libmaus2::bambam::ReadEndsBase::F;

				RE.score = p.getScore();
				
				if ( p.isPaired() && (!p.isMateUnmap()) )
					RE.read2Sequence = p.getNextRefIDChecked() + 1;
					
				int64_t const rg = p.getReadGroupId(header);
				RE.readGroup = rg + 1;
				RE.libraryId = header.getLibraryId(rg);
				RE.tagId = rtagid;
			}

			/**
			 * check whether alignment is left one of a pair
			 **/
			static bool isLeft(uint8_t const * pD, uint64_t const blocksize, libmaus2::autoarray::AutoArray<cigar_operation> & Aop)
			{
				// check for ref id order
				int32_t const prefid = libmaus2::bambam::BamAlignmentDecoderBase::getRefID(pD);
				int32_t const qrefid = libmaus2::bambam::BamAlignmentDecoderBase::getNextRefID(pD);

				if ( prefid != qrefid )
					return prefid < qrefid;

				// get flags
				uint32_t const pflags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(pD);

				// extract reverse bit
				bool const preverse = pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE;
				bool const qreverse = pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMREVERSE;

				size_t const numcigop = libmaus2::bambam::BamAlignmentDecoderBase::getNextCigarVector(pD,blocksize,Aop);

				// are the reads mapped to different strands?
				if ( preverse != qreverse )
				{
					// q is on reverse
					if ( qreverse )
					{
						int32_t const pleftmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedStart(pD);
						int32_t const qrightmost = libmaus2::bambam::BamAlignmentDecoderBase::getNextUnclippedEnd(pD,Aop.begin(),Aop.begin()+numcigop);
						return pleftmost <= qrightmost;
					}
					// p is on reverse
					else
					{
						int32_t const prightmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedEnd(pD);
						int32_t const qleftmost = libmaus2::bambam::BamAlignmentDecoderBase::getNextUnclippedStart(pD,Aop.begin(),Aop.begin()+numcigop);
						return prightmost <= qleftmost;
					}
				}
				// reads are on the same strand
				else
				{
					if ( qreverse )
					{
						int32_t const prightmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedEnd(pD);
						int32_t const qrightmost = libmaus2::bambam::BamAlignmentDecoderBase::getNextUnclippedEnd(pD,Aop.begin(),Aop.begin()+numcigop);
						return prightmost <= qrightmost;
					}
					// both on forward
					else
					{
						int32_t const pleftmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedStart(pD);
						int32_t const qleftmost = libmaus2::bambam::BamAlignmentDecoderBase::getNextUnclippedStart(pD,Aop.begin(),Aop.begin()+numcigop);
						return pleftmost <= qleftmost;
					}
				}
			}

			/**
			 * check whether order of two fragments is ok (return true) or needs to be swapped (return false)
			 **/
			static bool orderOK(uint8_t const * pD, uint8_t const * qD)
			{
				// check for ref id order
				int32_t const prefid = libmaus2::bambam::BamAlignmentDecoderBase::getRefID(pD);
				int32_t const qrefid = libmaus2::bambam::BamAlignmentDecoderBase::getRefID(qD);

				if ( prefid != qrefid )
					return prefid < qrefid;

				// get flags
				uint32_t const pflags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(pD);
				uint32_t const qflags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(qD);

				// extract reverse bit
				bool const preverse = pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE;
				bool const qreverse = qflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE;

				// are the reads mapped to different strands?
				if ( preverse != qreverse )
				{
					// q is on reverse
					if ( qreverse )
					{
						int32_t const pleftmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedStart(pD);
						int32_t const qrightmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedEnd(qD);
						return pleftmost <= qrightmost;
					}
					// p is on reverse
					else
					{
						int32_t const prightmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedEnd(pD);
						int32_t const qleftmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedStart(qD);
						return prightmost <= qleftmost;
					}
				}
				// reads are on the same strand
				else
				{
					if ( qreverse )
					{
						int32_t const prightmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedEnd(pD);
						int32_t const qrightmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedEnd(qD);
						return prightmost <= qrightmost;
					}
					// both on forward
					else
					{
						int32_t const pleftmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedStart(pD);
						int32_t const qleftmost = libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedStart(qD);
						return pleftmost <= qleftmost;
					}
				}
			}

			/**
			 * check whether order of two fragments is ok (return true) or needs to be swapped (return false)
			 **/
			static bool orderOK(libmaus2::bambam::BamAlignment const & A, libmaus2::bambam::BamAlignment const & B)
			{
				return orderOK(A.D.begin(),B.D.begin());
			}

			/**
			 * compute orientation of a pair (which needs to be in correct order as given by orderOK)
			 **/
			static read_end_orientation computePairOrientation(
				uint32_t const pflags,
				uint32_t const qflags
			)
			{
				bool const preverse = pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE;
				bool const qreverse = qflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE;

				read_end_orientation orientation;

				if ( preverse != qreverse )
				{
					if ( ! preverse )
						orientation = ::libmaus2::bambam::ReadEndsBase::FR;
					else
						orientation = ::libmaus2::bambam::ReadEndsBase::RF;
				}
				else
				{
					bool const pisread1 = pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREAD1;

					if ( ! preverse )
					{
						if ( pisread1 )
							orientation = ::libmaus2::bambam::ReadEndsBase::FF;
						else
							orientation = ::libmaus2::bambam::ReadEndsBase::RR;
					}
					else
					{
						if ( pisread1 )
							orientation = ::libmaus2::bambam::ReadEndsBase::RR;
						else
							orientation = ::libmaus2::bambam::ReadEndsBase::FF;
					}
				}

				return orientation;
			}

			static void checkSameStrandCoordinates(
				uint8_t const * pD,
				uint32_t const pflags,
				uint8_t const * qD,
				::libmaus2::bambam::ReadEndsBase & RE
			)
			{
				switch ( RE.orientation )
				{
					case FF:
					case RR:
					{
						bool const flagreverse = pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FREVERSE;

						if ( flagreverse )
						{
							// suspected start of sequenced molecule
							RE.read1Coordinate = signedEncode(libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedStart(pD) + 1);
						}
						else
						{
							// suspected end of sequenced molecule
							RE.read2Coordinate = signedEncode(libmaus2::bambam::BamAlignmentDecoderBase::getUnclippedEnd(qD) + 1);
						}

						break;
					}
					default:
					{
						break;
					}
				}
			}

			/**
			 * fill pair type ReadEndsBase object
			 *
			 * @param pD first alignment block
			 * @param qD second alignment block
			 * @param header BAM header
			 * @param RE ReadEndsBase object to be filled
			 **/
			template<typename header_type>
			static void fillFragPair(
				uint8_t const * pD, 
				uint64_t const pblocksize,
				uint8_t const * qD, 
				uint64_t const qblocksize,
				header_type const & header,
				::libmaus2::bambam::ReadEndsBase & RE,
				uint64_t const rtagId = 0
			)
			{
				uint32_t const pflags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(pD);
				uint32_t const qflags = libmaus2::bambam::BamAlignmentDecoderBase::getFlags(qD);
				RE.orientation = computePairOrientation(pflags,qflags);

				fillCommon(pD,pblocksize,RE);

				RE.read2Sequence = libmaus2::bambam::BamAlignmentDecoderBase::getRefIDChecked(qD) + 1;
				RE.read2Coordinate = signedEncode(libmaus2::bambam::BamAlignmentDecoderBase::getCoordinate(qD) + 1);
				RE.read2IndexInFile = libmaus2::bambam::BamAlignmentDecoderBase::getRank(qD,qblocksize);

				RE.score = libmaus2::bambam::BamAlignmentDecoderBase::getScore(pD) + libmaus2::bambam::BamAlignmentDecoderBase::getScore(qD);
				
				if ( 
					(pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FPAIRED) 
					&&
					(!(pflags & libmaus2::bambam::BamFlagBase::LIBMAUS2_BAMBAM_FMUNMAP))
				)
					RE.read2Sequence = libmaus2::bambam::BamAlignmentDecoderBase::getNextRefIDChecked(pD) + 1;
				
				int64_t const rg = libmaus2::bambam::BamAlignmentDecoderBase::getReadGroupId(pD,pblocksize,header);
								
				RE.readGroup = rg + 1;
				RE.libraryId = header.getLibraryId(rg);
				RE.tagId = rtagId;

				checkSameStrandCoordinates(pD,pflags,qD,RE);
			}

			/**
			 * fill pair type ReadEndsBase object
			 *
			 * @param p first alignment
			 * @param q second alignment
			 * @param header BAM header
			 * @param RE ReadEndsBase object to be filled
			 **/
			template<typename header_type>
			static void fillFragPair(
				::libmaus2::bambam::BamAlignment const & p, 
				::libmaus2::bambam::BamAlignment const & q, 
				header_type const & header,
				::libmaus2::bambam::ReadEndsBase & RE,
				uint64_t const rtagId = 0
			)
			{
				fillFragPair(
					p.D.begin(),
					p.blocksize,
					q.D.begin(),
					q.blocksize,
					header,
					RE,
					rtagId
				);
			}

			#define READENDSBASECOMPACT

			/**
			 * decode ReadEndsBase object from compacted form
			 *
			 * @param G input stream
			 **/
			template<typename get_type>
			void get(get_type & G)
			{
				#if defined(READENDSBASECOMPACT)
				this->libraryId = ::libmaus2::util::UTF8::decodeUTF8Unchecked(G);
				this->tagId = ::libmaus2::util::NumberSerialisation::deserialiseNumber(G);
				this->read1Sequence = ::libmaus2::util::UTF8::decodeUTF8Unchecked(G);
				this->read1Coordinate = ::libmaus2::util::UTF8::decodeUTF8Unchecked(G);
				this->orientation = static_cast<read_end_orientation>(G.get());
				
				this->read2Sequence = ::libmaus2::util::UTF8::decodeUTF8Unchecked(G);
				this->read2Coordinate = ::libmaus2::util::UTF8::decodeUTF8Unchecked(G);
				
				this->read1IndexInFile = ::libmaus2::util::NumberSerialisation::deserialiseNumber(G);
				this->read2IndexInFile = ::libmaus2::util::NumberSerialisation::deserialiseNumber(G);

				this->score = ::libmaus2::util::UTF8::decodeUTF8Unchecked(G);
				this->readGroup = ::libmaus2::util::UTF8::decodeUTF8Unchecked(G);
				
				this->tile = ::libmaus2::util::UTF8::decodeUTF8Unchecked(G);

				this->x = ::libmaus2::util::NumberSerialisation::deserialiseNumber(G,4);
				this->y = ::libmaus2::util::NumberSerialisation::deserialiseNumber(G,4);
				#else
				G.read(reinterpret_cast<char *>(this),sizeof(*this));
				#endif
			}

			/**
			 * encode ReadEndsBase object to compacted form
			 *
			 * @param P output stream
			 **/
			template<typename put_type>
			void put(put_type & P) const
			{
				#if defined(READENDSBASECOMPACT)
				::libmaus2::util::UTF8::encodeUTF8(this->libraryId,P);

				::libmaus2::util::NumberSerialisation::serialiseNumber(P,this->tagId);

				::libmaus2::util::UTF8::encodeUTF8(this->read1Sequence,P);
				::libmaus2::util::UTF8::encodeUTF8(this->read1Coordinate,P);
				P.put(static_cast<uint8_t>(this->orientation));

				::libmaus2::util::UTF8::encodeUTF8(this->read2Sequence,P);
				::libmaus2::util::UTF8::encodeUTF8(this->read2Coordinate,P);

				::libmaus2::util::NumberSerialisation::serialiseNumber(P,this->read1IndexInFile);
				::libmaus2::util::NumberSerialisation::serialiseNumber(P,this->read2IndexInFile);
				
				::libmaus2::util::UTF8::encodeUTF8(this->score,P);
				::libmaus2::util::UTF8::encodeUTF8(this->readGroup,P);
				
				::libmaus2::util::UTF8::encodeUTF8(this->tile,P);

				::libmaus2::util::NumberSerialisation::serialiseNumber(P,this->x,4);
				::libmaus2::util::NumberSerialisation::serialiseNumber(P,this->y,4);					
				#else
				P.write(reinterpret_cast<char const *>(this),sizeof(*this));				
				#endif
			}
		};
	}
}
#endif
