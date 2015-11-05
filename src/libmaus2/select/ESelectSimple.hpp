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

#if ! defined(ESELECTSIMPLE_HPP)
#define ESELECTSIMPLE_HPP

#include <libmaus2/select/ESelectBase.hpp>
#include <libmaus2/bitio/BitWriter.hpp>
#include <libmaus2/bitio/getBit.hpp>

namespace libmaus2
{
	namespace select
	{
		/**
		 * one level select dictionary
		 **/
		template<bool sym, unsigned int const stepbitslog = 6>
		struct ESelectSimple : public ESelectBase<sym>
		{
			typedef ESelectSimple<sym,stepbitslog> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef ::libmaus2::bitio::BitWriter8 writer_type;

			private:
			uint64_t const * UUUUUUUU;
			uint64_t const n;
			uint64_t const n64;
			uint64_t const n1;

			static unsigned int const stepbits  = (1u << stepbitslog);
			static unsigned int const stepbits1 = (stepbits-1);

			::libmaus2::autoarray::AutoArray<uint64_t> L64;

			uint64_t computeNum1(uint64_t const * UUUUUUUU) const
			{
				uint64_t n1 = 0;
				for ( uint64_t const * U = UUUUUUUU; U != UUUUUUUU+n64; ++U )
					n1 += ::libmaus2::rank::ERankBase::popcount8(ESelectBase<sym>::process(*U));
				return n1;
			}

			public:
			ESelectSimple(uint64_t const * rUUUUUUUU, uint64_t const rn)
			: ESelectBase<sym>(), UUUUUUUU(rUUUUUUUU), n(rn), n64(n/64),
			  n1(computeNum1(UUUUUUUU)),
			  L64( (n1+stepbits1)/stepbits, false )
			{
				if ( n % 64 )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "ESelectSimple(): n is not a multiple of 64" << std::endl;
					se.finish();
					throw se;
				}

				// set up dictionary for mod stepbits = 0 ranks
				uint64_t c = 0;
				for ( uint64_t const * U = UUUUUUUU; U != UUUUUUUU+n64; ++U )
				{
					uint64_t const v = *U;
					for ( uint64_t i = 0, m = (1ull<<63); m; ++i, m >>= 1 )
						if ( v & m )
						{
							if ( !(c & stepbits1) )
							{
								assert ( ((c >> stepbitslog)<<stepbitslog) == c );
								L64 [ c >> stepbitslog ] = ((U-UUUUUUUU)<<6) + i;
							}
							++c;
						}

				}

				for ( uint64_t i = 0; i < L64.size(); ++i )
					assert ( ::libmaus2::bitio::getBit(UUUUUUUU,L64[i]) );
			}

			uint64_t select1(uint64_t i) const
			{
				uint64_t const ii = (i >> stepbitslog);
				i -= (ii << stepbitslog);
				uint64_t j = L64 [ ii ];

				if ( ! i )
					return j;

				if ( (j + 1) & 0x3F )
				{
					uint64_t const v = ESelectBase<sym>::process(UUUUUUUU[ (j+1) >> 6 ]);
					unsigned int const restbits = (64-((j+1)&0x3F));
					uint64_t const restmask = ((1ull << restbits)-1);
					uint64_t const p = ::libmaus2::rank::ERankBase::popcount8( v & restmask );

					// selected bit is not in this word
					if ( p < i )
						j += restbits, i -= p;
					// selected bit is in this word
					else
						return 1 + (j + ESelectBase<sym>::select1(((v&restmask) << (64-restbits)),i-1));
				}

				uint64_t w = (j+1) >> 6;
				unsigned int p;

				while ( (p=::libmaus2::rank::ERankBase::popcount8(ESelectBase<sym>::process(UUUUUUUU[w]))) < i )
				{
					j += 64;
					i -= p;
					w++;
				}

				return 1 + (j + ESelectBase<sym>::select1(ESelectBase<sym>::process(UUUUUUUU[w]),i-1));
			}

			uint64_t byteSize() const
			{
				return 3*sizeof(uint64_t) + sizeof(uint64_t const *) + L64.byteSize();
			}

		};
	}
}
#endif
