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
#if ! defined(LIBMAUS_FASTX_FASTQELEMENT_HPP)
#define LIBMAUS_FASTX_FASTQELEMENT_HPP

#include <string>
#include <cassert>
#include <libmaus/fastx/FASTQEntry.hpp>
#include <libmaus/fastx/Phred.hpp>
#include <libmaus/util/unique_ptr.hpp>  
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct FastQElement
		{
			typedef FastQElement this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			std::string name;
			std::string query;
			std::string plus;
			std::string quality;
			
			FastQElement() : name(), query(), plus(), quality()
			{
			
			}
			
			FastQElement(
				std::string const & rname, 
				std::string const & rquery, 
				std::string const & rplus,
				std::string const & rquality
			)
			: name(rname), query(rquery), plus(rplus), quality(rquality)
			{
				
			}
			
			FastQElement(FASTQEntry const & entry)
			: name(entry.sid), query(entry.spattern), plus(entry.plus), quality(entry.quality)
			{
			
			}
			
			unsigned int getBaseQuality(uint64_t const i, unsigned int const offset = 33) const
			{
				if ( ! (i < quality.size() ) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Index " << i << " >= " << quality.size() << " in FastQElement::getBaseQuality()" << std::endl;
					se.finish();
					throw se;
				}
				if ( ! (quality[i] >= static_cast<std::string::value_type>(offset) ) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Quality " << static_cast<int>(quality[i]) << " at index " << i 
						<< " is smaller than offset " << offset << " in FastQElement::getBaseQuality()" << std::endl;
					se.finish();
					throw se;
				}

				return quality[i] - offset;
			}
			
			double getBaseErrorProbability(uint64_t const i, unsigned int const offset = 33) const
			{
				unsigned int const q = getBaseQuality(i,offset);
				assert ( q < sizeof(Phred::phred_error)/sizeof(Phred::phred_error[0]) );
				return Phred::phred_error[q];
			}

			double getBaseCorrectnessProbability(uint64_t const i, unsigned int const offset = 33) const
			{
				unsigned int const q = getBaseQuality(i,offset);
				assert ( q < sizeof(Phred::phred_error)/sizeof(Phred::phred_error[0]) );
				return Phred::probCorrect(q);
			}
		};
		
		inline std::ostream & operator<<(std::ostream & out, FastQElement const & e)
		{
			out << "@" << e.name << '\n';
			out << e.query << '\n';
			out << "+" << e.plus << '\n';
			out << e.quality << '\n';
			return out;
		}
	}
}
#endif
