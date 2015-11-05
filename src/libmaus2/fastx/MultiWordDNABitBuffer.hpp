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

#if ! defined(LIBMAUS2_FASTX_MULTIWORDDNABITBUFFER)
#define LIBMAUS2_FASTX_MULTIWORDDNABITBUFFER

#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/bitio/getBits.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/math/lowbits.hpp>

namespace libmaus2
{
	namespace fastx
	{
		template < unsigned int _bases_per_word = 32 >
		struct MultiWordDNABitBuffer
		{
			static unsigned int const bases_per_word = _bases_per_word;

			typedef uint64_t data_type;

			static unsigned int getMaxBasesPerWord()
			{
				return _bases_per_word;
				// return ( sizeof(data_type) * 8 ) / 2;
			}

			unsigned int const width;
			unsigned int const width2;
			unsigned int const singlewordbuffers;
			unsigned int const fullshift;
			::libmaus2::autoarray::AutoArray < unsigned int > const basewidth;
			::libmaus2::autoarray::AutoArray < unsigned int > const topshift;
			::libmaus2::autoarray::AutoArray < data_type > const masks;
			::libmaus2::autoarray::AutoArray < data_type > buffers;

			void assign(MultiWordDNABitBuffer const & o)
			{
				assert ( width == o.width );
				for ( unsigned int i = 0; i < buffers.getN(); ++i )
					buffers[i] = o.buffers[i];
			}

			bool operator==(MultiWordDNABitBuffer const & o) const
			{
				if ( width != o.width )
					return false;
				for ( unsigned int i = 0; i < buffers.getN(); ++i )
					if ( buffers[i] != o.buffers[i] )
						return false;
				return true;
			}
			bool operator!=(MultiWordDNABitBuffer const & o) const
			{
				return !operator==(o);
			}


			::libmaus2::autoarray::AutoArray < unsigned int > generateBaseWidth()
			{
				::libmaus2::autoarray::AutoArray<unsigned int> basewidth(singlewordbuffers);

				for ( unsigned int i = 0; i < singlewordbuffers; ++i )
					basewidth[i] = getMaxBasesPerWord();
				if ( (width % getMaxBasesPerWord()) != 0 )
					basewidth[singlewordbuffers-1] = width - ((width/getMaxBasesPerWord())*getMaxBasesPerWord());

				return basewidth;
			}

			::libmaus2::autoarray::AutoArray < unsigned int > generateTopShift()
			{
				::libmaus2::autoarray::AutoArray<unsigned int> topshift(singlewordbuffers);

				for ( unsigned int i = 0; i < singlewordbuffers; ++i )
					topshift[i] = 2*(basewidth[i]-1);;

				return topshift;
			}

			::libmaus2::autoarray::AutoArray < data_type > generateMasks()
			{
				::libmaus2::autoarray::AutoArray<data_type> masks(singlewordbuffers);

				for ( unsigned int i = 0; i < singlewordbuffers; ++i )
					masks[i] = ::libmaus2::math::lowbits(2*basewidth[i]);

				return masks;
			}


			static unsigned int getNumberOfWords(unsigned int const width)
			{
				return ( width + getMaxBasesPerWord() - 1 ) / getMaxBasesPerWord();
			}

			static uint64_t getNumberOfBufferBytes(unsigned int const width)
			{
				return getNumberOfWords(width) * sizeof(data_type);
			}

			MultiWordDNABitBuffer(unsigned int const rwidth)
			: width(rwidth), width2(2*width),
			  singlewordbuffers( getNumberOfWords(width) ),
			  fullshift(singlewordbuffers?singlewordbuffers-1:0),
			  basewidth(generateBaseWidth()),
			  topshift(generateTopShift()),
			  masks(generateMasks()),
			  buffers(singlewordbuffers)
			{
				assert ( getMaxBasesPerWord() <= ((sizeof(data_type)*8)/2) );
			}
			void pushBack(data_type const v)
			{
				for ( unsigned int i = 0; i < fullshift; ++i )
				{
					buffers[i] <<= 2;
					buffers[i] &= masks[i]; // not necessary for 32
					buffers[i] |= buffers[i+1] >> topshift[i+1];
				}

				buffers[fullshift] <<= 2;
				buffers[fullshift] &= masks[fullshift];
				buffers[fullshift] |= v;
			}
			void pushFront(data_type const v)
			{
				for ( int i = static_cast<int>(singlewordbuffers) - 1; i >= 1; --i )
				{
					buffers[i] >>= 2;
					buffers[i] |= (buffers[i-1] & 3) << (topshift[i]);
				}
				buffers[0] >>= 2;
				buffers[0] |= (v << topshift[0]);
			}
			void reset()
			{
				for ( unsigned int i = 0; i < singlewordbuffers; ++i )
					buffers[i] = 0;
			}

			static std::string toString(uint64_t const buffer, unsigned int width)
			{
				unsigned int const width2 = width*2;
				unsigned int const width22 = width2-2;
				unsigned int shift = width22;
				data_type pmask = ::libmaus2::math::lowbits(2) << shift;
				std::ostringstream ostr;

				for ( unsigned int i = 0; i < width; ++i, pmask >>= 2, shift -= 2 )
					ostr << ((buffer & pmask) >> shift);

				return ostr.str();
			}
			std::string toString() const
			{
				std::ostringstream ostr;

				for ( unsigned int i = 0; i < fullshift; ++i )
					ostr << toString(buffers[i],getMaxBasesPerWord());

				if ( width )
					ostr << toString(buffers[fullshift], width - fullshift*getMaxBasesPerWord() );

				return ostr.str();
			}
			uint64_t getFrontSymbols(unsigned int l) const
			{
				assert ( l <= getMaxBasesPerWord() );
				assert ( l <= width );

				if ( fullshift )
				{
					return buffers[0] >> (2*(getMaxBasesPerWord() - l));
				}
				else
				{
					return buffers[0] >> (2*(width-l));
				}
			}
		};
	}
}
#endif
