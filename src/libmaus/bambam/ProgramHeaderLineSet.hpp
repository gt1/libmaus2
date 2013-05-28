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
#if ! defined(LIBMAUS_BAMBAM_PROGRAMHEADERLINESET_HPP)
#define LIBMAUS_BAMBAM_PROGRAMHEADERLINESET_HPP

#include <libmaus/bambam/HeaderLine.hpp>
#include <cassert>

namespace libmaus
{
	namespace bambam
	{
		struct ProgramHeaderLineSet
		{
			std::vector<HeaderLine> lines;
			std::map<std::string,uint64_t> idmap;
			std::vector<int64_t> parent;
			std::vector<uint64_t> roots;
			std::map< uint64_t,std::vector<uint64_t> > edges;
			
			ProgramHeaderLineSet() {}
			ProgramHeaderLineSet(std::string const & headertext)
			: lines(HeaderLine::extractProgramLines(headertext)), parent(lines.size(),-1)
			{
				for ( uint64_t i = 0; i < lines.size(); ++i )
					idmap [ lines[i].getValue("ID") ] = i;
				
				for ( uint64_t i = 0; i < lines.size(); ++i )
					if ( lines[i].hasKey("PP") )
					{
						std::string const PP = lines[i].getValue("PP");
						
						if ( idmap.find(PP) == idmap.end() )
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "Broken sam header: referenced ID by PP " << PP << " does not exist" << std::endl;
							se.finish();
							throw se;						
						}
						
						uint64_t const parid = idmap.find(PP)->second;
						parent[i] = parid;
						edges [ parid ] . push_back(i);
					}
					
				for ( uint64_t i = 0; i < parent.size(); ++i )
					if ( parent[i] < 0 )
						roots.push_back(i);
				
				if ( roots.size() > 1 )
					std::cerr << "WARNING: SAM header designates more than one PG tree root by PP tags." << std::endl;
					
				for ( 
					std::map< uint64_t,std::vector<uint64_t> >::const_iterator ita = edges.begin(); 
					ita != edges.end(); 
					++ita 
				)
					if ( ita->second.size() > 1 )
					{
						std::cerr << "WARNING: PG line with id " << lines[ita->first].getValue("ID") << " has multiple children referencing it by PP tags." << std::endl;
					}
				
				#if 0
				for ( uint64_t i = 0; i < roots.size(); ++i )
				{
					uint64_t const root = roots[i];
					std::cerr << "[ROOT=" << root << "]" << std::endl;
					
					std::stack< std::pair<uint64_t,uint64_t> > S;
					S.push(std::pair<uint64_t,uint64_t>(root,0));
					std::set<uint64_t> seen;
					
					while ( S.size() )
					{
						uint64_t const cur = S.top().first;
						uint64_t const d = S.top().second;
						seen.insert(cur);
						S.pop();
						
						std::cerr << std::string(d,' ');
						std::cerr << lines[cur].getValue("ID") << " [" << cur << "]" << std::endl;
						
						if ( edges.find(cur) != edges.end() )
						{
							std::vector<uint64_t> const & E = edges.find(cur)->second;
							for ( uint64_t j = 0; j < E.size(); ++j )
							{
								if ( seen.find(E[j]) == seen.end() )
								{
									// std::cerr << "Pushing " << E[j] << " as child of " << cur << std::endl;
									S.push(std::pair<uint64_t,uint64_t>(E[j],d+1));
								}
								else
								{
									std::cerr << "Loop detected." << std::endl;
								}
							}
						}
						
					}
				}
				#endif
			}		
			
			std::string getLastIdInChain() const
			{
				// no lines -> no parent ID
				if ( ! lines.size() )
					return std::string();
					
				assert ( roots.size() );	
					
				// more than one root, assume last inserted line was last ID
				if ( roots.size() > 1 )
					return lines.back().getValue("ID");
				
				uint64_t cur = roots[0];
				
				while ( edges.find(cur) != edges.end() )
				{
					std::vector<uint64_t> const & E = edges.find(cur)->second;
					assert ( E.size() );
					if ( E.size() > 1 )
						std::cerr << "WARNING: PG lines in header do not form a linear chain." << std::endl;
					cur = E[0];
				}
				
				return lines[cur].getValue("ID");
			}

			static std::string addProgramLine(
				std::string const & headertext,
				std::string ID,
				std::string const & PN,
				std::string const & CL,
				std::string const & PP,
				std::string const & VN
			)
			{
				std::vector<HeaderLine> hlv = HeaderLine::extractLines(headertext);
				
				std::set<std::string> ids;
				std::string LPP;
				int64_t lp = -1;
				for ( uint64_t i = 0; i < hlv.size(); ++i )
					if ( hlv[i].type == "PG" )
					{
						ids.insert(hlv[i].getValue("ID"));
						LPP = hlv[i].getValue("ID");
						lp = i;
					}
				
				// add ' while ID is not unique		
				while ( ids.find(ID) != ids.end() )
					ID += (char)39; // prime
					
				if ( !ID.size() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ID is empty in addProgramLine" << std::endl;
					se.finish();
					throw se;
				}
				if ( ID == PP )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ID==PP in addProgramLine" << std::endl;
					se.finish();
					throw se;
				}
				if ( PP.size() && (ids.find(PP) == ids.end()) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "PP=" << PP << " does not exist in addProgramLine" << std::endl;
					se.finish();
					throw se;				
				}
				
				std::ostringstream ostr;
				for ( uint64_t i = 0; i < hlv.size(); ++i )
				{
					ostr << hlv[i].line << std::endl;
					if ( 
						static_cast<int64_t>(i) == lp 
						||
						((i+1 == hlv.size()) && lp == -1)
					)
					{
						ostr << "@PG" << "\tID:" << ID;
						if ( PN.size() )
							ostr << "\tPN:" << PN;
						if ( CL.size() )
							ostr << "\tCL:" << CL;
						if ( PP.size() )
							ostr << "\tPP:" << PP;
						if ( VN.size() )
							ostr << "\tVN:" << VN;
						ostr << std::endl;
					}		
				}
				
				return ostr.str();
			}
		};
	}
}
#endif
