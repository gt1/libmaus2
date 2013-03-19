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


#if ! defined(FASTQENTRY_HPP)
#define FASTQENTRY_HPP

#include <libmaus/fastx/Pattern.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct FASTQEntry : public Pattern
		{
		        typedef FASTQEntry this_type;
                        typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
                        typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			std::string plus;
			std::string quality;
                
			unsigned int getQuality(unsigned int i) const
			{
				return quality[i];
			}
			
			void reverse()
			{
			    Pattern::reverse();
			    std::reverse(quality.begin(),quality.end());
			}
			
			FASTQEntry & operator=(FASTQEntry const & o)
			{
				if ( this != &o )
				{
					// std::cerr << "FASTQ copy." << std::endl;
					Pattern::operator=(static_cast<Pattern const &>(o));
					plus = o.plus;
					quality = o.quality;
				}
				else
				{
					//std::cerr << "FASTQ self copy." << std::endl;
				}
				return *this;
			}
		};

		inline std::ostream & operator<< ( std::ostream & out, FASTQEntry const & p )
		{
			out << "@" << p.getStringId() << std::endl;
			if ( p.pattern )
				out << p.pattern << std::endl;
			else
				out << "null" << std::endl;
			out << "+" << p.plus << std::endl;
			out << p.quality << std::endl;
			return out;
		}

		inline std::ostream & oneline ( std::ostream & out, FASTQEntry const & p )
		{
			out << "@" << p.getStringId() << "\t";
			if ( p.pattern )
				out << p.pattern;
                        out << "\t" << "+" << p.plus << "\t" << p.quality << "\n";
			return out;
		}
	}
}
#endif
