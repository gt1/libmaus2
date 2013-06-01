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
#if ! defined(LIBMAUS_BAMBAM_READGROUP_HPP)
#define LIBMAUS_BAMBAM_READGROUP_HPP

#include <libmaus/util/unordered_map.hpp>
#include <libmaus/hashing/hash.hpp>

#include <string>
#include <ostream>

namespace libmaus
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
			::libmaus::util::unordered_map<std::string,std::string>::type M;
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
				return libmaus::hashing::EvaHash::hash(reinterpret_cast<uint8_t const *>(ita),ite-ita);
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
std::ostream & operator<<(std::ostream & out, libmaus::bambam::ReadGroup const & RG);
#endif
