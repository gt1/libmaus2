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
#if ! defined(LIBMAUS2_FASTX_DNAINDEXMETADATABIGBANDBIDIR_HPP)
#define LIBMAUS2_FASTX_DNAINDEXMETADATABIGBANDBIDIR_HPP

#include <libmaus2/fastx/DNAIndexMetaDataSequence.hpp>
#include <libmaus2/math/numbits.hpp>
#include <libmaus2/math/lowbits.hpp>
#include <libmaus2/util/PrefixSums.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct DNAIndexMetaDataBigBandBiDir
		{
			typedef DNAIndexMetaDataBigBandBiDir this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::vector<DNAIndexMetaDataSequence> S;
			std::vector<uint64_t> L;
			uint64_t maxl;

			DNAIndexMetaDataBigBandBiDir(std::istream & in)
			{
				uint64_t const numseq = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				S.resize(numseq);
				for ( uint64_t i = 0; i < numseq; ++i )
					S[i] = DNAIndexMetaDataSequence(in);
				L.resize(numseq+1);
				for ( uint64_t i = 0; i < numseq; ++i )
					L[i] = S[i].l;
				libmaus2::util::PrefixSums::prefixSums(L.begin(),L.end());

				maxl = 0;
				for ( uint64_t i = 0; i < S.size(); ++i )
					maxl = std::max(maxl,S[i].l);
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

			struct Coordinates
			{
				bool valid;
				uint64_t seq;
				bool rc;
				uint64_t left;
				uint64_t length;

				Coordinates() {}
				Coordinates(bool const rvalid, uint64_t const rseq, bool const rrc, uint64_t const rleft, uint64_t const rlength)
				: valid(rvalid), seq(rseq), rc(rrc), left(rleft), length(rlength) {}

				std::ostream & print(std::ostream & out) const
				{
					out << "Coordinates(valid=" << valid << ",seq=" << seq << ",rc=" << rc << ",left=" << left << ",length=" << length << ")";
					return out;
				}
			};

			Coordinates mapCoordinatePair(uint64_t const il, uint64_t const ir)
			{
				if ( ir <= il )
					return Coordinates(false,0,false,0,0);

				std::pair<uint64_t,uint64_t> Pl = mapCoordinates(il);
				std::pair<uint64_t,uint64_t> Pr = mapCoordinates(ir-1);

				if ( Pl.first != Pr.first )
				{
					if ( Pl.first < S.size()/2 )
					{
						uint64_t const seq = Pl.first;
						uint64_t const left = Pl.second;
						return Coordinates(false,seq,false,left,S[seq].l);
					}
					else
					{
						uint64_t const seq = S.size()-Pl.first-1;
						assert( S[Pl.first].l == S[seq].l );
						uint64_t const left = S[seq].l - Pr.second - 1;
						return Coordinates(false,seq,true,left,S[seq].l);
					}
				}

				assert ( S.size() % 2 == 0 );

				if ( Pl.first < S.size()/2 )
				{
					uint64_t const seq = Pl.first;
					uint64_t const left = Pl.second;
					uint64_t const right = Pr.second+1;
					return Coordinates(true,seq,false,left,right-left);
				}
				else
				{
					uint64_t const seq = S.size()-Pl.first-1;
					assert( S[Pl.first].l == S[seq].l );
					uint64_t const left = S[seq].l - Pr.second - 1;
					uint64_t const right = S[seq].l - Pl.second;
					return Coordinates(true,seq,true,left,right-left);
				}
			}

			bool valid(std::pair<uint64_t,uint64_t> const & P, uint64_t const k) const
			{
				uint64_t const seqlen = S[P.first].l;
				return P.second + k <= seqlen;
			}
		};

		std::ostream & operator<<(std::ostream & out, DNAIndexMetaDataBigBandBiDir const & D);
	}
}
#endif
