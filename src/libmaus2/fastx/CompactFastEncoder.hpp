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

#if ! defined(COMPACTFASTENCODER_HPP)
#define COMPACTFASTENCODER_HPP

#include <libmaus2/aio/BufferedOutput.hpp>
#include <libmaus2/fastx/CompactFastTerminator.hpp>
#include <libmaus2/fastx/FastInterval.hpp>
#include <libmaus2/util/utf8.hpp>
#include <iostream>
#include <libmaus2/util/PutObject.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct CompactFastEncoder : public ::libmaus2::util::UTF8
		{
			template<typename out_type>
			static void encodeEndMarker(out_type & B)
			{
				// write end marker
				::libmaus2::util::UTF8::encodeUTF8(::libmaus2::fastx::CompactFastTerminator::getTerminator(), B);
			}
			
			template<typename out_type>
			static uint64_t encode(	
				char const * m,
				uint32_t const patlen,
				uint32_t const flags,
				out_type & B
			)
			{
				::libmaus2::util::UTF8::encodeUTF8(patlen, B);					
				::libmaus2::util::UTF8::encodeUTF8(flags, B);
				
				// indeterminate bases?
				if ( flags & 1 )
				{
					uint64_t const full = (patlen >> 1);
					uint64_t const brok = patlen - (full<<1);
					
					for ( uint64_t i = 0; i < full; ++i, m+=2 )
						B.put( (m[0]<<0)|(m[1]<<4));

					if ( brok )
						B.put(m[0]);		
				}
				else
				{
					uint64_t const full = (patlen >> 2);
					uint64_t const brok = patlen - (full<<2);
					
					for ( uint64_t i = 0; i < full; ++i, m+=4 )
						B.put( (m[0]<<0)|(m[1]<<2)|(m[2]<<4)|(m[3]<<6) );

					if ( brok == 3 )
						B.put( (m[0]<<0)|(m[1]<<2)|(m[2]<<4) );
					else if ( brok == 2 )
						B.put( (m[0]<<0)|(m[1]<<2) );
					else if ( brok == 1 )
						B.put( (m[0]<<0) );
				}
				
				return patlen;
			
			}
			
			template<typename pattern_type, typename out_type>
			static uint64_t encodePattern(
				pattern_type & pattern, out_type & B,
				uint32_t const flagadd = 0
			)
			{
				pattern.computeMapped();
				bool const isdontcarefree = pattern.isDontCareFree();

				uint32_t flags = flagadd;
				if ( !isdontcarefree )
					flags |= (1ul)<<0;
					
				return encode(pattern.mapped,pattern.getPatternLength(),flags,B);
			}
			
			template<typename out_type>
			static void putNumber(uint64_t const n, out_type & B)
			{				
				B.put( (n >> (7*8))&0xFF );
				B.put( (n >> (6*8))&0xFF );
				B.put( (n >> (5*8))&0xFF );
				B.put( (n >> (4*8))&0xFF );
				B.put( (n >> (3*8))&0xFF );
				B.put( (n >> (2*8))&0xFF );
				B.put( (n >> (1*8))&0xFF );
				B.put( (n >> (0*8))&0xFF );
			}
			
			template<typename out_type>
			static void putIndex(std::vector< ::libmaus2::fastx::FastInterval > const & index, out_type & B)
			{
				std::ostringstream indexostr;
				::libmaus2::fastx::FastInterval::serialiseVector(indexostr,index);
				std::string const indexs = indexostr.str();
				uint8_t const * indexu = reinterpret_cast<uint8_t const *>(indexs.c_str());
				for ( uint64_t i = 0; i < indexs.size(); ++i )
					B.put(indexu[i]);
				
				putNumber(indexs.size(), B);
			}

			template<typename out_type>
			static void putEndMarkerAndIndex(std::vector< ::libmaus2::fastx::FastInterval > const & index, out_type & B)
			{
				// write end marker
				::libmaus2::fastx::CompactFastEncoder::encodeEndMarker(B);
				// write index
				putIndex(index,B);				
			}
			
			template<typename reader_type>
			static void encodeFile(reader_type * reader, std::ostream & out, uint64_t const mod)
			{
				typedef typename reader_type::pattern_type pattern_type;
				
				::libmaus2::aio::BufferedOutput<uint8_t> B(out,64*1024);

				std::vector < ::libmaus2::fastx::FastInterval > index;

				uint64_t prevpos = 0;
				uint64_t numsyms = 0;
				uint64_t prevnumreads = 0;
				uint64_t numreads = 0;
				uint64_t minlen = std::numeric_limits<uint64_t>::max();
				uint64_t maxlen = 0;
				
				pattern_type pattern;
				std::cerr << "Parsing file...";
				while ( reader->getNextPatternUnlocked(pattern) )
				{
					if ( (numreads % mod == 0) && numreads )
					{
						uint64_t const pos = B.getWrittenBytes();
						
						::libmaus2::fastx::FastInterval FI(prevnumreads,numreads,prevpos,pos,numsyms,minlen,maxlen);
						index.push_back(FI);
						
						prevpos = pos;
						prevnumreads = numreads;
						numsyms = 0;
						minlen = std::numeric_limits<uint64_t>::max();
						maxlen = 0;
					}

					uint64_t const patlen = ::libmaus2::fastx::CompactFastEncoder::encodePattern(pattern, B);
					
					if ( (pattern.getPatID() & (1024*1024-1)) == 0 )
						std::cerr << "(" << (pattern.getPatID()/(1024*1024)) << "m)";
		
					numreads += 1;
					numsyms += patlen;
					minlen = std::min(minlen,patlen);
					maxlen = std::max(maxlen,patlen);
				}
				std::cerr << "done." << std::endl;

				uint64_t const pos = B.getWrittenBytes();
				::libmaus2::fastx::FastInterval FI(prevnumreads,numreads,prevpos,pos,numsyms,minlen,maxlen);
				index.push_back(FI);
				
				putEndMarkerAndIndex(index,B);
			}

			template<typename reader_type>
			static void encodeFile(std::vector<std::string> const & inputfilenames, std::ostream & out, uint64_t const mod)
			{
				typedef typename reader_type::unique_ptr_type reader_ptr_type;
				reader_ptr_type reader;

				if ( inputfilenames.size() == 1 && inputfilenames[0] == "-" )
					reader = UNIQUE_PTR_MOVE(reader_ptr_type(new reader_type("/dev/stdin")));
				else
					reader = UNIQUE_PTR_MOVE(reader_ptr_type(new reader_type(inputfilenames)));

				::libmaus2::fastx::CompactFastEncoder::encodeFile(reader.get(),out,mod);
			}

			template<typename reader_type>
			static void encodeFileSubSample(std::vector<std::string> const & inputfilenames, std::ostream & out, uint64_t const mod, uint64_t const c, uint64_t const d)
			{
				typedef typename reader_type::unique_ptr_type reader_ptr_type;
				reader_ptr_type reader;

				if ( inputfilenames.size() == 1 && inputfilenames[0] == "-" )
					reader = UNIQUE_PTR_MOVE(reader_ptr_type(new reader_type("/dev/stdin",c,d)));
				else
					reader = UNIQUE_PTR_MOVE(reader_ptr_type(new reader_type(inputfilenames,c,d)));

				::libmaus2::fastx::CompactFastEncoder::encodeFile(reader.get(),out,mod);
			}
			
		};
	}
}
#endif
