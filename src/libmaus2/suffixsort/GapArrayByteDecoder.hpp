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
#if ! defined(LIBMAUS2_SUFFIXSORT_GAPARRAYBYTEDECODER_HPP)
#define LIBMAUS2_SUFFIXSORT_GAPARRAYBYTEDECODER_HPP

#include <libmaus2/suffixsort/GapArrayByteOverflowKeyAccessor.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/util/iterator.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		struct GapArrayByteDecoder
		{
			typedef GapArrayByteDecoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uint8_t const * G;
			uint64_t const gsize;

			libmaus2::aio::InputStreamInstance::unique_ptr_type pCIS;

			uint64_t sparsecnt;

			libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type pSGI;

			uint64_t offset;

			static libmaus2::aio::InputStreamInstance::unique_ptr_type openSparseFile(std::string const & filename)
			{
				libmaus2::aio::InputStreamInstance::unique_ptr_type tCIS(
					new libmaus2::aio::InputStreamInstance(filename)
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

			static libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type openWordPairFile(
				std::istream & CIS, uint64_t const n, uint64_t const offset
			)
			{
				libmaus2::suffixsort::GapArrayByteOverflowKeyAccessor acc(CIS);
				libmaus2::util::ConstIterator<libmaus2::suffixsort::GapArrayByteOverflowKeyAccessor,uint64_t> ita(&acc,0);
				libmaus2::util::ConstIterator<libmaus2::suffixsort::GapArrayByteOverflowKeyAccessor,uint64_t> ite(&acc,n);

				libmaus2::util::ConstIterator<libmaus2::suffixsort::GapArrayByteOverflowKeyAccessor,uint64_t> itc = std::lower_bound(ita,ite,offset);

				uint64_t const el = itc-ita;
				uint64_t const restel = n - el;

				CIS.clear();
				CIS.seekg( el * 2 * sizeof(uint64_t), std::ios::beg );

				libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type tSGI(
					new libmaus2::aio::SynchronousGenericInput<uint64_t>(CIS,1024,2*restel)
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
				assert ( offset <= gsize );
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
