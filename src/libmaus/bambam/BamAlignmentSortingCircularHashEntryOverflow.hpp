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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTSORTINGCIRCULARHASHENTRYOVERFLOW_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTSORTINGCIRCULARHASHENTRYOVERFLOW_HPP

#include <string>
#include <libmaus/types/types.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/bambam/BamAlignmentNameComparator.hpp>
#include <libmaus/bambam/SnappyAlignmentMergeInput.hpp>
#include <libmaus/bambam/BamAlignmentExpungeCallback.hpp>
#include <libmaus/lz/SnappyOutputStream.hpp>

namespace libmaus
{
	namespace bambam
	{		
		/**
		 * overflow class for collating BAM input
		 **/
		struct BamAlignmentSortingCircularHashEntryOverflow
		{
			//! this type
			typedef BamAlignmentSortingCircularHashEntryOverflow this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			private:
			//! temp/overflow file name
			std::string const fn;
			//! output file
			libmaus::aio::CheckedOutputStream COS;
			//! output buffer
			libmaus::autoarray::AutoArray<uint64_t> B;
			
			//! end of output buffer
			uint64_t * const Pe;
			//! back insert pointer
			uint64_t * P;
			//! start of buffer pointer
			uint8_t * Da;
			//! current data pointer
			uint8_t * D;

			//! name comparator on buffer
			libmaus::bambam::BamAlignmentNameComparator BANC;
			
			//! index for blocks in temp file
			std::vector < std::pair<uint64_t,uint64_t> > index;
			
			//! number of entries extracted from the flush buffer
			uint64_t flushbufptr;
			//! number of elements in the flush buffer
			uint64_t flushbufcnt;
			
			//! expunge callback
			BamAlignmentExpungeCallback * expungecallback;

			/**
			 * get length of i'th entry in buffer
			 *
			 * @param i entry index
			 * @return length of alignment block in bytes
			 **/
			uint32_t getLength(uint64_t const i) const
			{
				uint32_t len = 0;
				for ( unsigned int j = 0; j < sizeof(uint32_t); ++j )
					len |= static_cast<uint32_t>(Da[P[i]+j]) << (8*j);
				return len;
			}

			public:
			/**
			 * constructor
			 *
			 * @param rfn output file name
			 * @param bufsize size of buffer in bytes
			 **/
			BamAlignmentSortingCircularHashEntryOverflow(std::string const & rfn, uint64_t const bufsize = 128*1024)
			: fn(rfn),
			  COS(fn),
			  B( (bufsize+7) / 8 ,false ),
			  Pe(B.end()),
			  P(Pe),
			  Da(reinterpret_cast<uint8_t *>(B.begin())),
			  D(Da),
			  BANC(Da),
			  flushbufptr(0),
			  flushbufcnt(0),
			  expungecallback(0)
			{
			
			}
			
