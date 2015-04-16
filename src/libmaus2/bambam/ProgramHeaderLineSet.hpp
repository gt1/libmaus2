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
#if ! defined(LIBMAUS2_BAMBAM_PROGRAMHEADERLINESET_HPP)
#define LIBMAUS2_BAMBAM_PROGRAMHEADERLINESET_HPP

#include <libmaus2/bambam/HeaderLine.hpp>
#include <cassert>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * set of PG header lines in a SAM/BAM header
		 **/
		struct ProgramHeaderLineSet
		{
			//! vector of PG lines
			std::vector<HeaderLine> lines;
			//! map ID -> line number
			std::map<std::string,uint64_t> idmap;
			//! parent map (parent[i] is the parent of line i, -1 if not parent)
			std::vector<int64_t> parent;
			//! vector of roots (this should be exactly one unless the header is broken)
			std::vector<uint64_t> roots;
			//! PG tree edges
			std::map< uint64_t,std::vector<uint64_t> > edges;
			
			/**
			 * constructor for empty PG line set
			 **/
			ProgramHeaderLineSet() {}
			/**
			 * constructor from SAM/BAM header text
			 **/
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
							::libmaus2::exception::LibMausException se;
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
			
			/**
			 * @return last id in PG id chain
			 **/
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

			/**
			 * add PG line at the end of the current list; prime symbols (ASCII 39) will be added
			 * to the ID string until it is unique relative to the already existing ones
			 *
			 * @param headertext header text
			 * @param ID of line to be added
			 * @param PN program name of line to be added
			 * @param CL command line of line to be added
			 * @param PP previous program id (use getLastIdInChain() to obtain it), empty string for none
			 * @param VN version number of line to be added
			 **/
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
					::libmaus2::exception::LibMausException se;
					se.getStream() << "ID is empty in addProgramLine" << std::endl;
					se.finish();
					throw se;
				}
				if ( ID == PP )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "ID==PP in addProgramLine" << std::endl;
					se.finish();
					throw se;
				}
				if ( PP.size() && (ids.find(PP) == ids.end()) )
				{
					::libmaus2::exception::LibMausException se;
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

			/**
			 * add PG line at the end of the current list; prime symbols (ASCII 39) will be added
			 * to the ID string until it is unique relative to the already existing ones; the modified
			 * ID string is return via the reference given in the ID parameter
			 *
			 * @param headertext header text
			 * @param ID of line to be added
			 * @param PN program name of line to be added
			 * @param CL command line of line to be added
			 * @param PP previous program id (use getLastIdInChain() to obtain it), empty string for none
			 * @param VN version number of line to be added
			 **/
			static std::string addProgramLineRef(
				std::string const & headertext,
				std::string & ID,
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
					::libmaus2::exception::LibMausException se;
					se.getStream() << "ID is empty in addProgramLine" << std::endl;
					se.finish();
					throw se;
				}
				if ( ID == PP )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "ID==PP in addProgramLine" << std::endl;
					se.finish();
					throw se;
				}
				if ( PP.size() && (ids.find(PP) == ids.end()) )
				{
					::libmaus2::exception::LibMausException se;
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

		/**
		 **/
		struct ProgramHeaderLinesMerge
		{
			typedef ProgramHeaderLinesMerge this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			std::string PGtext;
			std::vector < std::string > PGids;
			libmaus2::autoarray::AutoArray< ::libmaus2::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type > tries;
			std::vector < std::vector<uint64_t> > triedictmaps;

			std::string const & mapPG(uint64_t const srcfileid, char const * from)
			{
				uint64_t const len = strlen(from);
				int64_t const id = tries[srcfileid]->searchCompleteNoFailure(from,from+len);
				
				if ( id >= 0 && id < static_cast<int64_t>(PGids.size()) )
					return PGids[triedictmaps[srcfileid][id]];
				else
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "Unknown PG id in mapPG: " << std::string(from,from+len) << std::endl;
					se.finish();
					throw se;
				}
			}
		
			ProgramHeaderLinesMerge(
				std::vector< std::string const * > const & headers
			) 
			: tries(headers.size())
			{
				std::vector < HeaderLine > PGlines;
				std::vector < std::vector < HeaderLine > > headerlines;
				std::vector < std::map<std::string,std::string> > PGlinesmappingS;
				
				std::vector<std::string> lastinchain;
				
				// extract pg lines and get last in chain ids
				for ( uint64_t i = 0; i < headers.size(); ++i )
				{
					headerlines.push_back(HeaderLine::extractProgramLines(*(headers[i])));
					lastinchain.push_back(ProgramHeaderLineSet(*(headers[i])).getLastIdInChain());
				}

				// count number of occurences for each PG ID
				std::map<std::string,uint64_t> idcntmap;
				for ( uint64_t i = 0; i < headerlines.size(); ++i )
					for ( uint64_t j = 0; j < headerlines[i].size(); ++j )
					{
						HeaderLine & line = headerlines[i][j];
						
						if ( ! line.hasKey("ID") )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "ProgramHeaderLinesMerge: PG line without ID field is invalid: " << line.line << std::endl;
							lme.finish();
							throw lme;
						}

						idcntmap[line.getValue("ID")]++;
					}

				// assign new numerical id to each occuring more than once
				std::map<std::string,uint64_t> idcntremap;
				std::map< std::pair<uint64_t,uint64_t>, uint64_t > idremap;
				for ( uint64_t i = 0; i < headerlines.size(); ++i )
					for ( uint64_t j = 0; j < headerlines[i].size(); ++j )
					{
						HeaderLine & line = headerlines[i][j];
						std::string const ID = line.getValue("ID");
						std::map<std::string,uint64_t>::const_iterator it = idcntmap.find(ID);
						assert ( it != idcntmap.end() );
						assert ( it->second > 0 );
						if ( it->second > 1 )
							idremap[ std::pair<uint64_t,uint64_t>(i,j) ] = idcntremap[ID]++;
					}
									
				std::ostringstream PGtextstr;
				std::set < std::string > gids;	
				for ( uint64_t i = 0; i < headerlines.size(); ++i )
				{
					std::map<std::string,std::string> lidmap;
					
					for ( uint64_t j = 0; j < headerlines[i].size(); ++j )
					{
						HeaderLine & line = headerlines[i][j];
						assert ( line.hasKey("ID") );
						
						std::string const origID = line.getValue("ID");
						std::string ID = origID;
						
						if ( idremap.find(std::pair<uint64_t,uint64_t>(i,j)) != idremap.end() )
						{
							std::ostringstream idostr;
							idostr << origID << "_" << idremap.find(std::pair<uint64_t,uint64_t>(i,j))->second;
							ID = idostr.str();
						}
						
						while ( gids.find(ID) != gids.end() )
							ID += '\'';
							
						#if 0
						if ( ID != origID )
							std::cerr << "[D] replacing PG ID " << origID << " by " << ID << std::endl;
						#endif
						
						line.M["ID"] = ID;
						
						lidmap [ origID ] = ID;
						gids.insert(ID);
						
						if ( lastinchain[i] == origID )
							lastinchain[i] = ID;
					}

					for ( uint64_t j = 0; j < headerlines[i].size(); ++j )
					{
						HeaderLine & line = headerlines[i][j];
						
						if ( line.hasKey("PP") )
							line.M["PP"] = lidmap.find(line.getValue("PP"))->second;
						else if ( i > 0 && lastinchain[i-1].size() )
							line.M["PP"] = lastinchain[i-1];
							
						line.constructLine();
						
						PGlines.push_back(line);
						PGtextstr << line.line << std::endl;
					}
					
					PGlinesmappingS.push_back(lidmap);
				}

				std::map < std::string, uint64_t > PGidmap;
				for ( std::set < std::string >::const_iterator ita = gids.begin();
					ita != gids.end(); ++ita )
				{
					PGidmap [ *ita ] = PGids.size();
					PGids.push_back(*ita);
				}

				for ( uint64_t i = 0; i < PGlinesmappingS.size(); ++i )
				{
					std::map<std::string,std::string> const & M = PGlinesmappingS[i];
					std::vector < std::string > ldict;
					std::vector < uint64_t > ldictmap;
					
					for ( std::map<std::string,std::string>::const_iterator ita = M.begin();
						ita != M.end(); ++ita )
					{
						std::string const & from = ita->first;
						std::string const & to = ita->second;
						uint64_t const toid = PGidmap.find(to)->second;
						ldictmap.push_back(toid);
						ldict.push_back(from);
					}
					
					::libmaus2::trie::Trie<char> trienofailure;
					trienofailure.insertContainer(ldict);
					::libmaus2::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type LHTnofailure 
						(trienofailure.toLinearHashTrie<uint32_t>());
						
					tries[i] = UNIQUE_PTR_MOVE(LHTnofailure);
						
					triedictmaps.push_back(ldictmap);
				}
				
				PGtext = PGtextstr.str();
				
				#if 0
				headerlines.resize(0);
				for ( uint64_t i = 0; i < headers.size(); ++i )
					headerlines.push_back(HeaderLine::extractProgramLines(*(headers[i])));
				for ( uint64_t i = 0; i < headerlines.size(); ++i )
					for ( uint64_t j = 0; j < headerlines[i].size(); ++j )
					{
						HeaderLine const & H = headerlines[i][j];
						std::cerr << H.getValue("ID") << "\t" << mapPG(i,H.getValue("ID").c_str()) << std::endl;
					}
				#endif
			}
		};
	}
}
#endif
