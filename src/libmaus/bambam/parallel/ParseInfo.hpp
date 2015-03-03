/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_PARSEINFO_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_PARSEINFO_HPP

#include <libmaus/bambam/BamHeaderLowMem.hpp>
#include <libmaus/bambam/BamHeaderParserState.hpp>
#include <libmaus/bambam/parallel/PushBackSpace.hpp>
#include <libmaus/bambam/parallel/AlignmentBuffer.hpp>
#include <libmaus/bambam/parallel/DecompressedBlock.hpp>
#include <libmaus/util/GetObject.hpp>
#include <libmaus/bambam/parallel/ParseInfoHeaderCompleteCallback.hpp>

namespace libmaus
{
	namespace bambam
	{		
		namespace parallel
		{
			struct ParseInfo
			{
				typedef ParseInfo this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				PushBackSpace BPDPBS;
				
				libmaus::bambam::BamHeaderParserState BHPS;
				bool volatile headerComplete;
				libmaus::bambam::BamHeaderLowMem::unique_ptr_type Pheader;
	
				libmaus::autoarray::AutoArray<char> concatBuffer;
				unsigned int volatile concatBufferFill;
				
				enum parser_state_type {
					parser_state_read_blocklength,
					parser_state_read_block
				};
				
				parser_state_type volatile parser_state;
				unsigned int volatile blocklengthread;
				uint32_t volatile blocklen;
				uint64_t volatile parseacc;
				
				void setHeaderFromText(char const * c, size_t const s)
				{
					libmaus::bambam::BamHeaderLowMem::unique_ptr_type Theader(libmaus::bambam::BamHeaderLowMem::constructFromText(c,c+s));
					Pheader = UNIQUE_PTR_MOVE(Theader);
					headerComplete = true;
					
					// produce BAM header
					std::string const text(c,c+s);
					libmaus::bambam::BamHeader header(text);
					std::ostringstream ostr;
					header.serialise(ostr);
					
					// fill header parser state
					std::istringstream istr(ostr.str());
					std::pair<bool,uint64_t> const P = BHPS.parseHeader(istr);
					assert ( P.first );
				}
				
				ParseInfoHeaderCompleteCallback * headerCompleteCallback;
				
				ParseInfo()
				: BPDPBS(), BHPS(), headerComplete(false),
				  concatBuffer(), concatBufferFill(0), 
				  parser_state(parser_state_read_blocklength),
				  blocklengthread(0), blocklen(0), parseacc(0), headerCompleteCallback(0)
				{
				
				}

				ParseInfo(ParseInfoHeaderCompleteCallback * rheaderCompleteCallback)
				: BPDPBS(), BHPS(), headerComplete(false),
				  concatBuffer(), concatBufferFill(0), 
				  parser_state(parser_state_read_blocklength),
				  blocklengthread(0), blocklen(0), parseacc(0), headerCompleteCallback(rheaderCompleteCallback)
				{
				
				}
				
				static uint32_t getLE4(char const * A)
				{
					unsigned char const * B = reinterpret_cast<unsigned char const *>(A);
					
					return
						(static_cast<uint32_t>(B[0]) << 0)  |
						(static_cast<uint32_t>(B[1]) << 8)  |
						(static_cast<uint32_t>(B[2]) << 16) |
						(static_cast<uint32_t>(B[3]) << 24) ;
				}
				
				void putBackLastName(AlignmentBuffer & algnbuf)
				{
					algnbuf.removeLastName(BPDPBS);
				}
				
				bool putBackBufferEmpty() const
				{
					return BPDPBS.empty();
				}
				
				libmaus::bambam::BamHeader::unique_ptr_type getHeader()
				{
					libmaus::bambam::BamHeader::unique_ptr_type ptr(new libmaus::bambam::BamHeader(BHPS));
					return UNIQUE_PTR_MOVE(ptr);
				}
	
				/**
				 * parsed decompressed bam block into algnbuf
				 *
				 * @param block decompressed bam block data
				 * @param algnbuf alignment buffer
				 * @return true if block was fully processed
				 **/
				bool parseBlock(
					DecompressedBlock & block,
					AlignmentBuffer & algnbuf
				)
				{
					if ( ! headerComplete )
					{
						libmaus::util::GetObject<uint8_t const *> G(reinterpret_cast<uint8_t const *>(block.P));
						std::pair<bool,uint64_t> Q = BHPS.parseHeader(G,block.uncompdatasize);
						
						block.P += Q.second;
						block.uncompdatasize -= Q.second;
						
						if ( Q.first )
						{
							headerComplete = true;
							libmaus::bambam::BamHeaderLowMem::unique_ptr_type Theader(
								libmaus::bambam::BamHeaderLowMem::constructFromText(
									BHPS.text.begin(),
									BHPS.text.begin()+BHPS.l_text
								)
							);
							Pheader = UNIQUE_PTR_MOVE(Theader);
							
							if ( headerCompleteCallback )
								headerCompleteCallback->bamHeaderComplete(BHPS);
						}
						else
						{
							// if this is the last block then this is an unexpected EOF within the header
							if ( block.final )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "ParseInfo::parseBlock(): Unexpected EOF in BAM header." << std::endl;
								lme.finish();
								throw lme;
							}
						
							return true;
						}
					}
	
