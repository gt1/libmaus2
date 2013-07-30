/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_BAMRANGE_HPP)
#define LIBMAUS_BAMBAM_BAMRANGE_HPP

#include <libmaus/bambam/BamAlignment.hpp>
#include <libmaus/bambam/BamIndex.hpp>
#include <libmaus/fastx/SpaceTable.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamRange
		{
			enum interval_rel_pos
			{
				interval_rel_pos_pre,
				interval_rel_pos_matching,
				interval_rel_pos_unmatching,
				interval_rel_pos_post
			};

			typedef BamRange this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			virtual ~BamRange() {}
			//! return position of alignment relative to interval
			virtual interval_rel_pos operator()(libmaus::bambam::BamAlignment const &) const = 0;
			//! return range as string
			virtual std::ostream & toStream(std::ostream & ostr) const = 0;
			//! get reference id
			virtual uint64_t getRefID() const = 0;
			//! get position range
			virtual std::pair<uint64_t,uint64_t> getRange() const = 0;
			//! get chunks
			virtual std::vector< std::pair<uint64_t,uint64_t> > getChunks(libmaus::bambam::BamIndex const & index) const
			{
				return index.reg2chunks(getRefID(),getRange().first,getRange().second);
			}
		};

		struct BamRangeChromosome : public BamRange
		{
			std::string name;
			int64_t id;
			uint64_t beg;
			uint64_t end;
			
			BamRangeChromosome() : name(), id(-1)
			{
			
			}
			
			BamRangeChromosome(std::string const & rname, libmaus::bambam::BamHeader const & header)
			: name(rname), id(header.getIdForRefName(name)), beg(0), end(header.chromosomes[id].len)
			{
			
			}
			
			interval_rel_pos operator()(libmaus::bambam::BamAlignment const & algn) const
			{
				int64_t const alrefid = algn.getRefID();

				if ( alrefid == id )
					return interval_rel_pos_matching;
				else if ( alrefid < id )
					return interval_rel_pos_pre;
				else
					return interval_rel_pos_post;
			}

			std::ostream & toStream(std::ostream & ostr) const
			{
				ostr << "BamRangeChromosome(" << name << "," << id << ",[" << beg << "," << end << "))";
				return ostr;
			}

			//! get position range
			std::pair<uint64_t,uint64_t> getRange() const
			{
				return std::pair<uint64_t,uint64_t>(beg,end);
			}

			//! get reference id
			uint64_t getRefID() const
			{
				return id;
			}
		};

		struct BamRangeHalfOpen : public BamRange
		{
			std::string name;
			int64_t id;
			int64_t start;
			int64_t end;
			
			BamRangeHalfOpen() : name(), id(-1), start(-1), end(-1)
			{
			
			}
			
			BamRangeHalfOpen(std::string const & rname, uint64_t const rstart, libmaus::bambam::BamHeader const & header)
			: name(rname), id(header.getIdForRefName(name)), start(rstart), end(header.chromosomes[id].len)
			{
			
			}
			
			interval_rel_pos operator()(libmaus::bambam::BamAlignment const & algn) const
			{
				int64_t const alrefid = algn.getRefID();

				if ( alrefid == id )
				{
					int64_t const pos = algn.getPos();
					int64_t const len = algn.getReferenceLength();
					
					if ( pos + len > start )
						return interval_rel_pos_matching;
					else
						return interval_rel_pos_pre;
				}
				else if ( alrefid < id )
					return interval_rel_pos_pre;
				else
					return interval_rel_pos_post;
			}

			std::ostream & toStream(std::ostream & ostr) const
			{
				ostr << "BamRangeHalfOpen(" << name << "," << id << ",[" << start << "," << end << "))";
				return ostr;
			}

			//! get position range
			std::pair<uint64_t,uint64_t> getRange() const
			{
				return std::pair<uint64_t,uint64_t>(start,end);
			}

			//! get reference id
			uint64_t getRefID() const
			{
				return id;
			}
		};

		struct BamRangeInterval : public BamRange
		{
			std::string name;
			int64_t id;
			int64_t start;
			int64_t end;
			
			BamRangeInterval() : name(), id(-1), start(-1), end(-1)
			{
			
			}
			
			BamRangeInterval(std::string const & rname, uint64_t const rstart, uint64_t const rend, libmaus::bambam::BamHeader const & header)
			: name(rname), id(header.getIdForRefName(name)), start(rstart), end(rend)
			{
			
			}
			
			interval_rel_pos operator()(libmaus::bambam::BamAlignment const & algn) const
			{
				int64_t const alrefid = algn.getRefID();

				if ( alrefid == id )
				{
					int64_t const pos = algn.getPos();
					int64_t const len = algn.getReferenceLength();
					
					if ( pos + len > start )
					{
						if ( pos <= end )
							return interval_rel_pos_matching;
						else
							return interval_rel_pos_post;
					}
					else
						return interval_rel_pos_pre;
				}
				else if ( alrefid < id )
					return interval_rel_pos_pre;
				else
					return interval_rel_pos_post;
			}

			std::ostream & toStream(std::ostream & ostr) const
			{
				ostr << "BamRangeInterval(" << name << "," << id << ",[" << start << "," << end << "))";
				return ostr;
			}

			//! get position range
			std::pair<uint64_t,uint64_t> getRange() const
			{
				return std::pair<uint64_t,uint64_t>(start,end);
			}

			//! get reference id
			uint64_t getRefID() const
			{
				return id;
			}
		};

		std::ostream & operator<<(std::ostream & out, BamRange const & R);
	}
}

#endif
