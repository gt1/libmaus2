/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_ALIGNMENTREWRITEBUFFER_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_ALIGNMENTREWRITEBUFFER_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace bambam
	{		
		namespace parallel
		{
			struct AlignmentRewriteBuffer
			{
				typedef uint64_t pointer_type;
			
				// data
				libmaus2::autoarray::AutoArray<uint8_t> A;
				
				// text (block data) insertion pointer
				uint8_t * pA;
				// alignment block pointer insertion marker
				pointer_type * pP;
				
				// pointer difference for insertion (insert this many pointers for each alignment)
				uint64_t volatile pointerdif;
				
				std::vector<uint64_t> blocksizes;
	
				uint64_t blocksCompressed;
				libmaus2::parallel::PosixSpinLock blocksCompressedLock;
	
				std::pair<uint8_t const *,uint64_t> at(uint64_t const i) const
				{
					uint64_t const offset = pP[i];
					uint8_t const * text = A.begin() + offset + sizeof(uint32_t);
					uint64_t const len = decodeLength(offset);
					return std::pair<uint8_t const *,uint64_t>(text,len);
				}
				
				uint64_t lengthAt(uint64_t const i) const
				{
					return decodeLength(pP[i]);
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
	
				int64_t decodeCoordinate(uint64_t const off) const
				{
					uint8_t const * B = A.begin() + off + sizeof(uint32_t);
					
					return
						static_cast<int64_t>
						((static_cast<uint64_t>(B[0]) << 0)  |
						(static_cast<uint64_t>(B[1]) << 8)  |
						(static_cast<uint64_t>(B[2]) << 16) |
						(static_cast<uint64_t>(B[3]) << 24) |
						(static_cast<uint64_t>(B[4]) << 32) |
						(static_cast<uint64_t>(B[5]) << 40) |
						(static_cast<uint64_t>(B[6]) << 48) |
						(static_cast<uint64_t>(B[7]) << 56)) 
						
						;
				}
							
				AlignmentRewriteBuffer(uint64_t const buffersize, uint64_t const rpointerdif = 1)
				: A(buffersize,false), pA(A.begin()), pP(reinterpret_cast<pointer_type *>(A.end())), pointerdif(rpointerdif)
				{
					assert ( pointerdif >= 1 );
				}
				
				bool empty() const
				{
					return pA == A.begin();
				}
				
				uint64_t fill() const
				{
					return (reinterpret_cast<pointer_type const *>(A.end()) - pP);
				}
				
				void reset()
				{
					pA = A.begin();
					pP = reinterpret_cast<pointer_type *>(A.end());
					blocksizes.resize(0);
	
					blocksCompressedLock.lock();
					blocksCompressed = 0;
					blocksCompressedLock.unlock();
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
				
				std::pair<pointer_type *,uint64_t> getPointerArray()
				{
					return std::pair<pointer_type *,uint64_t>(pP,fill());
				}
				
				std::pair<pointer_type *,uint64_t> getAuxPointerArray()
				{
					uint64_t const numauxpointers = (pointerdif - 1)*fill();
					return std::pair<pointer_type *,uint64_t>(pP - numauxpointers,numauxpointers);
				}
				
				bool put(char const * p, uint64_t const n)
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
						return false;
					}
				}
	
				bool put(char const * p, uint64_t const n, int64_t const coord)
				{
					if ( n + sizeof(uint32_t) + sizeof(uint64_t) + pointerdif * sizeof(pointer_type) <= free() )
					{
						// store pointer
						pP -= pointerdif;
						*pP = pA-A.begin();
	
						// store length
						*(pA++) = (n >>  0) & 0xFF;
						*(pA++) = (n >>  8) & 0xFF;
						*(pA++) = (n >> 16) & 0xFF;
						*(pA++) = (n >> 24) & 0xFF;
						
						// store coordinate
						uint64_t const ucoord = coord;
						*(pA++) = (ucoord >>  0) & 0xFF;
						*(pA++) = (ucoord >>  8) & 0xFF;
						*(pA++) = (ucoord >> 16) & 0xFF;
						*(pA++) = (ucoord >> 24) & 0xFF;
						*(pA++) = (ucoord >> 32) & 0xFF;
						*(pA++) = (ucoord >> 40) & 0xFF;
						*(pA++) = (ucoord >> 48) & 0xFF;
						*(pA++) = (ucoord >> 56) & 0xFF;
						
						// copy alignment data
						// std::copy(p,p+n,reinterpret_cast<char *>(pA));
						memcpy(pA,p,n);
						pA += n;
															
						return true;
					}
					else
					{
						return false;
					}
				}
			};
		}
	}
}
#endif
