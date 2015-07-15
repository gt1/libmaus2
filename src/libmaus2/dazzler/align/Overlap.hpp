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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_OVERLAP_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_OVERLAP_HPP

#include <libmaus2/dazzler/align/Path.hpp>
#include <libmaus2/lcs/Aligner.hpp>
		
namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct Overlap : public libmaus2::dazzler::db::InputBase
			{
				Path path;
				uint32_t flags;
				int32_t aread;
				int32_t bread;
				
				// 40
				
				uint64_t deserialise(std::istream & in)
				{
					uint64_t s = 0;
					
					s += path.deserialise(in);
					
					uint64_t offset = 0;
					flags = getUnsignedLittleEndianInteger4(in,offset);
					aread = getLittleEndianInteger4(in,offset);
					bread = getLittleEndianInteger4(in,offset);

					getLittleEndianInteger4(in,offset); // padding
					
					s += offset;
					
					// std::cerr << "flags=" << flags << " aread=" << aread << " bread=" << bread << std::endl;
					
					return s;
				}
				
				Overlap()
				{
				
				}
				
				Overlap(std::istream & in)
				{
					deserialise(in);
				}

				Overlap(std::istream & in, uint64_t & s)
				{
					s += deserialise(in);
				}
				
				bool isInverse() const
				{
					return (flags & 1) != 0;
				}
				
				uint64_t getNumErrors() const
				{
					return path.getNumErrors();
				}
				
				/**
				 * compute alignment trace
				 *
				 * @param aptr base sequence of aread
				 * @param bptr base sequence of bread if isInverse() returns false, reverse complement of bread if isInverse() returns true
				 * @param tspace trace point spacing
				 * @param ATC trace container for storing trace
				 * @param aligner aligner
				 **/
				void computeTrace(
					uint8_t const * aptr,
					uint8_t const * bptr,
					int64_t const tspace,
					libmaus2::lcs::AlignmentTraceContainer & ATC,
					libmaus2::lcs::Aligner & aligner
				) const
				{
					// current point on A
					int32_t a_i = ( path.abpos / tspace ) * tspace;
					// current point on B
					int32_t b_i = ( path.bbpos );
					
					// reset trace container
					ATC.reset();
					
					for ( size_t i = 0; i < path.path.size(); ++i )
					{
						// block end point on A
						int32_t const a_i_1 = std::min ( static_cast<int32_t>(a_i + tspace), static_cast<int32_t>(path.aepos) );
						// block end point on B
						int32_t const b_i_1 = b_i + path.path[i].second;

						// block on A
						uint8_t const * asubsub_b = aptr + std::max(a_i,path.abpos);
						uint8_t const * asubsub_e = asubsub_b + a_i_1-std::max(a_i,path.abpos);
						
						// block on B
						uint8_t const * bsubsub_b = bptr + b_i;
						uint8_t const * bsubsub_e = bsubsub_b + (b_i_1-b_i);

						aligner.align(asubsub_b,(asubsub_e-asubsub_b),bsubsub_b,bsubsub_e-bsubsub_b);
						
						// add trace to full alignment
						ATC.push(aligner.getTraceContainer());
						
						// update start points
						b_i = b_i_1;
						a_i = a_i_1;
					}
				}
			};

			std::ostream & operator<<(std::ostream & out, Overlap const & P);
		}
	}
}
#endif
