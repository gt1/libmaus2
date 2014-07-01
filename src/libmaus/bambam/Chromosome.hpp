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
#if ! defined(LIBMAUS_BAMBAM_CHROMOSOME_HPP)
#define LIBMAUS_BAMBAM_CHROMOSOME_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <iostream>
#include <sstream>
#include <cassert>
#include <libmaus/types/types.hpp>
#include <libmaus/util/unordered_map.hpp>
#include <libmaus/bambam/StrCmpNum.hpp>
#include <libmaus/util/StringMapCompare.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * reference sequence info
		 **/
		struct Chromosome
		{
			//! ref seq name
			std::string name;
			//! ref seq length
			uint64_t len;
			//! additional key:value fields
			::libmaus::util::unordered_map<std::string,std::string>::type M;
				
			/**
			 * constructor for empty reference sequence
			 **/
			Chromosome() : name(), len(0), M() {}

			/**
			 * constructor from other object
			 **/
			Chromosome(Chromosome const & o)
			: name(o.name), len(o.len), M(o.M) {}
			
			/**
			 * constructor from parameters
			 *
			 * @param rname ref seq name
			 * @param rlen ref seq length
			 **/
			Chromosome(std::string const & rname, uint64_t const rlen) : name(rname), len(rlen), M() {}

			/**
			 * assignment operator
			 *
			 * @param o other object
			 * @return *this
			 **/
			Chromosome & operator=(Chromosome const & o)
			{
				if ( this != &o )
				{
					this->name = o.name;
					this->len = o.len;
					this->M = o.M;
				}
				return *this;
			}			
			
			std::string createLine() const
			{
				std::ostringstream linestr;
				linestr << "@SQ" 
					<< "\t" << "SN:" << name
					<< "\t" << "LN:" << len;

				for ( ::libmaus::util::unordered_map<std::string,std::string>::type::const_iterator ita = M.begin();
					ita != M.end(); ++ita )
					linestr << "\t" << ita->first << ":" << ita->second;
				
				return linestr.str();
			}
		};		
	}
}
#endif
