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
#if ! defined(LIBMAUS_BAMBAM_BAMDECODER_HPP)
#define LIBMAUS_BAMBAM_BAMDECODER_HPP

#include <libmaus/bambam/BamAlignment.hpp>

namespace libmaus
{
	namespace bambam
	{				
		struct BamDecoder
		{
			typedef BamDecoder this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef ::libmaus::fastx::FASTQEntry pattern_type;
		
			::libmaus::util::unique_ptr< std::ifstream >::type PISTR;
			::libmaus::lz::GzipStream::unique_ptr_type PGZ;
			::libmaus::lz::GzipStream & GZ;
			::libmaus::bambam::BamHeader bamheader;

			::libmaus::bambam::BamAlignment alignment;
			::libmaus::bambam::BamFormatAuxiliary auxiliary;
			
			uint64_t patid;
			uint64_t rank;
			bool putrank;
			
			BamDecoder(std::string const & filename, bool const rputrank = false)
			: PISTR(new std::ifstream(filename.c_str(),std::ios::binary)),
			  PGZ(new ::libmaus::lz::GzipStream(*PISTR)),
			  GZ(*PGZ), bamheader(GZ), patid(0), rank(0), putrank(rputrank)
			{
			
			}
			
			BamDecoder(std::istream & in, bool const rputrank = false)
			: PISTR(), PGZ(new ::libmaus::lz::GzipStream(in)), GZ(*PGZ), bamheader(GZ), patid(0), rank(0), putrank(rputrank)
			{
			}
			
			BamDecoder(::libmaus::lz::GzipStream & rGZ, bool const rputrank = false)
			: PISTR(), PGZ(), GZ(rGZ), bamheader(GZ), patid(0), rank(0), putrank(rputrank)
			{
			
			}
			
			::libmaus::bambam::BamAlignment::unique_ptr_type ualignment() const
			{
				return UNIQUE_PTR_MOVE(alignment.uclone());
			}
			
			::libmaus::bambam::BamAlignment::shared_ptr_type salignment() const
			{
				return alignment.sclone();
			}

			std::string formatAlignment()
			{
				return alignment.formatAlignment(bamheader,auxiliary);
			}
			
			std::string formatFastq()
			{
				return alignment.formatFastq(auxiliary);
			}
			
			void putRank()
			{
				uint64_t const lrank = rank++;
				if ( putrank )
				{
					alignment.putRank("ZR",lrank /*,bamheader */);
				}			
			}
			
			template<typename stream_type>
			static bool readAlignment(
				stream_type & GZ,
				::libmaus::bambam::BamAlignment & alignment,
				::libmaus::bambam::BamHeader const * bamheader = 0
			)
			{
				/* read alignment block size */
				int64_t const bs0 = GZ.get();
				int64_t const bs1 = GZ.get();
				int64_t const bs2 = GZ.get();
				int64_t const bs3 = GZ.get();
				if ( bs3 < 0 )
					// reached end of file
					return false;
				
				/* assemble block size as LE integer */
				alignment.blocksize = (bs0 << 0) | (bs1 << 8) | (bs2 << 16) | (bs3 << 24) ;

				/* read alignment block */
				if ( alignment.blocksize > alignment.D.size() )
					alignment.D = ::libmaus::autoarray::AutoArray<uint8_t>(alignment.blocksize);
				GZ.read(reinterpret_cast<char *>(alignment.D.begin()),alignment.blocksize);

				if ( static_cast<int64_t>(GZ.gcount()) != static_cast<int64_t>(alignment.blocksize) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Invalid alignment (EOF in alignment block of length " << alignment.blocksize  << ")" << std::endl;
					se.finish();
					throw se;
				}
				
				libmaus_bambam_alignment_validity const validity = bamheader ? alignment.valid(*bamheader) : alignment.valid();
				if ( validity != ::libmaus::bambam::libmaus_bambam_alignment_validity_ok )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Invalid alignment: " << validity << std::endl;
					se.finish();
					throw se;					
				}
				
				return true;
			}
			
			bool readAlignment(bool const delayPutRank = false)
			{
				bool const ok = readAlignment(GZ,alignment,&bamheader);
				
				if ( ! ok )
					return false;
			
				if ( ! delayPutRank )
					putRank();
			
				return true;
			}
			
			bool getNextPatternUnlocked(pattern_type & pattern)
			{
				if ( !readAlignment() )
					return false;
				
				alignment.toPattern(pattern,patid++);
				
				return true;
			}
		};
	}
}
#endif
