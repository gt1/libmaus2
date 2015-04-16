/*
    libmaus2
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
#if ! defined(LIBMAUS_BAMBAM_SORTEDFRAGDECODER_HPP)
#define LIBMAUS_BAMBAM_SORTEDFRAGDECODER_HPP

#include <libmaus2/bambam/ReadEnds.hpp>
#include <libmaus2/bambam/ReadEndsHeapPairComparator.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/lz/SnappyInputStreamArrayFile.hpp>
#include <queue>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * sorted fragment list decoder
		 **/
		struct SortedFragDecoder
		{
			//! this type
			typedef SortedFragDecoder this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			//! uint64_t pair
			typedef std::pair<uint64_t,uint64_t> upair;
			//! snappy input array
			::libmaus2::lz::SnappyInputStreamArrayFile::unique_ptr_type infilearray;
			//! number of alignments per block
			std::vector < uint64_t > const tmpoutcnts;
			//! number of alignments read per block
			std::vector < uint64_t > tmpincnts;
			//! pair of list index and ReadEnds object
			typedef std::pair<uint64_t,::libmaus2::bambam::ReadEnds> qtype;
			//! merge heap
			std::priority_queue<qtype,std::vector<qtype>,::libmaus2::bambam::ReadEndsHeapPairComparator> Q;
			
			/**
			 * convert vector of pairs [u_0,u_1),[u_1,u_2),... to index vector [u_0,u_1,u_2,...)
			 *
			 * @param tmpoffsetintervals pair vector
			 * @return index vector
			 **/
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

			public:
			/**
			 * construct decoder
			 *
			 * @param filename input file name
			 * @param tmpoffsetintervals block index
			 * @param rtmpoutcnts number of ReadEnds object per block
			 * @return decoder object
			 **/
			static unique_ptr_type construct(
				std::string const & filename,
				std::vector < upair > const & tmpoffsetintervals,
				std::vector < uint64_t > const & rtmpoutcnts		
			)
			{
				unique_ptr_type ptr(new this_type(filename,tmpoffsetintervals,rtmpoutcnts));
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			/**
			 * constructor
			 *
			 * @param filename input file name
			 * @param tmpoffsetintervals block index
			 * @param rtmpoutcnts number of ReadEnds object per block
			 **/
			SortedFragDecoder(
				std::string const & filename,
				std::vector < upair > const & tmpoffsetintervals,
				std::vector < uint64_t > const & rtmpoutcnts
			)
			: infilearray(::libmaus2::lz::SnappyInputStreamArrayFile::construct(filename,pairsToIntervals(tmpoffsetintervals))),
			  tmpoutcnts(rtmpoutcnts),
			  tmpincnts(tmpoutcnts.size(),0)
			{
				infilearray->setEofThrow(true);
				
				for ( uint64_t i = 0; i < tmpoffsetintervals.size(); ++i )
					if ( tmpincnts[i] != tmpoutcnts[i] )
					{
						::libmaus2::bambam::ReadEnds RE;
						RE.get((*infilearray)[i]);
						Q.push(qtype(i,RE));
						tmpincnts[i]++;
					}
			}
			
			/**
			 * get next ReadEnds object and append it to V if one is available
			 *
			 * @param V vector of ReadEnds objects
			 * @return true iff an object was appended to V, false if no more objects were available
			 **/
			bool getNext(std::vector< ::libmaus2::bambam::ReadEnds> & V)
			{
				::libmaus2::bambam::ReadEnds RE;
				bool const ok = getNext(RE);
				
				if ( ok )
					V.push_back(RE);
				
				return ok;
			}
			
			/**
			 * get next ReadEnds object
			 *
			 * @param RE reference to ReadEnds object to be filled
			 * @return true iff an object could be decoded, false if no more objects were available
			 **/
			bool getNext(::libmaus2::bambam::ReadEnds & RE)
			{
				if ( Q.size() )
				{
					RE = Q.top().second;
					uint64_t const id = Q.top().first;
					Q.pop();
					
					// std::cerr << "id=" << id << std::endl;
					
					if ( tmpincnts[id] != tmpoutcnts[id] )
					{
						::libmaus2::bambam::ReadEnds NRE;
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
