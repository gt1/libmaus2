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
#if ! defined(LIBMAUS_BAMBAM_BAMALIGNMENTREADGROUPFILTER_HPP)
#define LIBMAUS_BAMBAM_BAMALIGNMENTREADGROUPFILTER_HPP

#include <libmaus/bambam/BamAlignmentFilter.hpp>
#include <libmaus/bambam/BamHeader.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamAlignmentReadGroupFilter : public BamAlignmentFilter
		{
			libmaus::bambam::BamHeader const & header;
			libmaus::bitio::BitVector::unique_ptr_type pBV;
			
			BamAlignmentReadGroupFilter(
				libmaus::bambam::BamHeader const & rheader,
				std::vector<std::string> const & rgnames
			) : header(rheader)
			{
				std::set<std::string> const rgset(rgnames.begin(),rgnames.end());
				std::vector<libmaus::bambam::ReadGroup> const & readgroups = header.getReadGroups();
				
				// set up and erase vector
				libmaus::bitio::BitVector::unique_ptr_type tBV(new libmaus::bitio::BitVector(readgroups.size()));
				pBV = UNIQUE_PTR_MOVE(tBV);
				for ( uint64_t i = 0; i < readgroups.size(); ++i )
					pBV->set(i,0);
				
				for ( uint64_t i = 0; i < readgroups.size(); ++i )
					if ( rgset.find(readgroups[i].ID) != rgset.end() )
						pBV->set(i,1);
			}
			virtual ~BamAlignmentReadGroupFilter() {}
			virtual bool operator()(libmaus::bambam::BamAlignment const & algn) const
			{
				int64_t const rg = header.getReadGroupId(algn.getReadGroup());
				return rg >= 0 && pBV->get(rg);
			}
		};
	}
}
#endif
