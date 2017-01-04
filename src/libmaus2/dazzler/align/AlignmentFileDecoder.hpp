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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTFILEDECODER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTFILEDECODER_HPP

#include <libmaus2/dazzler/align/DalignerIndexDecoder.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct AlignmentFileDecoder
			{
				typedef AlignmentFileDecoder this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
				std::istream & in;

				int64_t const tspace;
				bool const small;
				uint64_t const pb;
				uint64_t const pe;
				uint64_t p;

				bool slotactive;
				libmaus2::dazzler::align::Overlap slot;

				AlignmentFileDecoder(std::string const & lasfn, int64_t const rtspace, uint64_t const rpb, uint64_t const rpe)
				: PISI(new libmaus2::aio::InputStreamInstance(lasfn)), in(*PISI),
				  tspace(rtspace), small(libmaus2::dazzler::align::AlignmentFile::tspaceToSmall(tspace)),
				  pb(rpb), pe(rpe), p(pb), slotactive(false)
				{
					in.clear();
					in.seekg(pb,std::ios::beg);
				}

				bool getNextOverlapRaw(libmaus2::dazzler::align::Overlap & OVL)
				{
					if ( p == pe )
						return false;
					else
					{
						libmaus2::dazzler::align::AlignmentFile::readOverlap(in,OVL,p,small);
						return true;
					}
				}

				void tryFill()
				{
					if ( ! slotactive )
						slotactive = getNextOverlapRaw(slot);
				}

				bool peekNextOverlap(libmaus2::dazzler::align::Overlap & OVL)
				{
					tryFill();

					if ( slotactive )
					{
						OVL = slot;
						return true;
					}
					else
					{
						return false;
					}
				}

				bool getNextOverlap(libmaus2::dazzler::align::Overlap & OVL)
				{
					tryFill();

					if ( slotactive )
					{
						OVL = slot;
						slotactive = false;
						return true;
					}
					else
					{
						return false;
					}
				}
			};
		}
	}
}
#endif
