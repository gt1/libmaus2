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
#if ! defined(LIBMAUS_BAMBAM_READENDS_HPP)
#define LIBMAUS_BAMBAM_READENDS_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/util/utf8.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/util/DigitTable.hpp>
#include <map>
#include <cstring>
#include <sstream>

namespace libmaus
{
	namespace bambam
	{
		struct ReadEnds;
		std::ostream & operator<<(std::ostream & out, ReadEnds const & RE);
		struct OpticalComparator;

		struct ReadEndsBase
		{
			typedef ReadEndsBase this_type;
			static ::libmaus::util::DigitTable const D;
			
			friend std::ostream & operator<<(std::ostream & out, ReadEnds const & RE);
			friend struct OpticalComparator;
			
			enum read_end_orientation { F=0, R=1, FF=2, FR=3, RF=4, RR=5 };
			
			private:
			uint16_t libraryId;
			uint32_t read1Sequence;
			uint32_t read1Coordinate;
			read_end_orientation orientation;
			uint32_t read2Sequence;
			uint32_t read2Coordinate;

			uint64_t read1IndexInFile;
			uint64_t read2IndexInFile;

			uint32_t score;
			uint16_t readGroup;
			uint16_t tile;
			uint32_t x;
			uint32_t y;
			
			static int64_t const signshift = (-(1l<<29))+1;

			static uint32_t signedEncode(int32_t const n)
			{
				return static_cast<uint32_t>(static_cast<int64_t>(n) - signshift);
			}
			
			static int32_t signedDecode(uint32_t const n)
			{
				return static_cast<int32_t>(static_cast<int64_t>(n) + signshift);
			}
			public:
			bool isPaired() const
			{
				return read2Sequence != 0;
			}
			
			std::pair<int64_t,int64_t> getCoord1() const
			{
				return std::pair<int64_t,int64_t>(read1Sequence,getRead1Coordinate());
			}
			std::pair<int64_t,int64_t> getCoord2() const
			{
				return std::pair<int64_t,int64_t>(read2Sequence,getRead2Coordinate());
			}
			
			uint16_t getLibraryId() const { return libraryId; }
			read_end_orientation getOrientation() const { return orientation; }
			uint32_t getRead1Sequence() const { return read1Sequence; }
			int32_t getRead1Coordinate() const { return signedDecode(read1Coordinate); }
			uint32_t getRead2Sequence() const { return read2Sequence; }
			int32_t getRead2Coordinate() const { return signedDecode(read2Coordinate); }
			uint64_t getRead1IndexInFile() const { return read1IndexInFile; }
			uint64_t getRead2IndexInFile() const { return read2IndexInFile; }
			uint32_t getScore() const { return score; }
			uint16_t getReadGroup() const { return readGroup; }
			uint16_t getTile() const { return tile; }
			uint32_t getX() const { return x; }
			uint32_t getY() const { return y; }
			
			ReadEndsBase()
			{
				reset();
			}
			
			void reset()
			{
				memset(this,0,sizeof(*this));	
			}
			
			bool operator<(ReadEndsBase const & o) const 
			{
				if ( libraryId != o.libraryId ) return libraryId < o.libraryId;
				if ( read1Sequence != o.read1Sequence ) return read1Sequence < o.read1Sequence;
				if ( read1Coordinate != o.read1Coordinate ) return read1Coordinate < o.read1Coordinate;
				if ( orientation != o.orientation ) return orientation < o.orientation;
				if ( read2Sequence != o.read2Sequence ) return read2Sequence < o.read2Sequence;
				if ( read2Coordinate != o.read2Coordinate ) return read2Coordinate < o.read2Coordinate;
				if ( read1IndexInFile != o.read1IndexInFile ) return read1IndexInFile < o.read1IndexInFile;
				if ( read2IndexInFile != o.read2IndexInFile ) return read2IndexInFile < o.read2IndexInFile;

				return false;
			}
			bool operator>(ReadEndsBase const & o) const 
			{
				if ( libraryId != o.libraryId ) return libraryId > o.libraryId;
				if ( read1Sequence != o.read1Sequence ) return read1Sequence > o.read1Sequence;
				if ( read1Coordinate != o.read1Coordinate ) return read1Coordinate > o.read1Coordinate;
				if ( orientation != o.orientation ) return orientation > o.orientation;
				if ( read2Sequence != o.read2Sequence ) return read2Sequence > o.read2Sequence;
				if ( read2Coordinate != o.read2Coordinate ) return read2Coordinate > o.read2Coordinate;
				if ( read1IndexInFile != o.read1IndexInFile ) return read1IndexInFile > o.read1IndexInFile;
				if ( read2IndexInFile != o.read2IndexInFile ) return read2IndexInFile > o.read2IndexInFile;

				return false;
			}
			
			bool operator==(ReadEndsBase const & o) const
			{
				if ( libraryId != o.libraryId ) return false;
				if ( read1Sequence != o.read1Sequence ) return false;
				if ( read1Coordinate != o.read1Coordinate ) return false;
				if ( orientation != o.orientation ) return false;
				if ( read2Sequence != o.read2Sequence ) return false;
				if ( read2Coordinate != o.read2Coordinate ) return false;
				if ( read1IndexInFile != o.read1IndexInFile ) return false;
				if ( read2IndexInFile != o.read2IndexInFile ) return false;
				
				return true;
			}
			bool operator!=(ReadEndsBase const & o) const
			{
				return ! (*this == o);
			}

