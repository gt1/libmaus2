/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_CRAMRANGE_HPP)
#define LIBMAUS_BAMBAM_CRAMRANGE_HPP

#include <string>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct CramRange
		{
			std::string rangeref;
			int64_t rangestart;
			int64_t rangeend;
			
			static CramRange intersect(CramRange const & A, CramRange const & B)
			{
				// empty range if not the same sequence
				if ( A.rangeref != B.rangeref )
					return CramRange(std::string(),0,-1);
				
				// make sure A starts before B
				if ( A.rangestart > B.rangestart )
					return intersect(B,A);
				assert ( A.rangestart <= B.rangestart );
				
				// no intersection if A ends before B starts
				if ( A.rangeend < B.rangestart )
					return CramRange(std::string(),0,-1);
				
				return CramRange(A.rangeref,B.rangestart,std::min(A.rangeend,B.rangeend));
			}
			
			CramRange intersect(CramRange const & O) const
			{
				return intersect(*this,O);
			}
			
			bool empty() const
			{
				return rangestart > rangeend;
			}
			
			CramRange()
			: rangeref(), rangestart(-1), rangeend(-1)
			{
			
			}
			
			CramRange(std::string const & rrangeref, int64_t const rrangestart, int64_t const rrangeend)
			: rangeref(rrangeref), rangestart(rrangestart), rangeend(rrangeend)
			{
			
			}
			
			CramRange(std::string const & range)
			: rangeref(), rangestart(-1), rangeend(-1)
			{
				std::size_t const colpos = range.find(":");
				// just ref seq name
				if ( colpos == std::string::npos )
				{
					rangeref = range;
					rangestart = 0;
					rangeend = -1;
				}
				else
				{
					rangeref = range.substr(0,colpos);

					std::string rangerest = range.substr(colpos+1);
					std::size_t const minpos = rangerest.find("-");

					// no interval, just start index
					if ( minpos == std::string::npos )
					{
						rangestart = 0;
						for ( uint64_t i = 0; i < rangerest.size(); ++i )
							if ( isdigit(static_cast<uint8_t>(rangerest[i])) )
							{
								rangestart *= 10;
								rangestart += (rangerest[i]-'0');
							}
							else
							{
								libmaus2::exception::LibMausException ex;
								ex.getStream() << "CramRange(): cannot parse CRAM input range " << range << " (part after colon contains no minus and is not a number)" << std::endl;
								ex.finish();
								throw ex;
							}
						rangeend = -1;
					}
					else
					{
						std::string const sstart = rangerest.substr(0,minpos);
						std::string const send = rangerest.substr(minpos+1);
						
						rangestart = 0;
						for ( uint64_t i = 0; i < sstart.size(); ++i )
							if ( ! isdigit(static_cast<uint8_t>(sstart[i])) )
							{
								libmaus2::exception::LibMausException ex;
								ex.getStream() << "CramRange(): cannot parse CRAM input range " << range << " (start point is not a number)" << std::endl;
								ex.finish();
								throw ex;		
							}
							else
							{
								rangestart *= 10;
								rangestart += (sstart[i]-'0');
							}
						rangeend = 0;
						for ( uint64_t i = 0; i < send.size(); ++i )
							if ( ! isdigit(static_cast<uint8_t>(send[i])) )
							{
								libmaus2::exception::LibMausException ex;
								ex.getStream() << "CramRange(): cannot parse CRAM input range " << range << " (end point is not a number)" << std::endl;
								ex.finish();
								throw ex;		
							}
							else
							{
								rangeend *= 10;
								rangeend += (send[i]-'0');
							}
					}
				}
			}
		};
	}
}
#endif
