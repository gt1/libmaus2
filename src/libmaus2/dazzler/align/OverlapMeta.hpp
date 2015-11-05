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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAPMETA_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAPMETA_HPP

#include <libmaus2/dazzler/align/Overlap.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct OverlapMeta
			{
				int64_t aread;
				int64_t bread;
				bool inv;
				int64_t abpos;
				int64_t aepos;
				int64_t bbpos;
				int64_t bepos;

				bool operator<(OverlapMeta const & O) const
				{
					if ( aread != O.aread )
						return aread < O.aread;
					else if ( bread != O.bread )
						return bread < O.bread;
					else if ( inv != O.inv )
						return !inv;
					else if ( abpos != O.abpos )
						return abpos < O.abpos;
					#if 0
					else if ( aepos != O.aepos )
						return aepos < O.aepos;
					else if ( bbpos != O.bbpos )
						return bbpos < O.bbpos;
					else if ( bepos != O.bepos )
						return bepos < O.bepos;
					#endif
					else
						return false;
				}

				bool operator!=(OverlapMeta const & O) const
				{
					return (*this<O)||(O<*this);
				}

				bool operator==(OverlapMeta const & O) const
				{
					return !(*this!=O);
				}

				OverlapMeta() {}
				OverlapMeta(
					int64_t const raread,
					int64_t const rbread,
					bool const rinv,
					int64_t const rabpos,
					int64_t const raepos,
					int64_t const rbbpos,
					int64_t const rbepos
				) : aread(raread), bread(rbread), inv(rinv), abpos(rabpos), aepos(raepos), bbpos(rbbpos), bepos(rbepos) {}
				OverlapMeta(Overlap const & OVL)
				: aread(OVL.aread), bread(OVL.bread), inv(OVL.isInverse()), abpos(OVL.path.abpos), aepos(OVL.path.aepos), bbpos(OVL.path.bbpos), bepos(OVL.path.bepos)
				{

				}
				OverlapMeta(std::istream & in)
				{
					deserialise(in);
				}

				static uint64_t getSerialisedObjectSize()
				{
					return 7 * sizeof(uint64_t);
				}

				uint64_t serialise(std::ostream & out) const
				{
					uint64_t offset = 0;
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,aread,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,bread,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,inv,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,abpos,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,aepos,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,bbpos,offset);
					libmaus2::dazzler::db::OutputBase::putLittleEndianInteger8(out,bepos,offset);
					return offset;
				}

				uint64_t deserialise(std::istream & in)
				{
					uint64_t offset = 0;
					aread = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					bread = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					inv = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					abpos = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					aepos = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					bbpos = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					bepos = libmaus2::dazzler::db::InputBase::getLittleEndianInteger8(in,offset);
					return offset;
				}
			};

			std::ostream & operator<<(std::ostream & out, OverlapMeta const & OM);
		}
	}
}
#endif
