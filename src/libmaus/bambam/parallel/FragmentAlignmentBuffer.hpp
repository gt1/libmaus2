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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFER_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_FRAGMENTALIGNMENTBUFFER_HPP

#include <libmaus/bambam/parallel/FragmentAlignmentBufferFragment.hpp>
	
namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{		
			struct FragmentAlignmentBuffer
			{
				typedef FragmentAlignmentBuffer this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
				
				uint64_t id;
				uint64_t subid;
				
				libmaus::autoarray::AutoArray<FragmentAlignmentBufferFragment::unique_ptr_type> A;
				libmaus::autoarray::AutoArray<uint64_t ,libmaus::autoarray::alloc_type_c> O;
				libmaus::autoarray::AutoArray<uint8_t *,libmaus::autoarray::alloc_type_c> P;
				uint64_t const pointermult;

				std::vector<size_t> OSVO;
				
				static size_t multSize()
				{
					return
						libmaus::math::lcm(
							libmaus::math::lcm(
								sizeof(uint64_t),
								sizeof(size_t)
							),
							sizeof(uint8_t *)
						);
				}
				
				FragmentAlignmentBuffer(size_t const numbuffers, uint64_t const rpointermult)
				: id(0), subid(0), A(numbuffers), pointermult(rpointermult), OSVO(A.size()+1)
				{
					for ( size_t i = 0; i < numbuffers; ++i )
					{
						FragmentAlignmentBufferFragment::unique_ptr_type tptr(new FragmentAlignmentBufferFragment);
						A[i] = UNIQUE_PTR_MOVE(tptr);
					}
				}
				
				size_t size() const
				{
					return A.size();
				}
				
				FragmentAlignmentBufferFragment * operator[](size_t const i)
				{
					return A[i].get();
				}
				
				void checkPointerSpace(size_t const n)
				{
					if ( n > O.size() )
						O.resize(n);
					if ( n * pointermult > P.size() )
						P.resize(n*pointermult);
				}
				
				std::vector<size_t> & getOffsetStartVector()
				{
					return OSVO;
				}
				
				uint64_t getOffsetStartIndex(uint64_t const index) const
				{
					return OSVO.at(index);
				}

				size_t getNumAlignments(uint64_t const index) const
				{
					return OSVO.at(index+1)-OSVO.at(index);
				}
				
				uint64_t * getOffsetStart(uint64_t const index)
				{
					return (O.end() - OSVO.at(size())) + OSVO.at(index);
				}
				
				
				void rewritePointers(uint64_t const index)
				{
					uint64_t const * OO = getOffsetStart(0);
					
					for ( uint64_t i = OSVO.at(index); i < OSVO.at(index+1); ++i )
						P[i] = A[index]->A.begin() + OO[i];
				}
				
				std::pair<uint8_t *, uint32_t> at(uint64_t const i)
				{
					uint8_t * p = P[i];
					uint32_t const l =
						(static_cast<uint32_t>(p[0]) << 0 ) |
						(static_cast<uint32_t>(p[1]) << 8 ) |
						(static_cast<uint32_t>(p[2]) << 16) |
						(static_cast<uint32_t>(p[3]) << 24) ;
					return std::pair<uint8_t *, uint32_t>(p+4,l);
				}
				
				
				std::pair<uint8_t *, uint32_t> at(uint64_t const i, uint64_t const j)
				{
					uint64_t const off = getOffsetStart(i)[j];
					uint8_t * p = A[i]->A.begin() + off;

					uint32_t const l =
						(static_cast<uint32_t>(p[0]) << 0 ) |
						(static_cast<uint32_t>(p[1]) << 8 ) |
						(static_cast<uint32_t>(p[2]) << 16) |
						(static_cast<uint32_t>(p[3]) << 24) ;
					return std::pair<uint8_t *, uint32_t>(p+4,l);
				}
				
				void rewritePointers()
				{
					for ( uint64_t i = 0; i < size(); ++i )
						rewritePointers(i);
				}
								
				void reset()
				{
					for ( size_t i = 0; i < size(); ++i )
						A[i]->reset();
				}

				void getLinearOutputFragments(
					uint64_t const maxblocksize, std::vector<std::pair<uint8_t *,uint8_t *> > & V
				)
				{
					V.resize(0);
					for ( size_t i = 0; i < size(); ++i )
						A[i]->getLinearOutputFragments(maxblocksize,V);
				}
			};
		}
	}
}
#endif
