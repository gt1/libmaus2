/**
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
**/

#if ! defined(SUCCINCTFACTORLIST_HPP)
#define SUCCINCTFACTORLIST_HPP

#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/bitio/getBit.hpp>
#include <libmaus/bitio/putBit.hpp>
#include <libmaus/bitio/getBits.hpp>
#include <libmaus/bitio/putBits.hpp>
#include <libmaus/math/numbits.hpp>
#include <libmaus/util/Histogram.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		template<typename _iterator>
		struct SuccinctFactorList;
		
		template<typename _iterator>
		struct SuccinctFactorListContext
		{
			typedef _iterator iterator;
			typedef SuccinctFactorListContext<iterator> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			static unsigned int computeSmallThresPreLog(uint64_t const n, uint64_t const b)
			{
				return (3*::libmaus::math::numbits(n)+b-1) / b;
			}

			static unsigned int computeSmallThres(uint64_t const logn, uint64_t const b)
			{
				return (3*logn+b-1)/ b;
			}
		
			// text
			uint64_t const n;
			uint16_t const logn;
			uint16_t const b;
			uint16_t const smallthres;
			uint16_t const smalllen;
			// text iterator
			iterator const I;
			uint64_t const sentinel;
		
			SuccinctFactorListContext(
				uint64_t const rn,
				unsigned int const rb,
				iterator const rI,
				uint64_t const rsentinel
			)
			: n(rn),
			  logn(::libmaus::math::numbits(rn)), 
			  b(rb), 
			  smallthres(computeSmallThres(logn,b)),
			  smalllen( ::libmaus::math::numbits(smallthres) ),
			  I(rI),
			  sentinel(rsentinel)
			{
			
			}
		};
		
		template<typename _iterator>
		struct SuccinctFactorListBlock
		{
			//private:
			template<typename iterator> friend struct SuccinctFactorList;
			
			typedef _iterator iterator;
			typedef SuccinctFactorListBlock<iterator> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef uint32_t bit_ptr_type;
			
			// bit block
			::libmaus::autoarray::AutoArray < uint64_t > B;
			bit_ptr_type outptr;
			bit_ptr_type inptr;

			SuccinctFactorListContext<iterator> const * context;
			
			uint64_t byteSize() const
			{
				return 
					B.byteSize() +
					2*sizeof(uint16_t) +
					sizeof(SuccinctFactorListContext<iterator> const *);
			}

			enum storage_type { storage_explicit, storage_implicit };
			
			storage_type getType(uint64_t const ptr) const
			{
				if ( ::libmaus::bitio::getBit(B.get(),ptr) )
					return storage_implicit;
				else
					return storage_explicit;
			}

			struct const_iterator
			{
				uint64_t ptr;
				SuccinctFactorListBlock<iterator> const * owner;
			
				const_iterator() : ptr(0), owner(0) {}	
				const_iterator(uint64_t const rptr, SuccinctFactorListBlock<iterator> const * rowner)
				: ptr(rptr), owner(rowner) {}
				const_iterator(const_iterator const & o)
				: ptr(o.ptr), owner(o.owner) {}
				
				bool operator==(const_iterator const & o) const
				{
					return ptr == o.ptr && owner == o.owner;
				}
				bool operator!=(const_iterator const & o) const
				{
					return !(*this == o);
				}
				
				// prefix increment
				const_iterator operator++()
				{
					assert ( ptr != owner->inptr );
					ptr = owner->skip(ptr);
					return *this;
				}
				
				// postfix increment
				const_iterator operator++(int)
				{
					const_iterator copy(*this);
					++(*this);
					return copy;
				}
				
				storage_type getType() const
				{
					return owner->getType(ptr);
				}
				
				void print(std::ostream & out) const
				{
					owner->printSingle(out,ptr);
				}
				
				uint64_t peek() const
				{
					return owner->peek(ptr);
				}

				uint64_t peekPre() const
				{
					return owner->peekPre(ptr);
				}
				
				uint64_t size() const
				{
					return owner->size(ptr);
				}
				
				uint64_t operator[](uint64_t const i) const
				{
					return (*owner)(ptr,i);
				}
				
				bool operator<(const_iterator const & o) const
				{
					uint64_t l0 = size(), l1 = o.size();
					uint64_t i0 = 0, i1 = 0;
					
					// compare until we reach the end of one factor
					for ( ; i0 < l0 && i1 < l1 ; i0++, i1++ )
					{
						uint64_t const c0 = (*this)[i0];
						uint64_t const c1 = o[i1];
						if ( c0 != c1 )
							return c0 < c1;
					}
					
					// reduce length			
					l0 -= i0;
					l1 -= i1;
					
					return l0 > l1;
				}
			};
			
			const_iterator begin() const
			{
				return const_iterator(outptr,this);
			}
			const_iterator end() const
			{
				return const_iterator(inptr,this);
			}
			
			inline uint64_t getBits() const
			{
				return B.size() << 6; // * 64
			}

			bool pushImplicit(uint64_t i, uint64_t j, uint64_t k)
			{
				if ( (1ull+3ull*context->logn) > getBits()-inptr )
				{
					return false;
				}
				else
				{
					::libmaus::bitio::putBits(B.get(),inptr,1   /*numbits*/,1/*implicit*/); inptr += 1;
					::libmaus::bitio::putBits(B.get(),inptr,context->logn/*numbits*/,i/*implicit*/); inptr += context->logn;
					::libmaus::bitio::putBits(B.get(),inptr,context->logn/*numbits*/,j/*implicit*/); inptr += context->logn;
					::libmaus::bitio::putBits(B.get(),inptr,context->logn/*numbits*/,k/*implicit*/); inptr += context->logn;
					return true;
				}
			}

			bool pushImplicit(uint64_t i, uint64_t j)
			{
				return pushImplicit(i,j,j);
			}

			bool pushExplicit(uint64_t i, uint64_t j, uint64_t k)
			{
				if ( (1+2*context->smalllen+(j-i)*context->b) > getBits()-inptr )
				{
					return false;
				}
				else
				{
					::libmaus::bitio::putBits(B.get(),inptr,1       /*numbits*/,0   /*explicit*/); inptr += 1;
					::libmaus::bitio::putBits(B.get(),inptr,context->smalllen/*numbits*/,j-i /*explicit*/); inptr += context->smalllen;
					::libmaus::bitio::putBits(B.get(),inptr,context->smalllen/*numbits*/,k-i /*explicit*/); inptr += context->smalllen;
					
					for ( uint64_t l = i; l < j; ++l )
					{
						::libmaus::bitio::putBits(B.get(),inptr,context->b,context->I[l]); 
						inptr += context->b;
					}
					
					return true;
				}
			}

			uint64_t size(uint64_t ptr) const
			{
				if ( ::libmaus::bitio::getBit(B.get(),ptr++) )
				{
					uint64_t const i = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					uint64_t const j = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					return j-i;
				}
				else
				{
					uint64_t const len = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;
					return len;
				}
			}


			uint64_t cycle(uint64_t ptr)
			{
				// implicit
				if ( ::libmaus::bitio::getBit(B.get(),ptr++) )
				{
					uint64_t const i = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					uint64_t const j = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					uint64_t const k = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); 
					
					if ( k == i )
						::libmaus::bitio::putBits(B.get(),ptr,context->logn/*numbits*/,j-1/*implicit*/);
					else
						::libmaus::bitio::putBits(B.get(),ptr,context->logn/*numbits*/,k-1/*implicit*/);
								
					ptr += context->logn;
					
					return ptr;
				}
				else
				{
					uint64_t const len = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;
					uint64_t const shift = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); 
					
					if ( shift == 0 )
						::libmaus::bitio::putBits(B.get(),ptr,context->smalllen/*numbits*/,len-1/*implicit*/);
					else
						::libmaus::bitio::putBits(B.get(),ptr,context->smalllen/*numbits*/,shift-1/*implicit*/);

					ptr += context->smalllen;			
					ptr += len*context->b;
					
					return ptr;
				}
			}

			public:
			#if 0
			static unsigned int computeSmallThres(uint64_t const logn, uint64_t const b)
			{
				return (3*logn+b-1)/ b;
			}
			#endif
			
			SuccinctFactorListBlock(
				SuccinctFactorListContext<iterator> const * rcontext,
				uint64_t const bufsize
			)
			: 
				B(bufsize,false), 
				outptr(0), 
				inptr(0),
				context(rcontext)
			{
				// std::cerr << "smalthres " << smallthres << " context->smalllen=" << context->smalllen << std::endl;	
			}

			bool push(uint64_t const i, uint64_t const j, uint64_t const k)
			{
				if ( j-i <= context->smallthres )
					return pushExplicit(i,j,k);
				else
					return pushImplicit(i,j,k);
			}
			
			bool pushExplicitWord(uint64_t const word, uint64_t const len)
			{
				if ( (1+2*context->smalllen+(len)*context->b) > getBits()-inptr )
				{
					return false;
				}
				else
				{
					::libmaus::bitio::putBits(B.get(),inptr,1       /*numbits*/,0   /*explicit*/); inptr += 1;
					::libmaus::bitio::putBits(B.get(),inptr,context->smalllen/*numbits*/,len /*explicit*/); inptr += context->smalllen;
					::libmaus::bitio::putBits(B.get(),inptr,context->smalllen/*numbits*/,len /*explicit*/); inptr += context->smalllen;
					::libmaus::bitio::putBits(B.get(),inptr,len*context->b,word); 
					inptr += len*context->b;
					
					return true;
				}
			}

			void cycleAll()
			{
				uint64_t ptr = outptr;
				while ( ptr != inptr )
					ptr = cycle(ptr);
			}
			
			void cycle()
			{
				assert ( outptr != inptr );
				cycle(outptr);
			}
			
			void print(std::ostream & out) const
			{
				out << "buf" << std::endl;
				uint64_t ptr = outptr;
				
				while ( ptr != inptr )
				{
					ptr = printSingle(out,ptr);
					out << "\n";
				}
			}

			uint64_t printSingle(std::ostream & out) const
			{
				return printSingle(out,outptr);
			}
			
			uint64_t skip(uint64_t ptr) const
			{
				assert ( ptr != inptr );

				// implicit
				if ( ::libmaus::bitio::getBit(B.get(),ptr++) )
				{
					ptr += 3*context->logn;
				}
				// explicit
				else
				{
					uint64_t const len = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/);
					ptr += (2*context->smalllen + len*context->b);
				}

				return ptr;
				
			}

			template<typename iterator>
			void peekHistory(iterator & it) const
			{
				uint64_t ptr = outptr;
				
				while ( ptr != inptr )
				{
					it [ peek(ptr) ]++;
					ptr = skip(ptr);
				}
			}

			uint64_t printSingle(std::ostream & out, uint64_t ptr) const
			{
				assert ( ptr != inptr );
				
				// implicit
				if ( ::libmaus::bitio::getBit(B.get(),ptr++) )
				{
					uint64_t const i = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					uint64_t const j = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					uint64_t const k = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					out 
						<< "implicit(" << i << "," << j << "," << k << ",";

					for ( uint64_t l = i; l < j; ++l )
						out << static_cast<char>(context->I[l]);
						
					out << ",";
					for ( uint64_t l = i; l < k; ++l )
						out << static_cast<char>(context->I[l]);
					out << ",";
					for ( uint64_t l = k; l < j; ++l )
						out << static_cast<char>(context->I[l]);
						
					out
						<< ")";
				}
				// explicit
				else
				{
					uint64_t const len = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;
					uint64_t const shift = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;
					out << "explicit(" << len << "," << shift << ",";

					for ( uint64_t l = 0; l < len; ++l )
					{
						uint64_t const c = ::libmaus::bitio::getBits(B.get(),ptr + (l*context->b),context->b/*numbits*/);
						out << static_cast<char>(c);				
					}

					out << ",";
					for ( uint64_t l = 0; l < shift; ++l )
					{
						uint64_t const c = ::libmaus::bitio::getBits(B.get(),ptr + (l*context->b),context->b/*numbits*/);
						out << static_cast<char>(c);				
					}
					out << ",";
					for ( uint64_t l = shift; l < len; ++l )
					{
						uint64_t const c = ::libmaus::bitio::getBits(B.get(),ptr + (l*context->b),context->b/*numbits*/);
						out << static_cast<char>(c);				
					}
					
					ptr += len*context->b;
					
					out << ")";
				}

				return ptr;
			}
			
			// return reverse of this block
			unique_ptr_type reverse() const
			{
				unique_ptr_type P = unique_ptr_type(new this_type(context,B.size()));

				uint64_t ptr = outptr;
				uint64_t pptr = inptr - outptr;
				
				while ( ptr != inptr )
				{
					// implicit
					if ( ::libmaus::bitio::getBit(B.get(),ptr++) )
					{
						uint64_t const i = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
						uint64_t const j = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
						uint64_t const k = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
						
						pptr -= (1 + 3*context->logn);
						::libmaus::bitio::putBits(P->B.get(),pptr,1             /*numbits*/,1/*implicit*/);
						::libmaus::bitio::putBits(P->B.get(),pptr+1,context->logn        /*numbits*/,i/*implicit*/);
						::libmaus::bitio::putBits(P->B.get(),pptr+1+context->logn,context->logn   /*numbits*/,j/*implicit*/);
						::libmaus::bitio::putBits(P->B.get(),pptr+1+2*context->logn,context->logn /*numbits*/,k/*implicit*/);
					}
					// explicit
					else
					{
						uint64_t const len = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;
						uint64_t const shift = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;
						
						pptr -= (1 + 2*context->smalllen + len*context->b);

						::libmaus::bitio::putBits(P->B.get(),pptr,           1        /*numbits*/,0/*explicit*/);
						::libmaus::bitio::putBits(P->B.get(),pptr+1,         context->smalllen /*numbits*/,len);
						::libmaus::bitio::putBits(P->B.get(),pptr+1+context->smalllen,context->smalllen /*numbits*/,shift);
						
						for ( uint64_t i = 0; i < len; ++i )
						{
							uint64_t const c = ::libmaus::bitio::getBits(B.get(),ptr,context->b/*numbits*/); ptr += context->b;
							::libmaus::bitio::putBits(P->B.get(),pptr+1+2*context->smalllen+i*context->b,context->b /*numbits*/,c);
						}
					}
				}
				
				assert ( pptr == 0 );
				
				P->outptr = 0;
				P->inptr = (inptr-outptr);
				
				return UNIQUE_PTR_MOVE(P);
			}
			
			// look at last character of front element
			uint64_t peek() const
			{
				return peek(outptr);
			}

			// look at last character of element denoted by ptr
			uint64_t peek(uint64_t ptr) const
			{
				// implicit
				if ( ::libmaus::bitio::getBit(B.get(),ptr++) )
				{
					uint64_t const i = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					//uint64_t const j = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); 
					ptr += context->logn;
					uint64_t const k = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					
					if ( k == i )
						return context->sentinel;
					else
						return context->I[k-1];
				}
				// explicit
				else
				{
					// uint64_t const len = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); 
					ptr += context->smalllen;
					uint64_t const shift = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;
					
					if ( ! shift )
						return context->sentinel;
					else		
						return ::libmaus::bitio::getBits(B.get(),ptr + (shift-1)*context->b,context->b/*numbits*/);
				}
			}

			// look at last character of front element
			uint64_t peekPre() const
			{
				return peekPre(outptr);
			}

			// 
			uint64_t peekPre(uint64_t ptr) const
			{
				// implicit
				if ( ::libmaus::bitio::getBit(B.get(),ptr++) )
				{
					//uint64_t const i = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); 
					ptr += context->logn;
					//uint64_t const j = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); 
					ptr += context->logn;
					uint64_t const k = ::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					
					return context->I[k];
				}
				// explicit
				else
				{
					// uint64_t const len = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); 
					ptr += context->smalllen;
					uint64_t const shift = ::libmaus::bitio::getBits(B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;
					
					return ::libmaus::bitio::getBits(B.get(),ptr + (shift)*context->b,context->b/*numbits*/);
				}
			}

			// get single character
			uint64_t operator()(uint64_t ptr, uint64_t i) const
			{
				// implicit
				if ( ::libmaus::bitio::getBit(B.get(),ptr++) )
				{
					return context->I[::libmaus::bitio::getBits(B.get(),ptr,context->logn/*numbits*/)+i];
				}
				// explicit
				else
				{
					return ::libmaus::bitio::getBits(B.get(),ptr+2*context->smalllen+i*context->b,context->b);
				}
			}

			// pop front element
			bool pop()
			{
				// implicit
				if ( ::libmaus::bitio::getBit(B.get(),outptr++) )
				{
					outptr += 3*context->logn;
				}
				// explicit
				else
				{
					uint64_t const len = ::libmaus::bitio::getBits(B.get(),outptr,context->smalllen/*numbits*/);   
					outptr += 2*context->smalllen + len*context->b;
				}
				
				return outptr == inptr;
			}
			
			// copy element from O to this
			bool copy(this_type const & O)
			{
				assert ( O.outptr != O.inptr );
				uint64_t ptr = O.outptr;
						
				// implicit
				if ( ::libmaus::bitio::getBit(O.B.get(),ptr++) )
				{
					if ( getBits()-inptr < 1ull+3ull*context->logn )
						return false;

					uint64_t const i = ::libmaus::bitio::getBits(O.B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					uint64_t const j = ::libmaus::bitio::getBits(O.B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					uint64_t const k = ::libmaus::bitio::getBits(O.B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;

					::libmaus::bitio::putBits(B.get(),inptr,1    /*numbits*/,1/*implicit*/); inptr += 1;
					::libmaus::bitio::putBits(B.get(),inptr,context->logn /*numbits*/,i/*implicit*/); inptr += context->logn;
					::libmaus::bitio::putBits(B.get(),inptr,context->logn /*numbits*/,j/*implicit*/); inptr += context->logn;
					::libmaus::bitio::putBits(B.get(),inptr,context->logn /*numbits*/,k/*implicit*/); inptr += context->logn;
					
					return true;
				}
				// explicit
				else
				{
					uint64_t const len = ::libmaus::bitio::getBits(O.B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;
					uint64_t const shift = ::libmaus::bitio::getBits(O.B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;

					if ( getBits()-inptr < 1+2*context->smalllen+context->b*len )
						return false;
					
					::libmaus::bitio::putBits(B.get(),inptr,1        /*numbits*/,   0  /*explicit*/); inptr += 1;
					::libmaus::bitio::putBits(B.get(),inptr,context->smalllen /*numbits*/,   len/*explicit*/); inptr += context->smalllen;
					::libmaus::bitio::putBits(B.get(),inptr,context->smalllen    /*numbits*/,shift/*explicit*/); inptr += context->smalllen;
					
					for ( uint64_t i = 0; i < len; ++i )
					{
						uint64_t const c = ::libmaus::bitio::getBits(O.B.get(),ptr,context->b/*numbits*/); ptr += context->b;
						::libmaus::bitio::putBits(B.get(),inptr,context->b /*numbits*/,c/*explicit*/); inptr += context->b;		
					}
					
					return true;
				}
				
			}

			// copy element from O to this
			bool copyReset(this_type const & O)
			{
				assert ( O.outptr != O.inptr );
				uint64_t ptr = O.outptr;
						
				// implicit
				if ( ::libmaus::bitio::getBit(O.B.get(),ptr++) )
				{
					if ( getBits()-inptr < 1ull+3ull*context->logn )
						return false;

					uint64_t const i = ::libmaus::bitio::getBits(O.B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					uint64_t const j = ::libmaus::bitio::getBits(O.B.get(),ptr,context->logn/*numbits*/); ptr += context->logn;
					ptr += context->logn; // skip k

					::libmaus::bitio::putBits(B.get(),inptr,1    /*numbits*/,1/*implicit*/); inptr += 1;
					::libmaus::bitio::putBits(B.get(),inptr,context->logn /*numbits*/,i/*implicit*/); inptr += context->logn;
					::libmaus::bitio::putBits(B.get(),inptr,context->logn /*numbits*/,j/*implicit*/); inptr += context->logn;
					::libmaus::bitio::putBits(B.get(),inptr,context->logn /*numbits*/,j/*implicit*/); inptr += context->logn;
					
					return true;
				}
				// explicit
				else
				{
					uint64_t const len = ::libmaus::bitio::getBits(O.B.get(),ptr,context->smalllen/*numbits*/); ptr += context->smalllen;
					// skip shift
					ptr += context->smalllen;

					if ( getBits()-inptr < 1+2*context->smalllen+context->b*len )
						return false;
					
					::libmaus::bitio::putBits(B.get(),inptr,1        /*numbits*/,   0  /*explicit*/); inptr += 1;
					::libmaus::bitio::putBits(B.get(),inptr,context->smalllen /*numbits*/,   len/*explicit*/); inptr += context->smalllen;
					::libmaus::bitio::putBits(B.get(),inptr,context->smalllen    /*numbits*/,len/*explicit*/); inptr += context->smalllen;
					
					for ( uint64_t i = 0; i < len; ++i )
					{
						uint64_t const c = ::libmaus::bitio::getBits(O.B.get(),ptr,context->b/*numbits*/); ptr += context->b;
						::libmaus::bitio::putBits(B.get(),inptr,context->b /*numbits*/,c/*explicit*/); inptr += context->b;		
					}
					
					return true;
				}
				
			}
		};

		template<typename _iterator>
		struct SuccinctFactorList
		{
			typedef _iterator iterator;
			
			typedef SuccinctFactorListContext<iterator> context_type;
			typedef typename context_type::unique_ptr_type context_ptr_type;

			typedef SuccinctFactorListBlock<iterator> block_type;
			typedef typename block_type::unique_ptr_type block_ptr_type;
			
			typedef SuccinctFactorList<iterator> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			// static unsigned int const bufsize = 8*1024;
			static unsigned int const prebufsize = 512u;
			static unsigned int const bitptrbits = 8*sizeof(typename block_type::bit_ptr_type);
			static unsigned int const bitptrval = 1ull << bitptrbits;
			static unsigned int const bufsize = 
				((prebufsize * 64ull) >= (1ull << 16)) ? 
				(((1ull << 16) >> 6)-1) : prebufsize;
			// static unsigned int const bufsize = 2;

			context_ptr_type context;

			/*
			iterator const I;
			uint64_t const n;
			unsigned int const logn;
			unsigned int const b;
			uint64_t const sentinel;
			*/
			
			::libmaus::autoarray::AutoArray < 	block_ptr_type > B;
			uint32_t blow;
			uint32_t bhigh;
						
			uint64_t byteSize() const
			{
				uint64_t s = sizeof(context) + 2*sizeof(uint32_t);
				for ( uint64_t i = blow; i < bhigh; ++i )
					s += B[i]->byteSize();
				return s;
			}

			std::pair < uint64_t, uint64_t > getStorageHistogram()
			{
				uint64_t type_explicit = 0, type_implicit = 0;
				for ( const_iterator ita = begin(); ita != end(); ++ita )
					if ( ita.getType() == block_type::storage_explicit )
						type_explicit++;
					else
						type_implicit++;
				
				return std::pair<uint64_t,uint64_t>(type_explicit,type_implicit);
			}
			
			struct const_iterator
			{
				SuccinctFactorList<iterator> const * owner;
				uint64_t blockptr;
				typename block_type::const_iterator subit;
				
				const_iterator() : owner(0), blockptr(0), subit() {}
				const_iterator(SuccinctFactorList<iterator> const * rowner, uint64_t const rblockptr, typename block_type::const_iterator const & rsubit)
				: owner(rowner), blockptr(rblockptr), subit(rsubit)
				{}
				const_iterator(const_iterator const & o)
				: owner(o.owner), blockptr(o.blockptr), subit(o.subit)
				{}
				
				const_iterator & operator++()
				{
					assert ( blockptr != owner->bhigh );
					assert ( subit != owner->B[blockptr]->end() );
					
					++subit;

					while ( 
						blockptr != owner->bhigh 
						&& 
						subit == owner->B[blockptr]->end()
					)
					{
						blockptr++;
						if ( blockptr != owner->bhigh )
							subit = owner->B[blockptr]->begin();
						else
							subit = typename block_type::const_iterator(0,0);
					}
					
					return *this;			
				}
				const_iterator operator++(int)
				{
					const_iterator copy = *this;
					++(*this);
					return copy;
				}
				
				void print(std::ostream & out) const
				{
					subit.print(out);
				}
				
				bool operator==(const_iterator const & o) const
				{
					return owner==o.owner && blockptr==o.blockptr && subit==o.subit;
				}
				
				bool operator!=(const_iterator const & o) const
				{
					return !(*this==o);
				}
				
				uint64_t peek() const
				{
					return subit.peek();
				}

				uint64_t peekPre() const
				{
					return subit.peekPre();
				}

				uint64_t size() const
				{
					return subit.size();
				}
				
				uint64_t operator[](uint64_t const i) const
				{
					return subit[i];
				}

				bool operator<(const_iterator const & o) const
				{
					return subit < o.subit;
				}

				typename block_type::storage_type getType() const
				{
					return subit.getType();
				}
			};

			const_iterator begin() const
			{
				return const_iterator(this,blow,B[blow]->begin());
			}
			const_iterator end() const
			{
				return const_iterator(this,bhigh,typename block_type::const_iterator(0,0));
			}

			uint64_t size() const
			{
				uint64_t s = 0;
				for ( const_iterator it = begin(); it != end(); ++it )
					s++;
				return s;
			}
			::libmaus::util::Histogram::unique_ptr_type sizeHistogram() const
			{
				::libmaus::util::Histogram::unique_ptr_type hist(new ::libmaus::util::Histogram);
				for ( const_iterator it = begin(); it != end(); ++it )
					(*hist)(it.size());
				return UNIQUE_PTR_MOVE(hist);
			}
			
			void extendBlockArray()
			{
				uint64_t const nb = B.size() ? 2*B.size() : 1;
				::libmaus::autoarray::AutoArray < 	block_ptr_type > NB(nb);
				
				for ( uint64_t i = 0; i < B.size(); ++i )
					NB[i] = UNIQUE_PTR_MOVE(B[i]);
				
				B = NB;
			}
			void pushBlock()
			{
				if ( bhigh == B.size() )
					extendBlockArray();
				B[bhigh++] = UNIQUE_PTR_MOVE(block_ptr_type(new block_type(context.get(),bufsize)));
			}
			
			static uint64_t computeLogN(uint64_t const n)
			{
				return ::libmaus::math::numbits(n);
			}
			
			SuccinctFactorList(
				iterator rI, 
				uint64_t const rn, 
				uint64_t const rb, 
				uint64_t const rsentinel
			)
			: context(new context_type(rn,rb,rI,rsentinel)),
			  B(), blow(0), bhigh(0)
			{
			
			}
			
			void pushExplicitWord(uint64_t const word, uint64_t const len)
			{
				if ( (blow==bhigh) )
					pushBlock();
					
				if ( ! B[bhigh-1]->pushExplicitWord(word,len) )
				{
					pushBlock();
					bool const ok = B[bhigh-1]->pushExplicitWord(word,len);
					assert ( ok );
				}
			}
			
			void push(uint64_t const i, uint64_t const j, uint64_t const k)
			{
				if ( (blow==bhigh) )
					pushBlock();
				if ( ! B[bhigh-1]->push(i,j,k) )
				{
					pushBlock();
					bool const ok = B[bhigh-1]->push(i,j,k);
					assert ( ok );
				}
			}

			void push(uint64_t const i, uint64_t const j)
			{
				push(i,j,j);
			}
			
			void print(std::ostream & out) const
			{
				for ( uint64_t i = blow; i < bhigh; ++i )
					B[i]->print(out);
			}

			void printSingle(std::ostream & out) const
			{
				assert ( !empty() );
				B[blow]->printSingle(out);
			}
			
			std::string printSingle() const
			{
				std::ostringstream out;
				printSingle(out);
				return out.str();
			}

			void cycleAll()
			{
				for ( uint64_t i = blow; i < bhigh; ++i )
					B[i]->cycleAll();
			}
			
			void reverse()
			{
				for ( uint64_t i = 0; i < (bhigh-blow)/2; ++i )
				{
					block_ptr_type R0 = B[blow+i]->reverse();
					block_ptr_type R1 = B[bhigh-i-1]->reverse();
					B[blow+i] = UNIQUE_PTR_MOVE(R1);
					B[bhigh-i-1] = UNIQUE_PTR_MOVE(R0);
				}
				if ( (bhigh-blow) & 1 )
					B[blow+(bhigh-blow)/2] = UNIQUE_PTR_MOVE(B[blow+(bhigh-blow)/2]->reverse());
			}

			template<typename iterator>
			void peekHistory(iterator & it)
			{
				for ( uint64_t i = blow; i < bhigh; ++i )
					B[i]->peekHistory(it);
			}
			
			uint64_t peek()
			{
				if ( blow == bhigh )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "peek() called on empty SuccinctFactorList." << std::endl;
					se.finish();
					throw se;
				}
				else
				{
					return B[blow]->peek();
				}
			}
			uint64_t peekPre()
			{
				if ( blow == bhigh )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "peek() called on empty SuccinctFactorList." << std::endl;
					se.finish();
					throw se;
				}
				else
				{
					return B[blow]->peekPre();
				}
			}
			void cycle()
			{
				if ( blow == bhigh )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "cycle() called on empty SuccinctFactorList." << std::endl;
					se.finish();
					throw se;
				}
				else
				{
					return B[blow]->cycle();
				}
				
			}
			void pop()
			{
				if ( blow == bhigh )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "pop() called on empty SuccinctFactorList." << std::endl;
					se.finish();
					throw se;
				}
				else
				{
					if ( B[blow]->pop() )
						B[blow++].reset();
				}
			}
			bool empty() const
			{
				return blow==bhigh;
			}
			
			// copy element from O to this
			void copy(this_type const & O)
			{
				if ( blow == bhigh )
					pushBlock();
				while ( ! B[bhigh-1]->copy(*(O.B[O.blow])) )
					pushBlock();
			}
			void copyReset(this_type const & O)
			{
				if ( blow == bhigh )
					pushBlock();
				while ( ! B[bhigh-1]->copyReset(*(O.B[O.blow])) )
					pushBlock();
			}
		};
	}
}
#endif
