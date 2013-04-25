/**
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
**/
#if ! defined(LIBMAUS_BAMBAM_SNAPPYALIGNMENTMERGEINPUT_HPP)
#define LIBMAUS_BAMBAM_SNAPPYALIGNMENTMERGEINPUT_HPP

#include <libmaus/bambam/BamAlignmentNameComparator.hpp>
#include <libmaus/bambam/BamAlignmentHeapComparator.hpp>
#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/lz/SnappyCompress.hpp>
#include <queue>

namespace libmaus
{
	namespace bambam
	{
		struct SnappyAlignmentMergeInput
		{
			typedef SnappyAlignmentMergeInput this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			std::vector < std::pair < uint64_t, uint64_t > > index;
			libmaus::autoarray::AutoArray < libmaus::lz::SnappyOffsetFileInputStream::unique_ptr_type > streams;
			libmaus::autoarray::AutoArray < libmaus::bambam::BamAlignment > data;
			libmaus::bambam::BamAlignmentNameComparator const namecomp;
			libmaus::bambam::BamAlignmentHeapComparator < libmaus::bambam::BamAlignmentNameComparator > const heapcomp;
			std::priority_queue<uint64_t,std::vector<uint64_t>,libmaus::bambam::BamAlignmentHeapComparator < libmaus::bambam::BamAlignmentNameComparator > > Q;
			
			static unique_ptr_type construct(std::vector < std::pair < uint64_t, uint64_t > > const & rindex, std::string const & fn)
			{
				return UNIQUE_PTR_MOVE(unique_ptr_type(new this_type(rindex,fn)));
			}
			
			SnappyAlignmentMergeInput(
				std::vector < std::pair < uint64_t, uint64_t > > const & rindex,
				std::string const & fn)
			: index(rindex), streams(index.size()), data(index.size()), namecomp(static_cast<uint8_t const *>(0)), heapcomp(namecomp,data.begin()), Q(heapcomp)
			{
				for ( uint64_t i = 0; i < index.size(); ++i )
					if ( index[i].second-- )
					{
						streams [ i ] =
							UNIQUE_PTR_MOVE(
								libmaus::lz::SnappyOffsetFileInputStream::unique_ptr_type(
									new libmaus::lz::SnappyOffsetFileInputStream(fn,index[i].first)
								)
							);
							
						assert ( libmaus::bambam::BamDecoder::readAlignmentGz(*(streams[i]),data[i],0,false) );
						Q.push(i);
					}
			}
			
			bool readAlignment(libmaus::bambam::BamAlignment & algn)
			{
				if ( Q.empty() )
					return false;
				
				uint64_t const t = Q.top();
				Q.pop();
				
				libmaus::bambam::BamAlignment::D_array_type T = algn.D;
				
				algn.D = data[t].D;
				algn.blocksize = data[t].blocksize;
				
				data[t].D = T;
				data[t].blocksize = 0;
				
				if ( index[t].second-- )
				{
					assert ( libmaus::bambam::BamDecoder::readAlignmentGz(*(streams[t]),data[t],0,false) );
					Q.push(t);
				}
					
				return true;
			}
		};
	}
}
#endif
