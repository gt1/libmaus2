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
#if ! defined(LIBMAUS_BAMBAM_SAMDECODER_HPP)
#define LIBMAUS_BAMBAM_SAMDECODER_HPP

#include <libmaus/util/LineBuffer.hpp>
#include <libmaus/bambam/SamInfo.hpp>
#include <libmaus/bambam/BamHeader.hpp>
#include <libmaus/bambam/BamAlignmentDecoder.hpp>

namespace libmaus
{
	namespace bambam
	{	
		/**
		 * SAM file decoding class
		 **/
		struct SamDecoder : public libmaus::bambam::BamAlignmentDecoder
		{
			//! this type
			typedef SamDecoder this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			//! input file
			libmaus::aio::CheckedInputStream::unique_ptr_type PCIS;
			//! input stream
			std::istream & in;
			//! line buffer
			libmaus::util::LineBuffer lb;
			//! BAM header pointer
			::libmaus::bambam::BamHeader::unique_ptr_type Pbamheader;
			//! BAM header
			::libmaus::bambam::BamHeader const & bamheader;
			//! sam line
			::libmaus::bambam::SamInfo samline;

			/**
			 * interval alignment input method
			 *
			 * @param delayPutRank if true, then rank aux tag will not be inserted by this call
			 * @return true iff next alignment could be successfully read, false if no more alignment were available
			 **/
			bool readAlignmentInternal(bool const delayPutRank = false)
			{
				char const * pa = 0;
				char const * pe = 0;
				bool const ok = lb.getline(&pa,&pe);
				
				if ( ! ok )
					return false;
					
				samline.parseSamLine(pa,pe);
			
				if ( ! delayPutRank )
					putRank();
			
				return true;
			}

			static ::libmaus::bambam::BamHeader::unique_ptr_type readSamHeader(libmaus::util::LineBuffer & lb)
			{
				std::ostringstream istr;
				char const * pa = 0;
				char const * pe = 0;
				bool lineok = true;
				
				while ( (lineok = lb.getline(&pa,&pe)) )
				{
					if ( pe-pa > 0 && pa[0] == '@' )
					{
						istr.write(pa,pe-pa);
						istr.put('\n');
					}
					else
					{
						break;
					}
				}
				
				if ( lineok )
					lb.putback(pa);
				
				::libmaus::bambam::BamHeader::unique_ptr_type tptr(new ::libmaus::bambam::BamHeader(istr.str()));
				
				return UNIQUE_PTR_MOVE(tptr);
			}
						
			public:
			/**
			 * constructor by file name
			 *
			 * @param filename name of input BAM file
			 * @param rputrank if true, then a rank auxiliary tag will be attached to each alignment
			 **/
			SamDecoder(std::string const & filename, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  PCIS(new libmaus::aio::CheckedInputStream(filename)),
			  in(*PCIS),
			  lb(in,64*1024),
			  Pbamheader(readSamHeader(lb)),
			  bamheader(*Pbamheader),
			  samline(bamheader, alignment)
			{}
			
			/**
			 * constructor by compressed input stream
			 *
			 * @param in input stream delivering BAM
			 * @param rputrank if true, then a rank auxiliary tag will be attached to each alignment
			 **/
			SamDecoder(std::istream & rin, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  PCIS(),
			  in(rin),
			  lb(in,64*1024),
			  Pbamheader(readSamHeader(lb)),
			  bamheader(*Pbamheader),
			  samline(bamheader, alignment)
			{}

			/**
			 * @return BAM header
			 **/
			libmaus::bambam::BamHeader const & getHeader() const
			{
				return bamheader;
			}
		};
	}
}
#endif
