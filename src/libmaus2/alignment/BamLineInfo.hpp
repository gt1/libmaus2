/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_ALIGNMENT_BAMLINEINFO_HPP)
#define LIBMAUS_ALIGNMENT_BAMLINEINFO_HPP

#include <libmaus/bambam/BamWriter.hpp>

namespace libmaus
{
	namespace alignment
	{
		struct BamLineInfo
		{
			int32_t score;
			
			std::string name;
			int32_t refid;
			int32_t pos;
			uint32_t mapq;
			uint32_t flags;
			std::vector<libmaus::bambam::cigar_operation> cigops;
			int32_t nextrefid;
			int32_t nextpos;
			int32_t templatelength;
			std::string seq;
			std::string qual;
			unsigned int qualoff;
			
			uint64_t getFrontSoftClipping() const
			{
				if ( cigops.size() && cigops.front().first == libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CSOFT_CLIP )
					return cigops.front().second;	
				else
					return 0;
			}

			uint64_t getBackSoftClipping() const
			{
				if ( cigops.size() && cigops.back().first == libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CSOFT_CLIP )
					return cigops.back().second;	
				else
					return 0;
			}
			
			double getErrorRate() const
			{
				double n = 0, e = 0;
				
				for ( uint64_t i = 0; i < cigops.size(); ++i )
				{
					switch ( cigops[i].first )
					{
						case libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CDIFF:
						case libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CINS:
						case libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CDEL:
							e += cigops[i].second;
						case libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CEQUAL:
							n += cigops[i].second;
							break;
						default:
							break;
					}
				}
				
				double const erate = e/n;

				// std::cerr << "erate=" << erate << std::endl;
				
				return erate;		
			}
				
			BamLineInfo() : score(-1) {}
			BamLineInfo(
				int32_t rscore,
				std::string const & rname,
				int32_t rrefid,
				int32_t rpos,
				uint32_t rmapq,
				uint32_t rflags,
				std::vector<libmaus::bambam::cigar_operation> const & rcigops,
				int32_t rnextrefid,
				int32_t rnextpos,
				int32_t rtemplatelength,
				std::string const & rseq,
				std::string const & rqual,
				unsigned int rqualoff
			)
			: score(rscore), name(rname), refid(rrefid), pos(rpos), mapq(rmapq),
			  flags(rflags), cigops(rcigops), nextrefid(rnextrefid), nextpos(rnextpos),
			  templatelength(rtemplatelength), seq(rseq), qual(rqual), qualoff(rqualoff)
			{
			}
			
			void encode(libmaus::bambam::BamWriter & bamwr) const
			{
				bamwr.encodeAlignment<
					std::string::const_iterator,
					std::vector<libmaus::bambam::cigar_operation>::const_iterator,
					std::string::const_iterator,
					std::string::const_iterator
				>
				(
					name.begin(),
					name.size(),
					refid,                          // refid
					pos,                           // pos
					mapq,                              // mapq, not given
					flags,                                // flags
					cigops.begin(),                   // cigar ops
					cigops.size(),                    // number of cigar ops
					nextrefid,nextpos,                            // next
					templatelength, // template length
					seq.begin(),                   // sequence
					seq.size(),                    // length of sequence
					qual.begin(),                  // quality
					qualoff                                // quality offset
				);
				bamwr.putAuxNumber("AS",'i',score);
				bamwr.commit();
			}
			
			bool operator<(BamLineInfo const & O) const
			{
				return score > O.score;
			}
		};
	}
}
#endif