			static bool parseReadNameValid(uint8_t const * readname, uint8_t const * readnamee)
			{
				int cnt[2] = { 0,0 };

				for ( uint8_t const * c = readname; c != readnamee; ++c )
					cnt [ (static_cast<int>(*c) - ':') == 0 ] ++;

				bool const rnparseok = (cnt[1] == 4);
				
				return rnparseok;
			}

			static void parseReadNameTile(uint8_t const * readname, uint8_t const * readnamee, ::libmaus::bambam::ReadEndsBase & RE)
			{
				uint8_t const * sem[4];
				uint8_t const ** psem = &sem[0];
				for ( uint8_t const * c = readname; c != readnamee; ++c )
					if ( *c == ':' )
						*(psem++) = c+1;
				
				uint8_t const * t = sem[1];
				while ( D[*t] )
				{
					RE.tile *= 10;
					RE.tile += *(t++)-'0';
				}
				RE.tile += 1;

				t = sem[2];
				while ( D[*t] )
				{
					RE.x *= 10;
					RE.x += *(t++)-'0';
				}

				t = sem[3];
				while ( D[*t] )
				{
					RE.y *= 10;
					RE.y += *(t++)-'0';
				}			
			}
			
			static void fillCommon(
				::libmaus::bambam::BamAlignment const & p, 
				::libmaus::bambam::ReadEndsBase & RE
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

			static void fillFrag(
				::libmaus::bambam::BamAlignment const & p, 
				::libmaus::bambam::BamHeader const & header,
				::libmaus::bambam::ReadEndsBase & RE
			)
			{
				fillCommon(p,RE);
				
				RE.orientation = p.isReverse() ? ::libmaus::bambam::ReadEndsBase::R : ::libmaus::bambam::ReadEndsBase::F;

				RE.score = p.getScore();
				
				if ( p.isPaired() && (!p.isMateUnmap()) )
					RE.read2Sequence = p.getNextRefIDChecked() + 1;
					
				int64_t const rg = p.getReadGroupId(header);
				RE.readGroup = rg + 1;
				RE.libraryId = header.getLibraryId(rg);
			}

			static void fillFragPair(
				::libmaus::bambam::BamAlignment const & p, 
				::libmaus::bambam::BamAlignment const & q, 
				::libmaus::bambam::BamHeader const & header,
				::libmaus::bambam::ReadEndsBase & RE
			)
			{
				fillCommon(p,RE);

				RE.read2Sequence = q.getRefIDChecked() + 1;
				RE.read2Coordinate = signedEncode(q.getCoordinate() + 1);
				RE.read2IndexInFile = q.getRank();
				
				if ( ! p.isReverse() )
					if ( ! q.isReverse() )
						RE.orientation = ::libmaus::bambam::ReadEndsBase::FF;
					else
						RE.orientation = ::libmaus::bambam::ReadEndsBase::FR;
				else
					if ( ! q.isReverse() )
						RE.orientation = ::libmaus::bambam::ReadEndsBase::RF;
					else
						RE.orientation = ::libmaus::bambam::ReadEndsBase::RR;
				
				
				RE.score = p.getScore() + q.getScore();
				
				if ( p.isPaired() && (!p.isMateUnmap()) )
					RE.read2Sequence = p.getNextRefIDChecked() + 1;
				
				int64_t const rg = p.getReadGroupId(header);
								
				RE.readGroup = rg + 1;
			}
			
			#define READENDSBASECOMPACT

			template<typename get_type>
			void get(get_type & G)
			{
				#if defined(READENDSBASECOMPACT)
				this->libraryId = ::libmaus::util::UTF8::decodeUTF8Unchecked(G);
				this->read1Sequence = ::libmaus::util::UTF8::decodeUTF8Unchecked(G);
				this->read1Coordinate = ::libmaus::util::UTF8::decodeUTF8Unchecked(G);
				this->orientation = static_cast<read_end_orientation>(G.get());
				
				this->read2Sequence = ::libmaus::util::UTF8::decodeUTF8Unchecked(G);
				this->read2Coordinate = ::libmaus::util::UTF8::decodeUTF8Unchecked(G);
				
				this->read1IndexInFile = ::libmaus::util::NumberSerialisation::deserialiseNumber(G);
				this->read2IndexInFile = ::libmaus::util::NumberSerialisation::deserialiseNumber(G);

				this->score = ::libmaus::util::UTF8::decodeUTF8Unchecked(G);
				this->readGroup = ::libmaus::util::UTF8::decodeUTF8Unchecked(G);
				
				this->tile = ::libmaus::util::UTF8::decodeUTF8Unchecked(G);

				this->x = ::libmaus::util::NumberSerialisation::deserialiseNumber(G,2);
				this->y = ::libmaus::util::NumberSerialisation::deserialiseNumber(G,2);
				#else
				G.read(reinterpret_cast<char *>(this),sizeof(*this));
				#endif
			}

			template<typename put_type>
			void put(put_type & P) const
			{
				#if defined(READENDSBASECOMPACT)
				::libmaus::util::UTF8::encodeUTF8(this->libraryId,P);

				::libmaus::util::UTF8::encodeUTF8(this->read1Sequence,P);
				::libmaus::util::UTF8::encodeUTF8(this->read1Coordinate,P);
				P.put(static_cast<uint8_t>(this->orientation));

				::libmaus::util::UTF8::encodeUTF8(this->read2Sequence,P);
				::libmaus::util::UTF8::encodeUTF8(this->read2Coordinate,P);

				::libmaus::util::NumberSerialisation::serialiseNumber(P,this->read1IndexInFile);
				::libmaus::util::NumberSerialisation::serialiseNumber(P,this->read2IndexInFile);
				
				::libmaus::util::UTF8::encodeUTF8(this->score,P);
				::libmaus::util::UTF8::encodeUTF8(this->readGroup,P);
				
				::libmaus::util::UTF8::encodeUTF8(this->tile,P);

				::libmaus::util::NumberSerialisation::serialiseNumber(P,this->x,2);
				::libmaus::util::NumberSerialisation::serialiseNumber(P,this->y,2);					
				#else
				P.write(reinterpret_cast<char const *>(this),sizeof(*this));				
				#endif
			}
		};

