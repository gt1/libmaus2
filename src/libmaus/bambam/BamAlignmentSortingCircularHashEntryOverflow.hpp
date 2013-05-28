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

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentSortingCircularHashEntryOverflow
		{
			typedef BamAlignmentSortingCircularHashEntryOverflow this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
					
			std::string const fn;
			libmaus::aio::CheckedOutputStream COS;
			libmaus::autoarray::AutoArray<uint64_t> B;
			
			uint64_t * const Pe;
			uint64_t * P;
			uint8_t * Da;
			uint8_t * D;

			libmaus::bambam::BamAlignmentNameComparator BANC;
			
			std::vector < std::pair<uint64_t,uint64_t> > index;
			
			uint64_t flushbufptr;
			uint64_t flushbufcnt;

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
			  flushbufcnt(0)
			{
			
			}
			
			libmaus::bambam::SnappyAlignmentMergeInput::unique_ptr_type constructMergeInput()
			{
				flush();
				COS.flush();
				COS.close();
				B.release();
				return UNIQUE_PTR_MOVE(libmaus::bambam::SnappyAlignmentMergeInput::construct(index,fn));
			}
			
			uint32_t getLength(uint64_t const i) const
			{
				uint32_t len = 0;
				for ( unsigned int j = 0; j < sizeof(uint32_t); ++j )
					len |= static_cast<uint32_t>(Da[P[i]+j]) << (8*j);
				return len;
			}
			
			void flush()
			{	
				uint64_t const nump = Pe-P;
				
				std::sort(P,Pe,BANC);

				uint64_t const prepos = COS.tellp();
					
				::libmaus::lz::SnappyOutputStream<libmaus::aio::CheckedOutputStream> snapOut(COS,64*1024,true /* delay writing until there is data */);

				uint64_t occnt = 0;
				uint64_t outp = 0;
				uint64_t inp = 0;
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

				snapOut.flush();

				if ( outp )
					index.push_back(std::pair<uint64_t,uint64_t>(prepos,outp));

				flushbufcnt = occnt;
				flushbufptr = 0;
			}

			/**
			 * call this method after flush until it returns false
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
			 * returns true if buffer needs to be flushed before writing n more characters
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
		};
	}
}
#endif
