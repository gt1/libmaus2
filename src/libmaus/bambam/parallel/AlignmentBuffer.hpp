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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_ALIGNMENTBUFFER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_ALIGNMENTBUFFER_HPP

#include <libmaus/bambam/parallel/PushBackSpace.hpp>
#include <libmaus/util/GrowingFreeList.hpp>
#include <libmaus/bambam/ChecksumsInterface.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct AlignmentBuffer
			{
				typedef AlignmentBuffer this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				typedef uint64_t pointer_type;
				
				uint64_t id;
				uint64_t subid;
			
				// data
				libmaus::autoarray::AutoArray<uint8_t,libmaus::autoarray::alloc_type_c> A;
				
				// text (block data) insertion pointer
				uint8_t * pA;
				// alignment block pointer insertion marker
				pointer_type * pP;
				
				// pointer difference for insertion (insert this many pointers for each alignment)
				uint64_t volatile pointerdif;
				
				// is this the last block for the stream?
				bool volatile final;
				
				// absolute low mark of this buffer block
				uint64_t volatile low;
				
				// alignment free list
				libmaus::util::GrowingFreeList<libmaus::bambam::BamAlignment> freelist;
				
				// MQ filter (for fixmate operation)
				libmaus::bambam::BamAuxFilterVector const MQfilter;
				// MS filter 
				libmaus::bambam::BamAuxFilterVector const MSfilter;
				// MC filter 
				libmaus::bambam::BamAuxFilterVector const MCfilter;
				// MT filter 
				libmaus::bambam::BamAuxFilterVector const MTfilter;
				// MQ,MS,MC,MT filter
				libmaus::bambam::BamAuxFilterVector const MQMSMCMTfilter;
				
				std::deque<libmaus::bambam::BamAlignment *> stallBuffer;
				
				size_t byteSize()
				{
					return 
						2*sizeof(uint64_t) +
						A.byteSize() +
						sizeof(uint8_t *) +
						sizeof(pointer_type *) +
						sizeof(uint64_t) +
						sizeof(bool) + 
						sizeof(uint64_t) +
						freelist.byteSize() +
						MQfilter.byteSize() +
						MSfilter.byteSize() +
						MCfilter.byteSize() +
						MTfilter.byteSize() +
						MQMSMCMTfilter.byteSize() +
						stallBuffer.size() * sizeof(libmaus::bambam::BamAlignment *);
				}
				
				void pushFrontStallBuffer(libmaus::bambam::BamAlignment * algn)
				{
					stallBuffer.push_front(algn);
				}
	
				void pushBackStallBuffer(libmaus::bambam::BamAlignment * algn)
				{
					stallBuffer.push_back(algn);
				}
				
				libmaus::bambam::BamAlignment * popStallBuffer()
				{
					if ( stallBuffer.size() )
					{
						libmaus::bambam::BamAlignment * algn = stallBuffer.front();
						stallBuffer.pop_front();
						return algn;
					}
					else
					{
						return 0;
					}
				}
				
				char const * getNameAt(uint64_t const i)
				{
					return
						libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[i] + sizeof(uint32_t));
				}
				
				void computeSplitPoints(std::vector<size_t> & V)
				{
					assert ( V.size() > 0 );
					uint64_t const numpoints = V.size()-1;
					
					uint64_t low = 0;
					uint64_t const f = fill();
					uint64_t pointsleft = numpoints;
					uint64_t o = 0;
					
					V[o++] = 0;
					
					while ( pointsleft )
					{
						// upper end, not included in interval
						uint64_t high = low + ((f-low+pointsleft-1)/pointsleft);
						assert ( high <= f );

						while ( 
							high && (high < f) &&
							strcmp(getNameAt(high-1),getNameAt(high)) == 0 
						)
							++high;
							
						V[o++] = high;
							
						low = high;
						pointsleft -= 1;
					}
					
					assert ( o == numpoints+1 );
					assert ( V[numpoints] == f );
				}
							
				void returnAlignments(std::vector<libmaus::bambam::BamAlignment *> & algns)
				{
					for ( uint64_t i = 0; i < algns.size(); ++i )
						returnAlignment(algns[i]);
				}
				
				void returnAlignment(libmaus::bambam::BamAlignment * algn)
				{
					freelist.put(algn);
				}
				
				uint64_t extractNextSameName(std::deque<libmaus::bambam::BamAlignment *> & algns)
				{
					uint64_t const c = nextSameName();
					
					algns.resize(c);
					
					for ( uint64_t i = 0; i < c; ++i )
					{
						algns[i] = freelist.get();
						
						uint64_t len = getNextLength();
						uint8_t const * p = getNextData();
						
						if ( len > algns[i]->D.size() )
							algns[i]->D = libmaus::bambam::BamAlignment::D_array_type(len,false);
							
						algns[i]->blocksize = len;
						memcpy(algns[i]->D.begin(),p,len);
						
						advance();
					}
					
					return c;
				}
				
				uint64_t nextSameName() const
				{
					if ( ! hasNext() )
						return 0;
					
					char const * refname = libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + *pP + 4);
					uint64_t c = 1;
					
					while ( 
						((pP + c) != reinterpret_cast<pointer_type const *>(A.end())) &&
						(strcmp(libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[c] + 4),refname) == 0)
					)
					{
						++c;
					}
					
					return c;			
				}
				
				bool hasNext() const
				{
					return pP != reinterpret_cast<pointer_type const *>(A.end());
				}
				
				void advance(uint64_t const c = 1)
				{
					assert ( static_cast<ptrdiff_t>(c) <= (reinterpret_cast<pointer_type const *>(A.end())-pP) );
					pP += c;
				}
				
				std::pair<uint8_t const *,uint64_t> at(uint64_t const i) const
				{
					uint64_t const offset = pP[i];
					uint8_t const * text = A.begin() + offset + sizeof(uint32_t);
					uint64_t const len = decodeLength(offset);
					return std::pair<uint8_t const *,uint64_t>(text,len);
				}

				std::pair<uint8_t *,uint64_t> at(uint64_t const i)
				{
					uint64_t const offset = pP[i];
					uint8_t * text = A.begin() + offset + sizeof(uint32_t);
					uint64_t const len = decodeLength(offset);
					return std::pair<uint8_t *,uint64_t>(text,len);
				}
				
				uint8_t const * textAt(uint64_t const i) const
				{
					return A.begin() + pP[i] + sizeof(uint32_t);
				}

				uint8_t * textAt(uint64_t const i)
				{
					return A.begin() + pP[i] + sizeof(uint32_t);
				}
				
				void setLengthAt(uint64_t const i, uint32_t const l)
				{
					uint64_t const offset = pP[i];
					uint8_t * text = A.begin() + offset;
					text[0] = (l >> 0)&0xFF;
					text[1] = (l >> 8)&0xFF;
					text[2] = (l >> 16)&0xFF;
					text[3] = (l >> 24)&0xFF;
				}
				
				uint64_t lengthAt(uint64_t const i) const
				{
					return decodeLength(pP[i]);
				}
				
				uint64_t getTotalLength() const
				{
					uint64_t const f = fill();
					uint64_t l = 0;
					for ( uint64_t i = 0; i < f; ++i )
						l += decodeLength(pP[i]);
					return l;
				}
				
				void getLinearOutputFragments(
					uint64_t const maxblocksize, std::vector<std::pair<uint8_t *,uint8_t *> > & V
				)
				{
					// number of entries in buffer
					uint64_t const rf = fill();

					// compute total number of bytes in buffer
					uint8_t * pc = A.begin();
					if ( rf )
					{
						pc += pP[rf-1];
						uint32_t const l = 
							(static_cast<uint32_t>(pc[0]) << 0)  |
							(static_cast<uint32_t>(pc[1]) << 8)  |
							(static_cast<uint32_t>(pc[2]) << 16) |
							(static_cast<uint32_t>(pc[3]) << 24) ;
						pc += (4+l);
					}
					// total length of character data in bytes
					uint64_t const totallen = pc - (A.begin());
					// target number of blocks
					uint64_t const tnumblocks = (totallen + maxblocksize - 1)/maxblocksize;
					// block size
					uint64_t const blocksize = tnumblocks ? ((totallen + tnumblocks - 1)/tnumblocks) : 0;
					// number of blocks
					uint64_t const numblocks = 
						std::max(blocksize ? ((totallen + blocksize - 1)/blocksize) : 0,static_cast<uint64_t>(1));
					
					V.resize(numblocks);

					for ( uint64_t o = 0; o < numblocks; ++o )
					{
						uint64_t const low  = o*blocksize;
						uint64_t const high = std::min(low+blocksize,totallen); 
						V[o] = std::pair<uint8_t *,uint8_t *>(A.begin()+low,A.begin()+high);
					}
					
					assert ( V.size() );
					assert ( (V[V.size()-1].second - V[0].first) == static_cast<ptrdiff_t>(totallen) );
				}
	
				uint32_t decodeLength(uint64_t const off) const
				{
					uint8_t const * B = A.begin() + off;
					
					return
						(static_cast<uint32_t>(B[0]) << 0)  |
						(static_cast<uint32_t>(B[1]) << 8)  |
						(static_cast<uint32_t>(B[2]) << 16) |
						(static_cast<uint32_t>(B[3]) << 24) ;
				}
				
				uint32_t getNextLength() const
				{
					return decodeLength(*pP);
				}
	
				uint8_t const * getNextData() const
				{
					return A.begin() + *pP + sizeof(uint32_t);
				}
				
				static std::vector<std::string> getMQMSMCMTFilterVector()
				{
					std::vector<std::string> V;
					V.push_back(std::string("MQ"));
					V.push_back(std::string("MS"));
					V.push_back(std::string("MC"));
					V.push_back(std::string("MT"));
					return V;
				}
				
				static uint64_t alignPointerSize(uint64_t const buffersize)
				{
					return ((buffersize + sizeof(pointer_type)-1)/sizeof(pointer_type))*sizeof(pointer_type);
				}
				
				AlignmentBuffer(uint64_t const buffersize, uint64_t const rpointerdif = 1)
				: id(0), subid(0), A(alignPointerSize(buffersize),false), pA(A.begin()), pP(reinterpret_cast<pointer_type *>(A.end())), pointerdif(rpointerdif), final(false), low(0),
				  MQfilter(std::vector<std::string>(1,std::string("MQ"))),
				  MSfilter(std::vector<std::string>(1,std::string("MS"))),
				  MCfilter(std::vector<std::string>(1,std::string("MC"))),
				  MTfilter(std::vector<std::string>(1,std::string("MT"))),
				  MQMSMCMTfilter(getMQMSMCMTFilterVector())
				{
					assert ( pointerdif >= 1 );
				}

				void extend(uint64_t const frac = 1)
				{
					// number of text bytes inserted
					ptrdiff_t const textoffset = pA-A.begin();
					// number of bytes used for pointers
					ptrdiff_t const numptrbytes = A.end() - reinterpret_cast<uint8_t const *>(pP);
				
					// number of bytes in array
					size_t const numbytes = A.size();
					// extend by frac
					size_t const prenewsize = std::max(
						static_cast<size_t>(numbytes + (numbytes+frac-1)/frac),
						static_cast<size_t>(numbytes+1)
					);
					// make new size multiple of pointer size
					size_t const newsize = ((prenewsize + sizeof(pointer_type) - 1)/sizeof(pointer_type))*sizeof(pointer_type);
					
					assert ( newsize > A.size() );
					assert ( newsize % sizeof(pointer_type) == 0 );
					
					A.resize(newsize);
					
					memmove(
						// new pointer region
						A.end()-numptrbytes,
						// old pointer region
						A.begin() + (numbytes - numptrbytes),
						// number of bytes to copy
						numptrbytes
					);
					
					pA = A.begin() + textoffset;
					pP = reinterpret_cast<pointer_type *>(A.end() - numptrbytes);
				}
				
				bool empty() const
				{
					return pA == A.begin();
				}
				
				uint64_t fill() const
				{
					return (reinterpret_cast<pointer_type const *>(A.end()) - pP);
				}

				// number of entries in unpacked form
				uint64_t fillUnpacked() const
				{
					return fill() / pointerdif;
				}
				
				void reset()
				{
					low = 0;
					pA = A.begin();
					pP = reinterpret_cast<pointer_type *>(A.end());
				}
				
				uint64_t free() const
				{
					return reinterpret_cast<uint8_t *>(pP) - pA;
				}
				
				void reorder()
				{
					uint64_t const pref = (reinterpret_cast<pointer_type *>(A.end()) - pP) / pointerdif;
									
					// compact pointer array if pointerdif>1
					if ( pointerdif > 1 )
					{
						// start at the end
						pointer_type * i = reinterpret_cast<pointer_type *>(A.end());
						pointer_type * j = reinterpret_cast<pointer_type *>(A.end());
						
						// work back to front
						while ( i != pP )
						{
							i -= pointerdif;
							j -= 1;
							
							*j = *i;
						}
						
						// reset pointer
						pP = j;
					}
	
					// we filled the pointers from back to front so reverse their order
					std::reverse(pP,reinterpret_cast<pointer_type *>(A.end()));
					
					uint64_t const postf = reinterpret_cast<pointer_type *>(A.end()) - pP;
					
					assert ( pref == postf );
				}
				
				uint64_t removeLastName(PushBackSpace & BPDPBS)
				{
					uint64_t const c = countLastName();
					uint64_t const f = fill();
					
					for ( uint64_t i = 0; i < c; ++i )
					{
						uint64_t const p = pP[f-i-1];
						BPDPBS.push(A.begin()+p+4,decodeLength(p));
					}
	
					// start at the end
					pointer_type * i = reinterpret_cast<pointer_type *>(A.end())-c;
					pointer_type * j = reinterpret_cast<pointer_type *>(A.end());
					
					// move pointers to back
					while ( i != pP )
					{
						--i;
						--j;
						*j = *i;
					}
					
					pP = j;
					
					return c;
				}
				
				uint64_t countLastName() const
				{
					uint64_t const f = fill();
					
					if ( f <= 1 )
						return f;
					
					char const * lastname = libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[f-1] + 4);
					
					uint64_t s = 1;
					
					while ( 
						s < f 
						&&
						strcmp(
							lastname,
							libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[f-s-1] + 4)	
						) == 0
					)
						++s;
					
					#if 0	
					for ( uint64_t i = 0; i < s; ++i )
					{
						std::cerr << "[" << i << "]=" << libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[f-i-1] + 4) << std::endl;
					}
					#endif
					
					if ( s != f )
					{
						assert (
							strcmp(
								lastname,
								libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[f-s-1] + 4)
							)
							!= 
							0
						);
					}
						
					return s;
				}
				
				bool checkValidPacked(uint64_t const low, uint64_t const high) const
				{
					bool ok = true;
					
					for ( uint64_t i = low; ok && i < high; ++i )
					{
						libmaus::bambam::libmaus_bambam_alignment_validity val = 
							libmaus::bambam::BamAlignmentDecoderBase::valid(A.begin() + pP[i] + 4, decodeLength(pP[i]));
					
						ok = ok && ( val == libmaus::bambam::libmaus_bambam_alignment_validity_ok );
					}
					
					return ok;		
				}

				void updateChecksumsPacked(
					uint64_t const low, uint64_t const high,
					libmaus::bambam::ChecksumsInterface & chksums
				) const
				{
					for ( uint64_t i = low; i < high; ++i )
						chksums.update(A.begin() + pP[i] + 4, decodeLength(pP[i]));
				}

				bool checkValidPacked() const
				{
					return checkValidPacked(0,fill());
				}
	
				bool checkValidUnpacked() const
				{
					// number of elements in buffer
					uint64_t const f = fill() / pointerdif;
					bool ok = true;
					
					for ( uint64_t i = 0; ok && i < f; ++i )
					{
						libmaus::bambam::libmaus_bambam_alignment_validity val = 
							libmaus::bambam::BamAlignmentDecoderBase::valid(
								A.begin() + pP[pointerdif*i] + 4,
								decodeLength(pP[pointerdif*i])
						);
					
						ok = ok && ( val == libmaus::bambam::libmaus_bambam_alignment_validity_ok );
					}
					
					return ok;		
				}

				bool checkMultipleNamesUnpacked() const
				{
					// number of elements in buffer
					uint64_t const f = fillUnpacked();
					
					if ( f <= 1 )
						return false;

					bool singlename = true;
					char const * refname = 
						libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[pointerdif*0] + 4);					
					
					for ( uint64_t i = 1; singlename && i < f; ++i )
					{
						singlename = singlename &&
							strcmp(
								libmaus::bambam::BamAlignmentDecoderBase::getReadName(A.begin() + pP[pointerdif*i] + 4)
								,
								refname
							) == 0;
					}
										
					return !singlename;
				}
				
				bool put(char const * p, uint64_t const n)
				{
					while ( true )
					{
						if ( n + sizeof(uint32_t) + pointerdif * sizeof(pointer_type) <= free() )
						{
							// store pointer
							pP -= pointerdif;
							*pP = pA-A.begin();
		
							// store length
							*(pA++) = (n >>  0) & 0xFF;
							*(pA++) = (n >>  8) & 0xFF;
							*(pA++) = (n >> 16) & 0xFF;
							*(pA++) = (n >> 24) & 0xFF;
							// copy alignment data
							// std::copy(p,p+n,reinterpret_cast<char *>(pA));
							memcpy(pA,p,n);
							pA += n;
																
							return true;
						}
						else
						{
							// extend buffer if we do not have multiple read names
							if ( !checkMultipleNamesUnpacked() )
								extend(16);
							else
								return false;
						}
					}
				}

				void putExtend(char const * p, uint64_t const n)
				{
					while ( n + sizeof(uint32_t) + pointerdif * sizeof(pointer_type) > free() )
						extend(16);
									
					assert ( n + sizeof(uint32_t) + pointerdif * sizeof(pointer_type) <= free() );

					// store pointer
					pP -= pointerdif;
					*pP = pA-A.begin();

					// store length
					*(pA++) = (n >>  0) & 0xFF;
					*(pA++) = (n >>  8) & 0xFF;
					*(pA++) = (n >> 16) & 0xFF;
					*(pA++) = (n >> 24) & 0xFF;
					// copy alignment data
					// std::copy(p,p+n,reinterpret_cast<char *>(pA));
					memcpy(pA,p,n);
					pA += n;																
				}
			};
		}
	}
}
#endif
