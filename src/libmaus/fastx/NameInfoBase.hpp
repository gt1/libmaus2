/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_FASTX_NAMEINFOBASE_HPP)
#define LIBMAUS_FASTX_NAMEINFOBASE_HPP

#include <string>
#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct NameInfoBase
		{
			enum fastq_name_scheme_type { 
				fastq_name_scheme_generic,
				fastq_name_scheme_casava18_single,
				fastq_name_scheme_casava18_paired_end,
				fastq_name_scheme_pairedfiles
			};

			static fastq_name_scheme_type parseNameScheme(std::string const & schemename)
			{
				if ( schemename == "generic" )
					return fastq_name_scheme_generic;
				else if ( schemename == "c18s" )
					return fastq_name_scheme_casava18_single;
				else if ( schemename == "c18pe" )
					return fastq_name_scheme_casava18_paired_end;
				else if ( schemename == "pairedfiles" )
					return fastq_name_scheme_pairedfiles;
				else
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "unknown read name scheme " << schemename << std::endl;
					lme.finish();
					throw lme;	
				}
			}
		};

		std::ostream & operator<<(std::ostream & out, NameInfoBase::fastq_name_scheme_type const namescheme);
	}
}
#endif
