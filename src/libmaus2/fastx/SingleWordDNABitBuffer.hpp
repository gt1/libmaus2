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
#if ! defined(LIBMAUS_FASTX_SINGLEWORDDNABITBUFFER_HPP)
#define LIBMAUS_FASTX_SINGLEWORDDNABITBUFFER_HPP

#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bitio/getBits.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/math/lowbits.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct SingleWordDNABitBuffer
		{
			typedef uint64_t data_type;
			typedef SingleWordDNABitBuffer this_type;

			static unsigned int getMaxBases()
			{
				return ( sizeof(data_type) * 8 ) / 2;
			}

			unsigned int const width;
			unsigned int const width2;
			unsigned int const width22;
			data_type const mask;
			data_type buffer;

			unsigned int getMiddleSym() const
			{
				return (buffer >> (((width-1)>>1)<<1)) & 0x3;
			}
			
			unsigned int getHalfBits() const
			{
				return (((width-(width&1))>>1) << 1);
			}
			
			data_type getLowHalf() const
			{
				return buffer & ::libmaus2::math::lowbits( getHalfBits() );
			}

			data_type getLowHalfDownShiftOne() const
			{
				return (buffer >> 2) & ::libmaus2::math::lowbits( getHalfBits() );
			}

			data_type getTopHalf() const
			{
				return (buffer >> (getHalfBits()+((width&1)<<1) )) & ::libmaus2::math::lowbits( getHalfBits() );
			}
			
			static data_type middleToEnd(uint64_t const b, unsigned int k)
			{
				this_type B(k);
				B.buffer = b;
				return B.middleToEnd();
			}
			
			static data_type indexToEnd(data_type v, unsigned int k, unsigned int i)
			{
				// position from back
				unsigned int const ii = k-i-1;
				// symbol at position i
				data_type const sym = (v >> (ii<<1))&3;
				// symbols behind position i
				unsigned int const backsyms = ii;
				data_type const back = (v & (::libmaus2::math::lowbits(backsyms<<1))) << 2;
				// symbols in front of position i
				unsigned int const frontsyms = k-(backsyms+1);
				data_type const front = (v & (::libmaus2::math::lowbits(frontsyms<<1) << ((backsyms+1)<<1)));
				data_type const rv = (front | back | sym);
				return rv;
			}
			
			static data_type endToIndex(data_type v, unsigned int k, unsigned int i)
			{
				// position from back
				unsigned int const ii = k-i-1;
				// symbol at position i
				data_type const sym = (v & 0x3) << (ii<<1);
				// number of symbols to be behind i
				unsigned int const backsyms = ii;
				data_type const back = (v >> 2) & ::libmaus2::math::lowbits(backsyms<<1);
				// number of symbols to be in front of i
				unsigned int const frontsyms = k-(backsyms+1);
				data_type const front = (v & (::libmaus2::math::lowbits(frontsyms<<1) << ((backsyms+1)<<1)));
				// combined
				data_type const rv = (front | back | sym);
				return rv;
			}
			
			data_type middleToEnd() const
			{
				return
					(getTopHalf() << (getHalfBits()+2))
					|
					(getLowHalf() << 2)
					|
					getMiddleSym()
					;
			}
			
			data_type getBottomSym() const
			{
				return buffer & ::libmaus2::math::lowbits( 2 );
			}
			
			data_type endToMiddle() const
			{
				return					
					(getTopHalf() << (getHalfBits()+2))
					|
					(getBottomSym() << getHalfBits())
					|
					getLowHalfDownShiftOne();
			}

			static data_type endToMiddle(data_type const b, unsigned int k)
			{
				this_type B(k);
				B.buffer = b;
				return B.endToMiddle();
			}

			data_type getMiddleMask() const
			{
				return
					(::libmaus2::math::lowbits(getHalfBits()) << (getHalfBits()+2))
					|
					::libmaus2::math::lowbits(getHalfBits());
					;
			}

			SingleWordDNABitBuffer(unsigned int const rwidth)
			: width(rwidth), width2(2*width), width22(width2-2), mask(::libmaus2::math::lowbits(2*width)), buffer(0)
			{
				if ( width > getMaxBases() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Cannot handle " << width << " > " << getMaxBases() << " bases in SingleWordDNABitBuffer.";
					se.finish();
					throw se;
				}
			}
			void reset()
			{
				buffer = 0;
			}
			void pushBackUnmasked(data_type const v)
			{
				if ( v >= 4 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "SingleWordDNABitBuffer::pushBackUnmasked(): invalid symbol " << v << std::endl;
					se.finish();
					throw se;
				}
				buffer <<= 2;
				buffer |= v;
			}
			void pushBackMasked(data_type const v)
			{
				if ( v >= 4 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "SingleWordDNABitBuffer::pushBackMasked(): invalid symbol " << v << std::endl;
					se.finish();
					throw se;
				}
				buffer <<= 2;
				buffer |= v;
				buffer &= mask;
			}
			void pushFront(data_type const v)
			{
				if ( v >= 4 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "SingleWordDNABitBuffer::pushFront(): invalid symbol " << v << std::endl;
					se.finish();
					throw se;				}

				buffer >>= 2;
				buffer |= (v << width22);
			}

			static data_type rc(data_type v, unsigned int k)
			{
				data_type r = 0;
				
				for ( unsigned int i = 0; i < k; ++i )
				{
					r <<= 2;
					r |= ((v&3)^3);
					v >>= 2;
				}
				
				return r;
			}

			std::string toString() const
			{
				unsigned int shift = width22;
				data_type pmask = ::libmaus2::math::lowbits(2) << shift;
				std::ostringstream ostr;

				for ( unsigned int i = 0; i < width; ++i, pmask >>= 2, shift -= 2 )
					ostr << ((buffer & pmask) >> shift);
				
				return ostr.str();
			}
		};
	}
}
#endif
