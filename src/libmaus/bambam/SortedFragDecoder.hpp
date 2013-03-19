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
#if ! defined(LIBMAUS_BAMBAM_SORTEDFRAGDECODER_HPP)
#define LIBMAUS_BAMBAM_SORTEDFRAGDECODER_HPP

#include <libmaus/bambam/ReadEnds.hpp>
#include <libmaus/bambam/ReadEndsHeapPairComparator.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/lz/SnappyCompress.hpp>
#include <queue>

namespace libmaus
{
	namespace bambam
	{
		struct SortedFragDecoder
		{
			typedef SortedFragDecoder this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef std::pair<uint64_t,uint64_t> upair;

			::libmaus::lz::SnappyInputStreamArrayFile::unique_ptr_type infilearray;
			std::vector < uint64_t > const tmpoutcnts;
			std::vector < uint64_t > tmpincnts;
			typedef std::pair<uint64_t,::libmaus::bambam::ReadEnds> qtype;
			std::priority_queue<qtype,std::vector<qtype>,::libmaus::bambam::ReadEndsHeapPairComparator> Q;
			
			static std::vector < uint64_t > pairsToIntervals(std::vector < upair > const & tmpoffsetintervals)
			{
				if ( ! tmpoffsetintervals.size() )
				{
					return std::vector < uint64_t >(0);
				}
				else
				{
					std::vector < uint64_t > V;
					for ( uint64_t i = 0; i < tmpoffsetintervals.size(); ++i )
						V.push_back(tmpoffsetintervals[i].first);
					V.push_back(tmpoffsetintervals.back().second);
					return V;
				}
			}

			static unique_ptr_type construct(
				std::string const & filename,
				std::vector < upair > const & tmpoffsetintervals,
				std::vector < uint64_t > const & rtmpoutcnts		
			)
			{
				return UNIQUE_PTR_MOVE(unique_ptr_type(new this_type(filename,tmpoffsetintervals,rtmpoutcnts)));
			}
			
			SortedFragDecoder(
				std::string const & filename,
				std::vector < upair > const & tmpoffsetintervals,
				std::vector < uint64_t > const & rtmpoutcnts
			)
			: infilearray(::libmaus::lz::SnappyInputStreamArrayFile::construct(filename,pairsToIntervals(tmpoffsetintervals))),
			  tmpoutcnts(rtmpoutcnts),
			  tmpincnts(tmpoutcnts.size(),0)
			{
				for ( uint64_t i = 0; i < tmpoffsetintervals.size(); ++i )
					if ( tmpincnts[i] != tmpoutcnts[i] )
					{
						::libmaus::bambam::ReadEnds RE;
						RE.get((*infilearray)[i]);
						Q.push(qtype(i,RE));
						tmpincnts[i]++;
					}
			}
			
			bool getNext(std::vector< ::libmaus::bambam::ReadEnds> & V)
			{
				::libmaus::bambam::ReadEnds RE;
				bool const ok = getNext(RE);
				
				if ( ok )
					V.push_back(RE);
				
				return ok;
			}
			
			bool getNext(::libmaus::bambam::ReadEnds & RE)
			{
				if ( Q.size() )
				{
					RE = Q.top().second;
					uint64_t const id = Q.top().first;
					Q.pop();
					
					// std::cerr << "id=" << id << std::endl;
					
					if ( tmpincnts[id] != tmpoutcnts[id] )
					{
						::libmaus::bambam::ReadEnds NRE;
						NRE.get((*infilearray)[id]);
						Q.push(qtype(id,NRE));
						tmpincnts[id]++;
					}
					
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
#endif
