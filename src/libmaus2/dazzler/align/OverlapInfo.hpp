/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPINFO_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPINFO_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <ostream>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapInfo
			{
				int64_t aread;
				int64_t bread;
				int64_t abpos;
				int64_t aepos;
				int64_t bbpos;
				int64_t bepos;

				OverlapInfo() : aread(0), bread(0), abpos(0), aepos(0), bbpos(0), bepos(0)
				{
				}
				OverlapInfo(
					int64_t const raread,
					int64_t const rbread,
					int64_t const rabpos,
					int64_t const raepos,
					int64_t const rbbpos,
					int64_t const rbepos
				) : aread(raread), bread(rbread), abpos(rabpos), aepos(raepos), bbpos(rbbpos), bepos(rbepos) {}
				OverlapInfo(std::istream & in)
				{
					deserialise(in);
				}

				template<typename iterator>
				OverlapInfo swappedInverse(iterator it) const
				{
					return swappedInverse(it[aread/2],it[bread/2]);
				}

				OverlapInfo swappedInverse(uint64_t const alen, uint64_t const blen) const
				{
					return inverse(alen,blen).swapped();
				}

				OverlapInfo swapped() const
				{
					return OverlapInfo(bread,aread,bbpos,bepos,abpos,aepos);
				}

				OverlapInfo inverse(uint64_t const alen, uint64_t const blen) const
				{
					return OverlapInfo(
						aread ^ 1,
						bread ^ 1,
						alen - aepos,
						alen - abpos,
						blen - bepos,
						blen - bbpos
					);
				}

				OverlapInfo swappedStraight(uint64_t const alen, uint64_t const blen) const
				{
					if ( (bread & 1) )
						return inverse(alen,blen).swapped();
					else
						return swapped();
				}

				template<typename iterator>
				OverlapInfo swappedStraight(iterator A) const
				{
					return swappedStraight(A[aread/2],A[bread/2]);
				}

				template<typename iterator>
				OverlapInfo inverse(iterator A) const
				{
					return inverse(A[aread/2],A[bread/2]);
				}

				std::ostream & serialise(std::ostream & out) const
				{
					libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,aread);
					libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,bread);
					libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,abpos);
					libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,aepos);
					libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,bbpos);
					libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,bepos);
					return out;
				}

				std::istream & deserialise(std::istream & in)
				{
					aread = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
					bread = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
					abpos = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
					aepos = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
					bbpos = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
					bepos = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
					return in;
				}

				bool operator<(OverlapInfo const & O) const
				{
					if ( aread != O.aread )
						return aread < O.aread;
					if ( bread != O.bread )
						return bread < O.bread;
					if ( abpos != O.abpos )
						return abpos < O.abpos;
					if ( aepos != O.aepos )
						return aepos < O.aepos;
					if ( bbpos != O.bbpos )
						return bbpos < O.bbpos;
					if ( bepos != O.bepos )
						return bepos < O.bepos;
					return false;
				}

				bool operator!=(OverlapInfo const & O) const
				{
					return (*this < O) || (O < *this);
				}

				bool operator==(OverlapInfo const & O) const
				{
					return !operator!=(O);
				}
			};

			std::ostream & operator<<(std::ostream & out, OverlapInfo const & O);
		}
	}
}
#endif