		struct ReadEnds : public ReadEndsBase
		{
	
			BamAlignment::shared_ptr_type p;
			BamAlignment::shared_ptr_type q;
			
			ReadEnds() : ReadEndsBase()
			{
			}
			
			void reset()
			{
				ReadEndsBase::reset();
				p.reset();
				q.reset();
			}
			
			ReadEnds recode() const
			{
				std::ostringstream ostr;
				put(ostr);
				std::istringstream istr(ostr.str());
				ReadEnds RE;
				RE.get(istr);
				return RE;
			}

			template<typename get_type>
			void get(get_type & G)
			{
				ReadEndsBase::get(G);
				
				uint64_t numal = G.get();
				
				if ( numal > 0 )
					p = BamAlignment::shared_ptr_type(new BamAlignment(G));
				if ( numal > 1 )
					q = BamAlignment::shared_ptr_type(new BamAlignment(G));
			}

			template<typename put_type>
			void put(put_type & P) const
			{
				ReadEndsBase::put(P);

				unsigned int const havep = ((p.get() != 0) ? 1 : 0);
				unsigned int const haveq = ((q.get() != 0) ? 1 : 0);
				uint64_t const numal = havep+haveq;
				P.put(static_cast<uint8_t>(numal));
					
				if ( havep )
					p->serialise(P);
				if ( haveq )
					q->serialise(P);
			}

			ReadEnds(
				::libmaus::bambam::BamAlignment const & p, 
				::libmaus::bambam::BamHeader const & header,
				::libmaus::bambam::ReadEnds & RE,
				bool const copyAlignment = false
			)
			{
				reset();
				fillFrag(p,header,RE);
				if ( copyAlignment )
					 this->p = p.sclone();
			}

			ReadEnds(
				::libmaus::bambam::BamAlignment const & p, 
				::libmaus::bambam::BamAlignment const & q, 
				::libmaus::bambam::BamHeader const & header,
				::libmaus::bambam::ReadEnds & RE,
				bool const copyAlignment = false
			)
			{
				reset();
				fillFragPair(p,q,header,RE);
				if ( copyAlignment )
				{
					 this->p = p.sclone();
					 this->q = q.sclone();
				}
			}
			
		};

		inline std::ostream & operator<<(std::ostream & out, ReadEnds::read_end_orientation reo)
		{
			switch ( reo )
			{
				case ReadEnds::F: out << "F"; break;
				case ReadEnds::R: out << "R"; break;
				case ReadEnds::FF: out << "FF"; break;
				case ReadEnds::FR: out << "FR"; break;
				case ReadEnds::RF: out << "RF"; break;
				case ReadEnds::RR: out << "RR"; break;
			}
			
			return out;
		}

		inline std::ostream & operator<<(std::ostream & out, ReadEnds const & RE)
		{
			out << "ReadEnds("
				<< "libId=" << RE.libraryId << ","
				<< "r1Seq=" << RE.read1Sequence << ","
				<< "r1Cor=" << RE.read1Coordinate << ","
				<< "orie=" << RE.orientation << ","
				<< "r2Seq=" << RE.read2Sequence << ","
				<< "r2Cor=" << RE.read2Coordinate << ","
				<< "r1Rank=" << RE.read1IndexInFile << ","
				<< "r2Rank=" << RE.read2IndexInFile << ","
				<< "score=" << RE.score << ","
				<< "RG=" << RE.readGroup << ","
				<< "tile=" << static_cast<int>(RE.tile) << ","
				<< "x=" << RE.x << ","
				<< "y=" << RE.y 
				<< ")";
				
			if ( RE.p )
				out << "[" << RE.p->getName() << "]";
				
			return out;
		}
	}
}
#endif
