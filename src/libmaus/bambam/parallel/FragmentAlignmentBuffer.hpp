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

#include <libmaus/bambam/DecoderBase.hpp>
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
				
				bool final;
				
				size_t byteSize()
				{
					uint64_t s =
						3*sizeof(uint64_t) +
						1*sizeof(bool) +
						A.byteSize() +
						O.byteSize() +
						P.byteSize() +
						OSVO.size() * sizeof(size_t);
						
					for ( size_t i = 0; i < A.size(); ++i )
						if ( A[i] )
							s += A[i]->byteSize();

					return s;
				}
				
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
				
				std::pair<uint8_t **, uint8_t **> getPointerArray()
				{
					uint64_t const numalgn = OSVO.at(size())-OSVO.at(0);
					return std::pair<uint8_t **, uint8_t **>(P.begin(),P.begin()+numalgn);
				}

				template<typename order_type>
				bool checkSort(order_type const & order)
				{
					std::pair<uint8_t **, uint8_t **> P = getPointerArray();
					uint8_t ** pa = P.first;
					uint8_t ** pb = P.second;
					uint64_t const f = pb-pa;
					bool ok = true;
					for ( uint64_t i = 1; i < f; ++i )
					{
						uint8_t * A = pa[i-1];
						uint8_t * B = pa[i];
						ok = ok && (!(order(B,A)));
					}
					return ok;
				}
				
				struct FragmentAlignmentBufferCopyRequest
				{
					uint64_t * LO;
					FragmentAlignmentBufferFragment * frag;
					uint64_t n;
					uint8_t ** A;
					FragmentAlignmentBuffer * T;
					uint64_t t;
					
					FragmentAlignmentBufferCopyRequest()
					: LO(0), frag(0), n(0), A(0), T(0), t(0)
					{
					
					}
					
					FragmentAlignmentBufferCopyRequest(
						uint64_t * rLO,
						FragmentAlignmentBufferFragment * rfrag,
						uint64_t const rn,
						uint8_t ** rA,
						FragmentAlignmentBuffer * rT,
						uint64_t const rt
					) : LO(rLO), frag(rfrag), n(rn), A(rA), T(rT), t(rt)
					{
					
					}
					
					void dispatch()
					{
						for ( uint64_t i = 0; i < n; ++i )
						{							
							uint32_t const l = libmaus::bambam::DecoderBase::getLEInteger(
								reinterpret_cast<uint8_t const *>(A[i]),
								sizeof(uint32_t));
							uint8_t const * D = A[i] + sizeof(uint32_t);
							
							*(LO++) = frag->getOffset();
							frag->pushAlignmentBlock(D,l);
						}
						T->rewritePointers(t);
					}
				};
				
				void compareBuffers(FragmentAlignmentBuffer & T)
				{
					std::pair<uint8_t **, uint8_t **> P = getPointerArray();
					uint64_t const f = P.second-P.first;
					for ( uint64_t i = 0; i < f; ++i )
					{
						std::pair<uint8_t *, uint32_t> IN = this->at(i);
						std::pair<uint8_t *, uint32_t> OUT = T.at(i);
						assert ( IN.second == OUT.second );
						assert ( memcmp(IN.first,OUT.first,IN.second) == 0 );
					}				
				}
				
				std::vector<FragmentAlignmentBufferCopyRequest> setupCopy(FragmentAlignmentBuffer & T)
				{
					std::pair<uint8_t **, uint8_t **> P = getPointerArray();
					uint64_t const f = P.second-P.first;

					// set up meta data in target buffer
					T.reset();
					T.id = id;
					T.checkPointerSpace(f);
					uint64_t const alperbuf = (f+T.size()-1)/T.size();
					std::vector<size_t> & O = T.getOffsetStartVector();
					assert ( O.size() == T.size() + 1 );
					for ( uint64_t t = 0; t < T.size(); ++t )
					{
						uint64_t const low = std::min(t * alperbuf,f);
						O.at(t) = low;
					}
					O.at(T.size()) = f;

					// produce copy requests
					std::vector<FragmentAlignmentBufferCopyRequest> V;
					for ( uint64_t t = 0; t < T.size(); ++t )
					{
						uint64_t * LO = T.getOffsetStart(t);
						FragmentAlignmentBufferFragment * frag = T[t];
						uint64_t const low = O.at(t);
						uint64_t const high = O.at(t+1);
						uint64_t const n = high-low;
						uint8_t ** A = P.first + low;

						FragmentAlignmentBufferCopyRequest req(LO,frag,n,A,&T,t);
						V.push_back(req);
					}

					return V;				
				}
				
				// copy from this buffer to T
				void copyBuffer(FragmentAlignmentBuffer & T)
				{
					std::vector<FragmentAlignmentBufferCopyRequest> V = setupCopy(T);
					for ( uint64_t i = 0; i < V.size(); ++i )
						V[i].dispatch();
					compareBuffers(T);
				}

				std::pair<uint8_t **, uint8_t **> getAuxPointerArray()
				{
					uint64_t const numalgn = OSVO.at(size())-OSVO.at(0);
					return std::pair<uint8_t **, uint8_t **>(P.begin()+numalgn,P.begin()+(2*numalgn));
				}
				
				FragmentAlignmentBuffer(size_t const numbuffers, uint64_t const rpointermult)
				: id(0), subid(0), A(numbuffers), pointermult(rpointermult), OSVO(A.size()+1), final(false)
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

					final = false;
					id = 0;
					subid = 0;
				}

				void getLinearOutputFragments(
					uint64_t const maxblocksize, std::vector<std::pair<uint8_t *,uint8_t *> > & V
				)
				{
					V.resize(0);
					for ( size_t i = 0; i < size(); ++i )
						A[i]->getLinearOutputFragments(maxblocksize,V);
				}

				void getLinearOutputFragments(std::vector<std::pair<uint8_t *,uint8_t *> > & V)
				{
					V.resize(0);
					for ( size_t i = 0; i < size(); ++i )
						A[i]->getLinearOutputFragments(V);
				}
				
				uint64_t getFill() const
				{
					uint64_t c = 0;
					for ( size_t i = 0; i < size(); ++i )
						c += A[i]->f;
					return c;
				}
			};
		}
	}
}
#endif
