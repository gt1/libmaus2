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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTDECODER_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTDECODER_HPP

#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/BamHeader.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentDecoder
		{
			protected:
			bool putbackbuffer;
			::libmaus::bambam::BamFormatAuxiliary auxiliary;
			uint64_t patid;
			::libmaus::bambam::BamAlignment alignment;
			uint64_t rank;
			bool putrank;
			bool validate;

			public:
			typedef ::libmaus::fastx::FASTQEntry pattern_type;
		
			BamAlignmentDecoder(bool const rputrank = false) 
			: putbackbuffer(false), auxiliary(), patid(0), rank(0), putrank(rputrank), validate(true) {}
			virtual ~BamAlignmentDecoder() {}
			
			virtual bool readAlignmentInternal(bool const = false) = 0;
			libmaus::bambam::BamAlignment const & getAlignment() const
			{
				return alignment;
			}
			libmaus::bambam::BamAlignment & getAlignment()
			{
				return alignment;
			}
			virtual libmaus::bambam::BamHeader const & getHeader() const = 0;

			void putback()
			{
				putbackbuffer = true;
				if ( putrank )
					rank -= 1;
			}
			
			bool readAlignment(bool const delayPutRank = false)
			{
				if ( putbackbuffer )
				{
					putbackbuffer = false;
					return true;
				}
				
				return readAlignmentInternal(delayPutRank);
			}

			libmaus::bambam::BamAlignment::unique_ptr_type ualignment() const
			{
				return UNIQUE_PTR_MOVE(getAlignment().uclone());
			}
			
			libmaus::bambam::BamAlignment::shared_ptr_type salignment() const
			{
				return getAlignment().sclone();
			}

			std::string formatAlignment()
			{
				return getAlignment().formatAlignment(getHeader(),auxiliary);
			}
			
			std::string formatFastq()
			{
				return getAlignment().formatFastq(auxiliary);
			}
			
			bool getNextPatternUnlocked(pattern_type & pattern)
			{
				if ( !readAlignment() )
					return false;
				
				getAlignment().toPattern(pattern,patid++);
				
				return true;
			}
			
			uint64_t getRank() const
			{
				return rank;
			}

			void putRank()
			{
				uint64_t const lrank = rank++;
				if ( putrank )
				{
					alignment.putRank("ZR",lrank /*,bamheader */);
				}			
			}

			void disableValidation()
			{
				validate = false;
			}

			template<typename stream_type>
			static bool readAlignmentGz(
				stream_type & GZ,
				::libmaus::bambam::BamAlignment & alignment,
				::libmaus::bambam::BamHeader const * bamheader = 0,
				bool const validate = true
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
					alignment.D = ::libmaus::bambam::BamAlignment::D_array_type(alignment.blocksize,false);
				GZ.read(reinterpret_cast<char *>(alignment.D.begin()),alignment.blocksize);

				if ( static_cast<int64_t>(GZ.gcount()) != static_cast<int64_t>(alignment.blocksize) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Invalid alignment (EOF in alignment block of length " << alignment.blocksize  << ")" << std::endl;
					se.finish();
					throw se;
				}
				
				if ( validate )
				{
					libmaus_bambam_alignment_validity const validity = bamheader ? alignment.valid(*bamheader) : alignment.valid();
					if ( validity != ::libmaus::bambam::libmaus_bambam_alignment_validity_ok )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Invalid alignment: " << validity << std::endl;
						se.finish();
						throw se;					
					}
				}
				
				return true;
			}
		};
	}
}
#endif
