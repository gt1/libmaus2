/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTSINGLEDATABAMPARSEINFO_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_GENERICINPUTSINGLEDATABAMPARSEINFO_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bambam/BamHeaderLowMem.hpp>
#include <libmaus2/bambam/BamHeaderParserState.hpp>
#include <libmaus2/bambam/parallel/DecompressedBlock.hpp>
#include <libmaus2/util/GetObject.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			struct GenericInputSingleDataBamParseInfo
			{
				libmaus2::autoarray::AutoArray<uint8_t,libmaus2::autoarray::alloc_type_c> parsestallarray;
				size_t parsestallarrayused;
			
				libmaus2::bambam::BamHeaderParserState BHPS;
				bool headercomplete;
				libmaus2::bambam::BamHeaderLowMem::unique_ptr_type Pheader;
			
				GenericInputSingleDataBamParseInfo(bool const rheadercomplete = false)
				: parsestallarray(), parsestallarrayused(0), BHPS(), headercomplete(rheadercomplete), Pheader()
				{
				}
			
				void setupHeader()
				{
					libmaus2::bambam::BamHeaderLowMem::unique_ptr_type tptr(
						libmaus2::bambam::BamHeaderLowMem::constructFromText(BHPS.text.begin(),BHPS.text.begin()+BHPS.l_text)
					);
					Pheader = UNIQUE_PTR_MOVE(tptr);
				}
			
				void parseStallArrayPush(uint8_t const * p, size_t c)
				{
					if ( parsestallarrayused + c > parsestallarray.size() )
						parsestallarray.resize(parsestallarrayused + c);
					
					std::copy(p,p+c,parsestallarray.begin() + parsestallarrayused);
					parsestallarrayused += c;
				}
			
				void parseStallArrayPush(uint8_t const p)
				{
					if ( parsestallarrayused + 1 > parsestallarray.size() )
						parsestallarray.resize(parsestallarrayused + 1);
					
					parsestallarray[parsestallarrayused++] = p;
				}
			
				void parseBlock(libmaus2::bambam::parallel::DecompressedBlock::shared_ptr_type block)
				{
					uint8_t const * Da = reinterpret_cast<uint8_t const *>(block->D.begin());
					uint8_t const * Dc = Da;
					uint8_t const * De = Da + block->uncompdatasize;
					
					if ( ! this->headercomplete )
					{
						libmaus2::util::GetObject<uint8_t const *> G(reinterpret_cast<uint8_t const *>(Da));
						std::pair<bool,uint64_t> const P = this->BHPS.parseHeader(G,De-Da);
			
						this->headercomplete = P.first;
						Dc += P.second;
					
						if ( this->headercomplete )
							this->setupHeader();
					}
					
					// reset pointer array
					block->resetParseArray();
					
					// anything in stall array?
					if ( this->parsestallarrayused )
					{
						// extend until we have the length of the next record
						while ( this->parsestallarrayused < sizeof(uint32_t) && Dc != De )
							this->parseStallArrayPush(*(Dc++));
						
						// do we have at least the record length now?
						if ( this->parsestallarrayused >= sizeof(uint32_t) )
						{
							// length of record
							uint32_t const n = libmaus2::bambam::DecoderBase::getLEInteger(this->parsestallarray.begin(),4);
							// length we already have copied
							uint32_t const alcop = this->parsestallarrayused - sizeof(uint32_t);
							// rest in block
							ptrdiff_t rest = De-Dc;
							// number of bytes to copy
							size_t const tocopy = std::min(static_cast<ptrdiff_t>(n-alcop),rest);
							
							// append data
							this->parseStallArrayPush(Dc,tocopy);
							// update block pointer
							Dc += tocopy;
							
							// full record?
							if ( this->parsestallarrayused == n + sizeof(uint32_t) )
							{
								// get current offset from start of data block
								ptrdiff_t const o = Dc - Da;
			
								// push record
								block->pushParsePointer(							
									block->appendData(
										this->parsestallarray.begin(),
										this->parsestallarrayused
									)
								);
								
								// mark stall buffer as empty
								this->parsestallarrayused = 0;
			
								// set new block pointers (appendData may have reallocated the array)
								Da = reinterpret_cast<uint8_t const *>(block->D.begin());
								Dc = Da + o;
								De = Da + block->uncompdatasize;
							}
						}
					}
					
					uint32_t n = 0;
					while ( 
						De-Dc >= static_cast<ptrdiff_t>(sizeof(uint32_t)) &&
						De-Dc >= static_cast<ptrdiff_t>(sizeof(uint32_t) + (n=libmaus2::bambam::DecoderBase::getLEInteger(Dc,4)))
					)
					{
						#if 0
						if ( block->final )
							std::cerr << "n=" << n << std::endl;
						#endif
			
						block->pushParsePointer(reinterpret_cast<char const *>(Dc));
						Dc += n + sizeof(uint32_t);
					}
					
					// overlap with next block?
					if ( Dc != De )
					{
						this->parseStallArrayPush(Dc,De-Dc);
						Dc += De-Dc;
					}
					
					// file truncated?
					if ( block->final && this->parsestallarrayused )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "Stream is truncated (end of file inside a BAM record)" << std::endl;
						lme.finish();
						throw lme;
					}
					
					#if 0
					::libmaus2::bambam::BamFormatAuxiliary aux;
					for ( size_t i = 0; i < block->getNumParsePointers(); ++i )
					{
						uint8_t const * p = reinterpret_cast<uint8_t const *>(block->D.begin() + block->PP[i]);
						uint32_t const n = libmaus2::bambam::DecoderBase::getLEInteger(p,sizeof(uint32_t));
						libmaus2::bambam::BamAlignmentDecoderBase::formatAlignment(
							std::cout, p + sizeof(uint32_t), n, *(data[streamid]->Pheader),aux
						);
						std::cout.put('\n');
					}
					#endif
				
				}
			};
		}
	}
}
#endif