			/**
			 * construct reader for reading back merged and sorted entries on disk
			 **/
			libmaus::bambam::SnappyAlignmentMergeInput::unique_ptr_type constructMergeInput()
			{
				flush();
				COS.flush();
				COS.close();
				B.release();
				libmaus::bambam::SnappyAlignmentMergeInput::unique_ptr_type ptr(libmaus::bambam::SnappyAlignmentMergeInput::construct(index,fn));
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			/**
			 * flush the alignment buffer; this writes out unpaired entries out to disk
			 * and may leave some entries in the flush buffer, which can be extracted using
			 * the getFlushBufEntry method
			 **/			
			void flush()
			{	
				uint64_t const nump = Pe-P;
				
				std::sort(P,Pe,BANC);

				uint64_t const prepos = COS.tellp();
					
				::libmaus::lz::SnappyOutputStream<libmaus::aio::CheckedOutputStream> snapOut(COS,64*1024,true /* delay writing until there is data */);

				uint64_t occnt = 0;
				uint64_t outp = 0;
				uint64_t inp = 0;
				
				if ( expungecallback )
				{
					while ( inp != nump )
					{
						if ( 
							inp+1 < nump && 
							BANC.compareIntNameOnly(P[inp],P[inp+1]) == 0 &&
							(libmaus::bambam::BamAlignmentDecoderBase::isRead1(libmaus::bambam::BamAlignmentDecoderBase::getFlags(Da+P[inp  ]+sizeof(uint32_t)))
							!=
							libmaus::bambam::BamAlignmentDecoderBase::isRead1(libmaus::bambam::BamAlignmentDecoderBase::getFlags(Da+P[inp+1]+sizeof(uint32_t))))
						)
						{
							P [ occnt ++ ] = P[inp++];
							P [ occnt ++ ] = P[inp++];
						}
						else
						{
							uint32_t len = 0;
							for ( unsigned int j = 0; j < sizeof(uint32_t); ++j )
								len |= static_cast<uint32_t>(Da[P[inp]+j]) << (8*j);
					
							snapOut.write ( reinterpret_cast<char const *>(Da + P[inp]), len + sizeof(uint32_t) );
							
							expungecallback->expunged(Da + P[inp] + sizeof(uint32_t), len);
							
							outp++;
							inp++;
						}
					}
				}
				else
				{
					while ( inp != nump )
					{
						if ( 
							inp+1 < nump && 
							BANC.compareIntNameOnly(P[inp],P[inp+1]) == 0 &&
							(libmaus::bambam::BamAlignmentDecoderBase::isRead1(libmaus::bambam::BamAlignmentDecoderBase::getFlags(Da+P[inp  ]+sizeof(uint32_t)))
							!=
							libmaus::bambam::BamAlignmentDecoderBase::isRead1(libmaus::bambam::BamAlignmentDecoderBase::getFlags(Da+P[inp+1]+sizeof(uint32_t))))
						)
						{
							P [ occnt ++ ] = P[inp++];
							P [ occnt ++ ] = P[inp++];
						}
						else
						{
							uint32_t len = 0;
							for ( unsigned int j = 0; j < sizeof(uint32_t); ++j )
								len |= static_cast<uint32_t>(Da[P[inp]+j]) << (8*j);
					
							snapOut.write ( reinterpret_cast<char const *>(Da + P[inp]), len + sizeof(uint32_t) );
							
							outp++;
							inp++;
						}
					}
				}

				snapOut.flush();

				if ( outp )
					index.push_back(std::pair<uint64_t,uint64_t>(prepos,outp));

				flushbufcnt = occnt;
				flushbufptr = 0;
			}

			/**
			 * call this method after flush until it returns false
			 *
			 * @param ptr reference for storing alignment block pointer
			 * @param length reference for storing length of alignment block
			 * @return true iff a block was obtained, false if buffer was empty
			 **/	
			bool getFlushBufEntry(uint8_t const * & ptr, uint64_t & length)
			{
				if ( flushbufptr == flushbufcnt )
				{
					P = Pe;
					D = Da;
					return false;
				}
					
				uint64_t const fbid = flushbufptr++;
				
				ptr = Da+P[fbid]+sizeof(uint32_t);
				length = getLength(fbid);
				
				return true;
			}
			
			/**
			 * check whether the buffer needs to be flushed before putting an n byte data block
			 *
			 * @param n number of bytes to be put in buffer
			 * @param first true if this is the first write for an alignment which puts a pointer
			 * @return true if buffer needs to be flushed before writing n more characters
			 **/
			bool needFlush(uint64_t const n, bool const first) const
			{
				uint64_t const restspace = reinterpret_cast<uint8_t const *>(P)-D;
				bool const needflush = (restspace < n + (first ? (sizeof(uint32_t) + sizeof(uint64_t)):0));

				// throw exception if buffer is empty but space is still too small for input data		
				if ( needflush && (D == Da) )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "libmaus::bambam::BamAlignmentSortingCircularHashEntryOverflow::needFlush(): buffer is too small for single alignment." << std::endl;
					se.finish();
					throw se;
				}
				
				return needflush;
			}

			/**
			 * write data block of length n at p
			 *
			 * @param p start of data
			 * @param n number of bytes to be written
			 * @param first true if this is the first write for an alignment
			 * @param tlen total number of bytes in the alignment
			 **/
			void write(uint8_t const * p, uint64_t const n, bool first, uint64_t const tlen)
			{
				if ( needFlush(n,first) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "BamAlignmentSortingCircularHashEntryOverflow::write(): need flush for " << tlen << " bytes." << std::endl;
					se.finish();
					throw se;
				}
			
				if ( first )
				{
					// write pointer to entry
					*(--P) = D-Da;
			
					// write length of entry	
					for ( unsigned int i = 0; i < sizeof(uint32_t); ++i )
						*(D++) = (tlen >> (i*8)) & 0xFF;
				}
				
				// copy data
				std::copy(p,p+n,D);
				D += n;
			}

			/**
			 * set callback which is called whenever an alignment is written to secondary storage
			 **/
			void setExpungeCallback(BamAlignmentExpungeCallback * rexpungecallback)
			{
				expungecallback = rexpungecallback;
			}
		};
	}
}
#endif