					// check put back buffer
					while ( ! BPDPBS.empty() )
					{
						libmaus::bambam::BamAlignment * talgn = BPDPBS.top();
						
						if ( ! (algnbuf.put(reinterpret_cast<char const *>(talgn->D.begin()),talgn->blocksize)) )
							// block needs to be processed again
							return false;
							
						BPDPBS.pop();
					}
					
					// concat buffer contains data
					if ( concatBufferFill )
					{
						// parser state should be reading block
						assert ( parser_state == parser_state_read_block );
						
						// number of bytes to copy
						uint64_t const tocopy = std::min(
							static_cast<uint64_t>(blocklen - concatBufferFill),
							static_cast<uint64_t>(block.uncompdatasize)
						);
						
						// make sure there is sufficient space
						if ( concatBufferFill + tocopy > concatBuffer.size() )
							concatBuffer.resize(concatBufferFill + tocopy);
	
						// copy bytes
						std::copy(block.P,block.P+tocopy,concatBuffer.begin()+concatBufferFill);
						
						// adjust pointers
						concatBufferFill += tocopy;
						block.uncompdatasize -= tocopy;
						block.P += tocopy;
						
						if ( concatBufferFill == blocklen )
						{
							if ( ! (algnbuf.put(concatBuffer.begin(),concatBufferFill)) )
								return false;
	
							concatBufferFill = 0;
							parser_state = parser_state_read_blocklength;
							blocklengthread = 0;
							blocklen = 0;
						}
					}
					
					while ( block.uncompdatasize )
					{
						switch ( parser_state )
						{
							case parser_state_read_blocklength:
							{
								while ( 
									(!blocklengthread) && 
									(block.uncompdatasize >= 4) &&
									(
										block.uncompdatasize >= 4 + (blocklen = getLE4(block.P))
									)
								)
								{
									if ( ! (algnbuf.put(block.P+4,blocklen)) )
										return false;
	
									// skip
									blocklengthread = 0;
									block.uncompdatasize -= (blocklen+4);
									block.P += blocklen+4;
									blocklen = 0;
								}
								
								if ( block.uncompdatasize )
								{
									while ( blocklengthread < 4 && block.uncompdatasize )
									{
										blocklen |= static_cast<uint32_t>(*reinterpret_cast<unsigned char const *>(block.P)) << (blocklengthread*8);
										block.P++;
										block.uncompdatasize--;
										blocklengthread++;
									}
									
									if ( blocklengthread == 4 )
									{
										parser_state = parser_state_read_block;
									}
								}
	
								break;
							}
							case parser_state_read_block:
							{
								// copy data to concat buffer
								uint64_t const tocopy = std::min(
									static_cast<uint64_t>(blocklen),
									static_cast<uint64_t>(block.uncompdatasize)
								);
								if ( concatBufferFill + tocopy > concatBuffer.size() )
									concatBuffer.resize(concatBufferFill + tocopy);
								
								std::copy(
									block.P,
									block.P+tocopy,
									concatBuffer.begin()+concatBufferFill
								);
								
								concatBufferFill += tocopy;
								block.P += tocopy;
								block.uncompdatasize -= tocopy;
								
								// handle alignment if complete
								if ( concatBufferFill == blocklen )
								{
									if ( ! (algnbuf.put(concatBuffer.begin(),concatBufferFill)) )
										return false;
					
									blocklen = 0;
									blocklengthread = 0;
									parser_state = parser_state_read_blocklength;
									concatBufferFill = 0;
								}
							
								break;
							}
						}
					}
					
					if ( 
						block.final && 
						(
							(parser_state != parser_state_read_blocklength)
							||
							(blocklengthread != 0)
						)
					)
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "ParseInfo::parseBlock(): Unexpected EOF in BAM data, parser_state=";

						switch ( parser_state )
						{
							case parser_state_read_blocklength:
								lme.getStream() << "parser_state_read_blocklength";
								break;
							case parser_state_read_block:
								lme.getStream() << "parser_state_read_block";
								break;
						}
						
						lme.getStream() << " blocklengthread=" << blocklengthread 
							<< " headerComplete=" << headerComplete
							<< '\n';
						lme.finish();
						throw lme;
					}
					
					return true;
				}
			};
		}
	}
}
#endif
