/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_BAMBAM_PILEVECTORELEMENT_HPP)
#define LIBMAUS2_BAMBAM_PILEVECTORELEMENT_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct PileVectorElement
		{
			int32_t refid;
			int32_t readid;
			int32_t refpos;
			int32_t predif;
			int32_t readpos;
			int32_t readbackpos;
			char sym;

			PileVectorElement(
				int32_t rrefid = 0,
				int32_t rreadid = 0,
				int32_t rrefpos = 0,
				int32_t rpredif = 0,
				int32_t rreadpos = 0,
				int32_t rreadbackpos = 0,
				char rsym = 0
			)
			: refid(rrefid), readid(rreadid), refpos(rrefpos), predif(rpredif), readpos(rreadpos), readbackpos(rreadbackpos), sym(rsym)
			{

			}

			PileVectorElement(std::istream & in)
			{
				deserialise(in);
			}

			void serialise(std::ostream & out) const
			{
				libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,readid);
				libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,refpos);
				libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,predif);
				libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,readpos);
				libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,readbackpos);
				libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,sym);
			}

			void deserialise(std::istream & in)
			{
				readid = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
				refpos = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
				predif = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
				readpos = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
				readbackpos = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
				sym = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
			}

			bool operator<(PileVectorElement const & o) const
			{
				if ( refid != o.refid )
					return refid < o.refid;
				else if ( refpos != o.refpos )
					return refpos < o.refpos;
				else if ( predif != o.predif )
					return predif < o.predif;
				else if ( sym != o.sym )
					return sym < o.sym;
				else if ( readid != o.readid )
					return readid < o.readid;
				else
					return readpos < o.readpos;
			}
		};

		std::ostream & operator<<(std::ostream & out, PileVectorElement const & P);
	}
}
#endif
