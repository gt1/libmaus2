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
#if ! defined(LIBMAUS2_BAMBAM_CHROMOSOME_HPP)
#define LIBMAUS2_BAMBAM_CHROMOSOME_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <iostream>
#include <sstream>
#include <cassert>
#include <libmaus2/types/types.hpp>
#include <libmaus2/util/unordered_map.hpp>
#include <libmaus2/bambam/StrCmpNum.hpp>
#include <libmaus2/util/StringMapCompare.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/exception/LibMausException.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * reference sequence info
		 **/
		struct Chromosome
		{
			private:
			//! ref seq name
			std::string name;
			//! ref seq length
			uint64_t len;

			private:
			//! additional key:value fields
			std::string restkv;

			public:
			// get sorted vector of key=value pairs
			std::vector< std::pair<std::string,std::string> > getSortedKeyValuePairs() const
			{
				std::vector< std::pair<std::string,std::string> > V;

				// extract key:value pairs
				uint64_t low = 0;
				while ( low != restkv.size() )
				{
					// search for next tab
					uint64_t high = low;
					while ( high != restkv.size() && restkv[high] != '\t' )
						++high;

					// search for end of key
					uint64_t klow = low;
					uint64_t khigh = klow;
					while ( khigh != high && restkv[khigh] != ':' )
						++khigh;

					bool const ok = (khigh-klow==2) && restkv[khigh] == ':';
					if ( ! ok )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "libmaus2::bambam::Chromosome::getSortedKeyValuePairs(): malformed key/value part "
							<< restkv
							<< " low=" << low << " high=" << high << " interval=" << std::string(restkv.begin()+low,restkv.begin()+high)
							<< " klow=" << klow << " khigh=" << khigh << " kinterval=" << std::string(restkv.begin()+klow,restkv.begin()+khigh)
							<< std::endl;
						lme.finish();
						throw lme;
					}

					uint64_t vlow = khigh + 1;
					uint64_t vhigh = high;

					V.push_back(
						std::pair<std::string,std::string>(
							restkv.substr(klow,khigh-klow),
							restkv.substr(vlow,vhigh-vlow)
						)
					);

					assert ( high == restkv.size() || restkv[high] == '\t' );

					// consume '\t'
					if ( high != restkv.size() )
						high += 1;

					low = high;
				}

				// sort key:value pairs
				std::sort(V.begin(),V.end());

				return V;
			}

			std::string getRestKVString() const
			{
				return restkv;
			}

			// rest string containing rest of tab separated key:value pairs
			void setRestKVString(std::string const & rrestkv)
			{
				restkv = rrestkv;
			}

			/**
			 * constructor for empty reference sequence
			 **/
			Chromosome() : name(), len(0), restkv() {}

			/**
			 * constructor from other object
			 **/
			Chromosome(Chromosome const & o)
			: name(o.name), len(o.len), restkv(o.restkv) {}

			/**
			 * constructor from parameters
			 *
			 * @param rname ref seq name
			 * @param rlen ref seq length
			 **/
			Chromosome(std::string const & rname, uint64_t const rlen) : name(rname), len(rlen), restkv() {}

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
					this->restkv = o.restkv;
				}
				return *this;
			}

			std::string createLine() const
			{
				std::ostringstream linestr;
				linestr << "@SQ"
					<< "\t" << "SN:" << name
					<< "\t" << "LN:" << len;

				// append rest of key:value pairs if any
				if ( restkv.size() )
					linestr << '\t' << restkv;

				return linestr.str();
			}

			std::pair<char const *, char const *> getName() const
			{
				return std::pair<char const *, char const *>(name.c_str(),name.c_str() + name.size());
			}

			std::string getNameString() const
			{
				return name;
			}

			char const * getNameCString() const
			{
				return name.c_str();
			}

			uint64_t getLength() const
			{
				return len;
			}

			void setName(std::string const & rname)
			{
				name = rname;
			}
		};
	}
}
#endif
