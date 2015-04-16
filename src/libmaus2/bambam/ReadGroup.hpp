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
#if ! defined(LIBMAUS2_BAMBAM_READGROUP_HPP)
#define LIBMAUS2_BAMBAM_READGROUP_HPP

#include <libmaus2/util/unordered_map.hpp>
#include <libmaus2/hashing/hash.hpp>
#include <libmaus2/util/StringMapCompare.hpp>
#include <libmaus2/util/unique_ptr.hpp>

#include <string>
#include <ostream>
#include <set>
#include <iostream>
#include <sstream>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * read group class
		 **/
		struct ReadGroup
		{
			//! read group id
			std::string ID;
			//! key,value map
			::libmaus2::util::unordered_map<std::string,std::string>::type M;
			//! library id
			int64_t LBid;
			
			/**
			 * construct invalid read group
			 **/
			ReadGroup()
			: LBid(-1)
			{
			
			}
			
			/**
			 * assignment operator
			 *
			 * @param o other read group
			 * @return *this after assignment
			 **/
			ReadGroup & operator=(ReadGroup const & o)
			{
				if ( this != &o )
				{
					this->ID = o.ID;
					this->M = o.M;
					this->LBid = o.LBid;
				}
				return *this;
			}
			
			/**
			 * compute 32 bit hash value from iterator range
			 *
			 * @param ita start iterator (inclusive)
			 * @param ite end iterator (exclusive)
			 * @return hash value
			 **/
			template<typename iterator>
			static uint32_t hash(iterator ita, iterator ite)
			{
				return libmaus2::hashing::EvaHash::hash(reinterpret_cast<uint8_t const *>(ita),ite-ita);
			}

			/**
			 * compute hash value from read group id
			 *
			 * @return hash value
			 **/
			uint32_t hash() const
			{
				uint8_t const * ita = reinterpret_cast<uint8_t const *>(ID.c_str());
				return hash(ita,ita+ID.size());
			}
			
			std::string createLine() const
			{
				std::ostringstream linestr;
				
				linestr << "@RG\tID:" << ID;
				
				for ( ::libmaus2::util::unordered_map<std::string,std::string>::type::const_iterator ita = M.begin();
					ita != M.end(); ++ita )
					linestr << "\t" << ita->first << ":" << ita->second;
					
				return linestr.str();
			}
		};
		
		struct ReadGroupVectorMerge
		{
			typedef ReadGroupVectorMerge this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			struct ReadGroupIndexComparator
			{
				std::vector< std::vector<ReadGroup> const * > const & V;
				
				ReadGroupIndexComparator(std::vector< std::vector<ReadGroup> const * > const & rV)
				: V(rV)
				{
				
				}
				
				bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
				{
					ReadGroup const & RA = V[A.first]->at(A.second);
					ReadGroup const & RB = V[B.first]->at(B.second);
					
					if ( RA.ID != RB.ID )
						return RA.ID < RB.ID;
					else if ( 
						libmaus2::util::StringMapCompare::compare(RA.M,RB.M)
						||
						libmaus2::util::StringMapCompare::compare(RB.M,RA.M)
					)
						return libmaus2::util::StringMapCompare::compare(RA.M,RB.M);
					else
						return RA.LBid < RB.LBid;
				}
				
				bool equal(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
				{
					if ( (*this)(A,B) )
						return false;
					else if ( (*this)(B,A) )
						return false;
					else
						return true;
				}
			};
			
			std::vector < ReadGroup > readgroups;
			std::vector < std::vector<uint64_t> > readgroupsmapping;
		
			ReadGroupVectorMerge(std::vector< std::vector<ReadGroup> const * > const & V)
			{
				std::vector < std::pair<uint64_t,uint64_t> > M;
				
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					for ( uint64_t j = 0; j < V[i]->size(); ++j )
						M.push_back(std::pair<uint64_t,uint64_t>(i,j));
					readgroupsmapping.push_back(std::vector<uint64_t>(V[i]->size()));
				}
						
				ReadGroupIndexComparator comp(V);
				std::sort(M.begin(),M.end(),comp);
				std::set<std::string> idsused;
				
				uint64_t low = 0;
				while ( low != M.size() )
				{
					uint64_t high = low;
					
					while ( 
						high != M.size()
						&&
						comp.equal(M[low],M[high])
					)
						++high;
						
					ReadGroup RG = V[M[low].first]->at(M[low].second);
					while ( idsused.find(RG.ID) != idsused.end() )
						RG.ID += '\'';
					idsused.insert(RG.ID);
					
					for ( uint64_t i = low; i < high; ++i )
						readgroupsmapping[M[i].first][M[i].second] = readgroups.size();
						
					readgroups.push_back(RG);
						
					low = high;
				}
				
				#if 0
				for ( uint64_t i = 0; i < readgroupsmapping.size(); ++i )
					for ( uint64_t j = 0; j < readgroupsmapping[i].size(); ++j )
						std::cerr 
							<< V[i]->at(j).ID
							<< " -> " 
							<< readgroups[readgroupsmapping[i][j]].ID
							<< std::endl;
				#endif
			}
		};
	}
}

/**
 * format read group for output stream
 *
 * @param out output stream
 * @param RG read group
 * @return out
 **/
std::ostream & operator<<(std::ostream & out, libmaus2::bambam::ReadGroup const & RG);
#endif
