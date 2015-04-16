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

#if ! defined(PRODUCEEDGEREQUEST_HPP)
#define PRODUCEEDGEREQUEST_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/StringSerialisation.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <string>
#include <vector>
#include <fstream>

namespace libmaus2
{
	namespace clustering
	{
		struct ProduceEdgesRequest
		{
			uint64_t const numreads;
			std::vector < std::string > const freqfilenames;
			std::vector < std::string > const idexfilenames;
			
			ProduceEdgesRequest(
				uint64_t const rnumreads,
				std::vector<std::string> const & rfreqfilenames,
				std::vector<std::string> const & ridexfilenames)
			: numreads(rnumreads), 
			  freqfilenames(rfreqfilenames), idexfilenames(ridexfilenames)
			{
			
			}

			ProduceEdgesRequest(std::istream & in)
			: 
			numreads( ::libmaus2::util::NumberSerialisation::deserialiseNumber(in) ),
			freqfilenames ( ::libmaus2::util::StringSerialisation::deserialiseStringVector(in) ),
			idexfilenames ( ::libmaus2::util::StringSerialisation::deserialiseStringVector(in) )
			{
				
			}
				
			void serialise(std::ostream & out) const
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,numreads);
				::libmaus2::util::StringSerialisation::serialiseStringVector(out,freqfilenames);
				::libmaus2::util::StringSerialisation::serialiseStringVector(out,idexfilenames);
			}
			
			void serialise(std::string const & filename) const
			{
			        std::ofstream ostr(filename.c_str(),std::ios::binary);
			        assert(ostr.is_open());
			        serialise(ostr);
			        assert(ostr);
			        ostr.flush();
			        assert(ostr);
			        ostr.close();
			}
		};
	}
}
#endif
