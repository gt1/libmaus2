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
#if ! defined(LIBMAUS_SUFFIXSORT_GAPARRAYBYTEDECODER_HPP)
#define LIBMAUS_SUFFIXSORT_GAPARRAYBYTEDECODER_HPP

#include <libmaus/suffixsort/GapArrayByteOverflowKeyAccessor.hpp>
#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/aio/SynchronousGenericInput.hpp>
#include <libmaus/util/iterator.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		struct GapArrayByteDecoder
		{
			typedef GapArrayByteDecoder this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			uint8_t const * G;
			uint64_t const gsize;

			libmaus::aio::CheckedInputStream::unique_ptr_type pCIS;

			uint64_t sparsecnt;
			
			libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type pSGI;
				
			uint64_t offset;
				
			static libmaus::aio::CheckedInputStream::unique_ptr_type openSparseFile(std::string const & filename)
			{
				libmaus::aio::CheckedInputStream::unique_ptr_type tCIS(
					new libmaus::aio::CheckedInputStream(filename)
				);
				
				return UNIQUE_PTR_MOVE(tCIS);
			}

			static uint64_t getSparseCount(std::istream & CIS)
			{
				CIS.clear();
				CIS.seekg(0,std::ios::end);
				uint64_t const n = CIS.tellg() / (2*sizeof(uint64_t));
				return n;
			}
			
			static libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type openWordPairFile(
				std::istream & CIS, uint64_t const n, uint64_t const offset
			)
			{
				libmaus::suffixsort::GapArrayByteOverflowKeyAccessor acc(CIS);
				libmaus::util::ConstIterator<libmaus::suffixsort::GapArrayByteOverflowKeyAccessor,uint64_t> ita(&acc,0);
				libmaus::util::ConstIterator<libmaus::suffixsort::GapArrayByteOverflowKeyAccessor,uint64_t> ite(&acc,n);
			
				libmaus::util::ConstIterator<libmaus::suffixsort::GapArrayByteOverflowKeyAccessor,uint64_t> itc = std::lower_bound(ita,ite,offset);
				
				uint64_t const el = itc-ita;
				uint64_t const restel = n - el;
				
				CIS.clear();
				CIS.seekg( el * 2 * sizeof(uint64_t), std::ios::beg );
				
				libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type tSGI(
					new libmaus::aio::SynchronousGenericInput<uint64_t>(CIS,1024,2*restel)
				);
				
				return UNIQUE_PTR_MOVE(tSGI);
			}

			GapArrayByteDecoder(
				uint8_t const * rG, uint64_t const rgsize,
				std::string const & tmpfilename,
				uint64_t const roffset
			)
			: 
			  G(rG), gsize(rgsize), 
			  pCIS(openSparseFile(tmpfilename)), 
			  sparsecnt(getSparseCount(*pCIS)), 
			  pSGI(openWordPairFile(*pCIS,sparsecnt,roffset)),
			  offset(roffset)
			{
			
			}
			
			template<typename iterator>
			void decode(iterator it, uint64_t n)
			{
				assert ( offset + n <= gsize );
				
				while ( n )
				{
					uint64_t nextsparse;
					
					// no next sparse or next sparse out of range
					if ( (! pSGI->peekNext(nextsparse)) || (nextsparse >= offset + n) )
					{
						uint64_t const tocopy = n;
					
						std::copy(G+offset,G+offset+tocopy,it);
						it += tocopy;
						offset += tocopy;
						n -= tocopy;				
					}
					else
					{
						uint64_t const tocopy = nextsparse-offset;
						
						std::copy(G+offset,G+offset+tocopy,it);
						it += tocopy;
						offset += tocopy;
						n -= tocopy;

						assert ( offset == nextsparse );
						
						pSGI->getNext(nextsparse);
						pSGI->getNext(nextsparse);
						
						*(it++) = G[offset++] + 256 * nextsparse;
						n -= 1;
					}
				}
			}
		};
	}
}
#endif
