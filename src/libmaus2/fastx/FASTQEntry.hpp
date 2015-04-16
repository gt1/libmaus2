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


#if ! defined(FASTQENTRY_HPP)
#define FASTQENTRY_HPP

#include <libmaus2/fastx/Pattern.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct FASTQEntry : public Pattern
		{
		        typedef FASTQEntry this_type;
                        typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                        typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
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
					// std::cerr << "FASTQ copy." << '\n';
					Pattern::operator=(static_cast<Pattern const &>(o));
					plus = o.plus;
					quality = o.quality;
				}
				else
				{
					//std::cerr << "FASTQ self copy." << '\n';
				}
				return *this;
			}
		};

		inline std::ostream & operator<< ( std::ostream & out, FASTQEntry const & p )
		{
			out << '@' << p.getStringId() << '\n';
			if ( p.pattern )
				out << p.pattern << '\n';
			else
				out << "null" << '\n';
			out << '+' << p.plus << '\n';
			out << p.quality << '\n';
			return out;
		}

		inline std::ostream & oneline ( std::ostream & out, FASTQEntry const & p )
		{
			out << '@' << p.getStringId() << '\t';
			if ( p.pattern )
				out << p.pattern;
                        out << '\t' << '+' << p.plus << '\t' << p.quality << '\n';
			return out;
		}
	}
}
#endif
