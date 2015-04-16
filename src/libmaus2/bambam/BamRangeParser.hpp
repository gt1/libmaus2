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
#if ! defined(LIBMAUS2_BAMBAM_BAMRANGEPARSER_HPP)
#define LIBMAUS2_BAMBAM_BAMRANGEPARSER_HPP

#include <libmaus2/bambam/BamRange.hpp>
#include <libmaus2/fastx/SpaceTable.hpp>
#include <vector>
#include <string>

namespace libmaus2
{
	namespace bambam
	{
		struct BamRangeParser
		{
			static std::vector < std::string > splitSpace(std::string const & s, libmaus2::fastx::SpaceTable const & ST)
			{
				std::vector < std::string > tokens;
				uint64_t i = 0;
				
				while ( i != s.size() )
				{
					while ( i != s.size() && ST.spacetable[s[i]] )
						++i;
						
					uint64_t const p = i;
					
					while ( i != s.size() && ST.nospacetable[s[i]] )
						++i;
						
					if ( i != p )
						tokens.push_back(s.substr(p,i-p));		
				}
				
				return tokens;
			}

			static std::vector < std::string > splitSpace(std::string const & s)
			{
				libmaus2::fastx::SpaceTable const ST;
				return splitSpace(s,ST);
			}
			
			static libmaus2::autoarray::AutoArray<libmaus2::bambam::BamRange::unique_ptr_type> parse(std::string const & ranges, libmaus2::bambam::BamHeader const & header)
			{
				std::vector < std::string > const outertokens = splitSpace(ranges);
				libmaus2::autoarray::AutoArray<libmaus2::bambam::BamRange::unique_ptr_type> A(outertokens.size());

				for ( uint64_t i = 0; i < outertokens.size(); ++i )
				{
					std::string const & outertoken = outertokens[i];
					
					uint64_t sempos = outertoken.size();
					
					for ( uint64_t j = 0; j < outertoken.size(); ++j )
						if ( outertoken[j] == ':' )
							sempos = j;
							
					if ( sempos == outertoken.size() )
					{
						libmaus2::bambam::BamRange::unique_ptr_type tAi(new libmaus2::bambam::BamRangeChromosome(outertoken,header));
						A[i] = UNIQUE_PTR_MOVE(tAi);
					}
					else
					{
						std::string const refname = outertoken.substr(0,sempos);
						std::string const rest = outertoken.substr(sempos+1);
						
						// std::cerr << "refname=" << refname << " rest=" << rest << std::endl;
						
						uint64_t dashpos = rest.size();
						
						for ( uint64_t j = 0; j < rest.size(); ++j )
							if ( rest[j] == '-' )
								dashpos = j;
								
						if ( dashpos == rest.size() )
						{
							int64_t num = 0;
							for ( uint64_t j = 0; j < rest.size(); ++j )
								if ( isdigit(rest[j]) )
								{
									num *= 10;
									num += rest[j]-'0';
								}
								else if ( rest[j] == ',' )
								{
								
								}
								else
								{
									libmaus2::exception::LibMausException se;
									se.getStream() << "Found invalid range character in " << rest << std::endl;
									se.finish();
									throw se;
								}
								
							libmaus2::bambam::BamRange::unique_ptr_type tAi(new libmaus2::bambam::BamRangeHalfOpen(refname,num-1,header));
							A[i] = UNIQUE_PTR_MOVE(tAi);
						}
						else
						{
							std::string const sstart = rest.substr(0,dashpos);
							std::string const send = rest.substr(dashpos+1);
						
							int64_t start = 0;
							for ( uint64_t j = 0; j < sstart.size(); ++j )
								if ( isdigit(sstart[j]) )
								{
									start *= 10;
									start += sstart[j]-'0';
								}
								else if ( sstart[j] == ',' )
								{
								
								}
								else
								{
									libmaus2::exception::LibMausException se;
									se.getStream() << "Found invalid range character in " << sstart << std::endl;
									se.finish();
									throw se;
								}
							int64_t end = 0;
							for ( uint64_t j = 0; j < send.size(); ++j )
								if ( isdigit(send[j]) )
								{
									end *= 10;
									end += send[j]-'0';
								}
								else if ( send[j] == ',' )
								{
								
								}
								else
								{
									libmaus2::exception::LibMausException se;
									se.getStream() << "Found invalid range character in " << send << std::endl;
									se.finish();
									throw se;
								}
								
							libmaus2::bambam::BamRange::unique_ptr_type tAi(new libmaus2::bambam::BamRangeInterval(refname,start-1,end,header));
							A[i] = UNIQUE_PTR_MOVE(tAi);						
						}
					}
				}
				
				return A;
			}
		};
	}
}
#endif
