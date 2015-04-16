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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTDECODER_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTDECODER_HPP

#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/bambam/BamHeader.hpp>
#include <libmaus2/bambam/BamHeaderLowMem.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * base class for decoding BAM files
		 **/
		struct BamAlignmentDecoder
		{
			public:
			typedef BamAlignmentDecoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			protected:
			//! true if there is an alignment in the put back buffer, i.e. next call to readAlignment will not load a new alignment
			bool putbackbuffer;
			//! auxiliary memory space for decoding
			::libmaus2::bambam::BamFormatAuxiliary auxiliary;
			//! id of next pattern
			uint64_t patid;
			//! alignment
			::libmaus2::bambam::BamAlignment alignment;
			//! rank of next alignment
			uint64_t rank;
			//! true if we put a rank aux field in each decoded alignment
			bool putrank;
			//! true if validation of alignments on input is active
			bool validate;

			public:
			//! pattern type
			typedef ::libmaus2::fastx::FASTQEntry pattern_type;
		
			/**
			 * constructor
			 *
			 * @param rputrank if true then put rank as aux tag in each decoded alignment
			 **/
			BamAlignmentDecoder(bool const rputrank = false) : putbackbuffer(false), auxiliary(), patid(0), rank(0), putrank(rputrank), validate(true) {}
			/**
			 * destructor
			 **/
			virtual ~BamAlignmentDecoder() {}
			
			/**
			 * pure virtual alignment input method
			 *
			 * @return bool if alignment input was successfull and a new alignment was stored
			 **/
			virtual bool readAlignmentInternal(bool const = false) = 0;
			
			/**
			 * get current alignment
			 *
			 * @return current alignment
			 **/
			virtual libmaus2::bambam::BamAlignment const & getAlignment() const
			{
				return alignment;
			}

			/**
			 * get current alignment
			 *
			 * @return current alignment
			 **/
			virtual libmaus2::bambam::BamAlignment & getAlignment()
			{
				return alignment;
			}
			
			/**
			 * pure virtual method for retrieving the BAM header
			 *
			 * @return BAM header object
			 **/
			virtual libmaus2::bambam::BamHeader const & getHeader() const = 0;

			/**
			 * put alignment back into stream; only one alignment can be put back in this way
			 **/
			void putback()
			{
				putbackbuffer = true;
				if ( putrank )
					rank -= 1;
			}
			
			/**
			 * read the next alignment
			 *
			 * @param delayPutRank if true, then do not put the rank in this call, even if putrank is active
			 * @return true if alignment could be read, false is no more alignments are available
			 **/
			bool readAlignment(bool const delayPutRank = false)
			{
				if ( putbackbuffer )
				{
					putbackbuffer = false;
					return true;
				}
				
				return readAlignmentInternal(delayPutRank);
			}

			/**
			 * clone current alignment and return it as a unique pointer
			 *
			 * @return clone of current alignment as unique pointer
			 **/
			libmaus2::bambam::BamAlignment::unique_ptr_type ualignment() const
			{
				libmaus2::bambam::BamAlignment::unique_ptr_type ptr(getAlignment().uclone());
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			/**
			 * clone current alignment and return it as a shared pointer
			 *
			 * @return clone of current alignment as shared pointer
			 **/
			libmaus2::bambam::BamAlignment::shared_ptr_type salignment() const
			{
				return getAlignment().sclone();
			}

			/**
			 * format current alignment as SAM line and return it as a string object
			 *
			 * @return current alignment formatted as SAM line returned in a string object
			 **/
			std::string formatAlignment()
			{
				return getAlignment().formatAlignment(getHeader(),auxiliary);
			}
			
			/**
			 * format current alignment as FastQ entry and return it as a string object
			 *
			 * @return current alignment as FastQ entry returned as a string object
			 **/
			std::string formatFastq()
			{
				return getAlignment().formatFastq(auxiliary);
			}
			
			/**
			 * fill FastQ pattern
			 *
			 * @param pattern FastQ pattern object
			 * @return true if pattern was filled, false if no more pattern were available
			 **/
			bool getNextPatternUnlocked(pattern_type & pattern)
			{
				if ( !readAlignment() )
					return false;
				
				getAlignment().toPattern(pattern,patid++);
				
				return true;
			}
			
			/**
			 * get rank value which will be assigned to the next alignment
			 *
			 * @return next rank value
			 **/
			uint64_t getRank() const
			{
				return rank;
			}

			/**
			 * put rank aux tag in current alignment
			 **/
			void putRank()
			{
				uint64_t const lrank = rank++;
				if ( putrank )
				{
					alignment.putRank("ZR",lrank /*,bamheader */);
				}			
			}

			/**
			 * disable input validation
			 **/
			void disableValidation()
			{
				validate = false;
			}

			/**
			 * read alignment from an input stream which yields uncompressed BAM data
			 *
			 * @param GZ input stream yielding uncompressed BAM entries
			 * @param alignment block data object
			 * @param bamheader BAM file header for validating reference sequence ids
			 * @param validate if true, then validation will be run on alignment
			 * @return true if alignment could be read, false if no more alignments were available
			 **/
			template<typename stream_type>
			static bool readAlignmentGz(
				stream_type & GZ,
				::libmaus2::bambam::BamAlignment & alignment,
				libmaus2::bambam::BamHeader const * bamheader = 0,
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
					alignment.D = ::libmaus2::bambam::BamAlignment::D_array_type(alignment.blocksize,false);
				GZ.read(reinterpret_cast<char *>(alignment.D.begin()),alignment.blocksize);

				if ( static_cast<int64_t>(GZ.gcount()) != static_cast<int64_t>(alignment.blocksize) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Invalid alignment (EOF in alignment block of length " << alignment.blocksize  << ")" << std::endl;
					se.finish();
					throw se;
				}
				
				if ( validate )
				{
					libmaus2_bambam_alignment_validity const validity = bamheader ? alignment.valid(*bamheader) : alignment.valid();
					if ( validity != ::libmaus2::bambam::libmaus2_bambam_alignment_validity_ok )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Invalid alignment: " << validity << std::endl;
						se.finish();
						throw se;					
					}
				}
				
				return true;
			}

			/**
			 * read alignment from an input stream which yields uncompressed BAM data
			 *
			 * @param GZ input stream yielding uncompressed BAM entries
			 * @param alignment block data object
			 * @param bamheader BAM file header for validating reference sequence ids
			 * @param validate if true, then validation will be run on alignment
			 * @return true if alignment could be read, false if no more alignments were available
			 **/
			template<typename stream_type>
			static bool readAlignmentGzLo(
				stream_type & GZ,
				::libmaus2::bambam::BamAlignment & alignment,
				libmaus2::bambam::BamHeaderLowMem const * bamheader = 0,
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
					alignment.D = ::libmaus2::bambam::BamAlignment::D_array_type(alignment.blocksize,false);
				GZ.read(reinterpret_cast<char *>(alignment.D.begin()),alignment.blocksize);

				if ( static_cast<int64_t>(GZ.gcount()) != static_cast<int64_t>(alignment.blocksize) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Invalid alignment (EOF in alignment block of length " << alignment.blocksize  << ")" << std::endl;
					se.finish();
					throw se;
				}
				
				if ( validate )
				{
					libmaus2_bambam_alignment_validity const validity = bamheader ? alignment.valid(*bamheader) : alignment.valid();
					if ( validity != ::libmaus2::bambam::libmaus2_bambam_alignment_validity_ok )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Invalid alignment: " << validity << std::endl;
						se.finish();
						throw se;					
					}
				}
				
				return true;
			}
		};
		
		struct BamAlignmentDecoderWrapper
		{
			typedef BamAlignmentDecoderWrapper this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			virtual ~BamAlignmentDecoderWrapper() {}
			virtual BamAlignmentDecoder & getDecoder() = 0;
		};
	}
}
#endif
