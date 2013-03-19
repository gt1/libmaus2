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

		struct ReadEnds
		{
			static ::libmaus::util::DigitTable const D;

			enum read_end_orientation { F=0, R=1, FF=2, FR=3, RF=4, RR=5 };

			uint16_t libraryId;
			read_end_orientation orientation;
			uint32_t read1Sequence;
			 int32_t read1Coordinate;
			uint32_t read2Sequence;
			 int32_t read2Coordinate;

			uint64_t read1IndexInFile;
			uint64_t read2IndexInFile;

			uint32_t score;
			uint16_t readGroup;
			uint16_t tile;
			uint32_t x;
			uint32_t y;
			
			BamAlignment::shared_ptr_type p;
			BamAlignment::shared_ptr_type q;
			
			bool isPaired() const
			{
				return read2Sequence != 0;
			}
			
			std::pair<int64_t,int64_t> getCoord1() const
			{
				return std::pair<int64_t,int64_t>(read1Sequence,read1Coordinate);
			}
			std::pair<int64_t,int64_t> getCoord2() const
			{
				return std::pair<int64_t,int64_t>(read2Sequence,read2Coordinate);
			}
			
			ReadEnds()
			{
				reset();
			}
			
			void reset()
			{
				memset(this,0,sizeof(*this));	
				p.reset();
				q.reset();
			}
			
			bool operator<(ReadEnds const & o) const 
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
			bool operator>(ReadEnds const & o) const 
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
			
			bool operator==(ReadEnds const & o) const
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
			bool operator!=(ReadEnds const & o) const
			{
				return ! (*this == o);
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
				this->libraryId = ::libmaus::util::UTF8::decodeUTF8(G);
				this->read1Sequence = ::libmaus::util::UTF8::decodeUTF8(G);
				this->read1Coordinate = ::libmaus::util::NumberSerialisation::deserialiseSignedNumber(G);
				this->orientation = static_cast<read_end_orientation>(G.get());
				
				this->read2Sequence = ::libmaus::util::UTF8::decodeUTF8(G);
				this->read2Coordinate = ::libmaus::util::NumberSerialisation::deserialiseSignedNumber(G);
				
				this->read1IndexInFile = ::libmaus::util::NumberSerialisation::deserialiseNumber(G);
				this->read2IndexInFile = ::libmaus::util::NumberSerialisation::deserialiseNumber(G);

				this->score = ::libmaus::util::UTF8::decodeUTF8(G);
				this->readGroup = ::libmaus::util::UTF8::decodeUTF8(G);
				
				this->tile = ::libmaus::util::UTF8::decodeUTF8(G);

				this->x = ::libmaus::util::NumberSerialisation::deserialiseNumber(G,2);
				this->y = ::libmaus::util::NumberSerialisation::deserialiseNumber(G,2);
				
				uint64_t numal = G.get();
				
				if ( numal > 0 )
					p = BamAlignment::shared_ptr_type(new BamAlignment(G));
				if ( numal > 1 )
					q = BamAlignment::shared_ptr_type(new BamAlignment(G));
			}

			template<typename put_type>
			void put(put_type & P) const
			{
				try
				{
					::libmaus::util::UTF8::encodeUTF8(this->libraryId,P);

					::libmaus::util::UTF8::encodeUTF8(this->read1Sequence,P);
					::libmaus::util::NumberSerialisation::serialiseSignedNumber(P,this->read1Coordinate);
					P.put(static_cast<uint8_t>(this->orientation));

					::libmaus::util::UTF8::encodeUTF8(this->read2Sequence,P);
					::libmaus::util::NumberSerialisation::serialiseSignedNumber(P,this->read2Coordinate);

					::libmaus::util::NumberSerialisation::serialiseNumber(P,this->read1IndexInFile);
					::libmaus::util::NumberSerialisation::serialiseNumber(P,this->read2IndexInFile);
					
					::libmaus::util::UTF8::encodeUTF8(this->score,P);
					::libmaus::util::UTF8::encodeUTF8(this->readGroup,P);
					
					::libmaus::util::UTF8::encodeUTF8(this->tile,P);

					::libmaus::util::NumberSerialisation::serialiseNumber(P,this->x,2);
					::libmaus::util::NumberSerialisation::serialiseNumber(P,this->y,2);
					
					unsigned int const havep = ((p.get() != 0) ? 1 : 0);
					unsigned int const haveq = ((q.get() != 0) ? 1 : 0);
					uint64_t const numal = havep+haveq;
					P.put(static_cast<uint8_t>(numal));
					
					if ( havep )
						p->serialise(P);
					if ( haveq )
						q->serialise(P);
				}
				catch(...)
				{
					// std::cerr << "Error while putting " << *this << std::endl;
					throw;
				}
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

			static void fillFrag(
				::libmaus::bambam::BamAlignment const & p, 
				::libmaus::bambam::BamHeader const & header,
				::libmaus::bambam::ReadEnds & RE
			)
			{
				RE.libraryId = p.getLibraryId(header);
				RE.read1Sequence = p.getRefIDChecked() + 1;
				RE.read1Coordinate = p.getCoordinate() + 1;
				RE.orientation = p.isReverse() ? ::libmaus::bambam::ReadEnds::R : ::libmaus::bambam::ReadEnds::F;
				RE.read1IndexInFile = p.getRank();
				RE.score = p.getScore();
				
				if ( p.isPaired() && (!p.isMateUnmap()) )
					RE.read2Sequence = p.getNextRefIDChecked() + 1;
					
				char const * readname = p.getName();
				char const * readnamee = readname + (p.getLReadName()-1);

				int cnt[2] = { 0,0 };
				for ( char const * c = readname; c != readnamee; ++c )
					cnt [ (static_cast<int>(*c) - ':') == 0 ] ++;
				bool const rnparseok = (cnt[1] == 4);
				
				// parse tile, x, y
				if ( rnparseok )
				{
					uint8_t const * sem[4];
					uint8_t const ** psem = &sem[0];
					for ( uint8_t const * c = reinterpret_cast<uint8_t const *>(readname); c != reinterpret_cast<uint8_t const *>(readnamee); ++c )
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
				
				int64_t const rg = p.getReadGroupId(header);
				RE.readGroup = rg + 1;
			}

			static void fillFragPair(
				::libmaus::bambam::BamAlignment const & p, 
				::libmaus::bambam::BamAlignment const & q, 
				::libmaus::bambam::BamHeader const & header,
				::libmaus::bambam::ReadEnds & RE
			)
			{
				RE.read1Sequence = p.getRefIDChecked() + 1;
				RE.read1Coordinate = p.getCoordinate() + 1;
				RE.read2Sequence = q.getRefIDChecked() + 1;
				RE.read2Coordinate = q.getCoordinate() + 1;
				
				if ( ! p.isReverse() )
					if ( ! q.isReverse() )
						RE.orientation = ::libmaus::bambam::ReadEnds::FF;
					else
						RE.orientation = ::libmaus::bambam::ReadEnds::FR;
				else
					if ( ! q.isReverse() )
						RE.orientation = ::libmaus::bambam::ReadEnds::RF;
					else
						RE.orientation = ::libmaus::bambam::ReadEnds::RR;
				
				RE.read1IndexInFile = p.getRank();
				RE.read2IndexInFile = q.getRank();
				
				RE.score = p.getScore() + q.getScore();
				
				if ( p.isPaired() && (!p.isMateUnmap()) )
					RE.read2Sequence = p.getNextRefIDChecked() + 1;
					
				char const * readname = p.getName();
				char const * readnamee = readname + (p.getLReadName()-1);

				int cnt[2] = { 0,0 };
				for ( char const * c = readname; c != readnamee; ++c )
					cnt [ (static_cast<int>(*c) - ':') == 0 ] ++;
				bool const rnparseok = (cnt[1] == 4);
				
				// parse tile, x, y
				if ( rnparseok )
				{
					uint8_t const * sem[4];
					uint8_t const ** psem = &sem[0];
					for ( uint8_t const * c = reinterpret_cast<uint8_t const *>(readname); c != reinterpret_cast<uint8_t const *>(readnamee); ++c )
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
				
				int64_t const rg = p.getReadGroupId(header);
				RE.readGroup = rg + 1;
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
