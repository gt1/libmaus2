/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_FASTX_DNAINDEXMETADATA_HPP)
#define LIBMAUS2_FASTX_DNAINDEXMETADATA_HPP

#include <libmaus2/fastx/DNAIndexMetaDataSequence.hpp>
#include <libmaus2/math/numbits.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct DNAIndexMetaData
		{
			typedef DNAIndexMetaData this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::vector<DNAIndexMetaDataSequence> S;
			std::vector<uint64_t> L;
			uint64_t maxl;
			uint64_t seqbits;
			uint64_t posbits;
			uint64_t coordbits;

			uint64_t getSeqBits() const
			{
				return S.size() ? libmaus2::math::numbits(2*S.size()-1) : 0;
			}

			uint64_t getPosBits() const
			{
				return maxl ? libmaus2::math::numbits(maxl-1) : 0;
			}

			uint64_t getCoordBits() const
			{
				return getSeqBits() + getPosBits();
			}

			DNAIndexMetaData(std::istream & in)
			{
				uint64_t const numseq = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				S.resize(numseq);
				for ( uint64_t i = 0; i < numseq; ++i )
					S[i] = DNAIndexMetaDataSequence(in);
				L.resize(2*numseq+1);
				uint64_t a = 0;
				for ( uint64_t i = 0; i < 2*numseq; ++i )
				{
					L[i] = a;
					a += S[i/2].l;
				}
				L[2*numseq] = a;
				maxl = 0;
				for ( uint64_t i = 0; i < S.size(); ++i )
					maxl = std::max(maxl,S[i].l);
				seqbits = getSeqBits();
				posbits = getPosBits();
				coordbits = getCoordBits();
			}

			static unique_ptr_type load(std::istream & in)
			{
				unique_ptr_type tptr(new this_type(in));
				return UNIQUE_PTR_MOVE(tptr);
			}

			static unique_ptr_type load(std::string const & s)
			{
				libmaus2::aio::InputStreamInstance ISI(s);
				unique_ptr_type tptr(new this_type(ISI));
				return UNIQUE_PTR_MOVE(tptr);
			}

			std::pair<uint64_t,uint64_t> mapCoordinates(uint64_t const i) const
			{
				assert ( i < L.back() );
				std::vector<uint64_t>::const_iterator ita = std::lower_bound(L.begin(),L.end(),i);
				assert ( ita != L.end() );

				std::pair<uint64_t,uint64_t> R;

				if ( i == *ita )
					R = std::pair<uint64_t,uint64_t>(ita-L.begin(),0);
				else
				{
					ita -= 1;
					assert ( *ita <= i );
					R = std::pair<uint64_t,uint64_t>(ita-L.begin(),i - *ita);
				}

				assert ( L[R.first] + R.second == i );

				return R;
			}

			uint64_t mapCoordinatesToWord(uint64_t const i) const
			{
				std::pair<uint64_t,uint64_t> const P = mapCoordinates(i);
				assert ( P.first < (1ull<<seqbits) );
				assert ( P.second < (1ull<<posbits) );
				return (P.first << posbits) | P.second;
			}

			void checkMapCoordinates(uint64_t const n) const
			{
				for ( uint64_t i = 0; i < n; ++i )
				{
					mapCoordinates(i);
					mapCoordinatesToWord(i);
				}
				assert ( n == L.back() );
			}
		};

		std::ostream & operator<<(std::ostream & out, DNAIndexMetaData const & D);
	}
}
#endif
