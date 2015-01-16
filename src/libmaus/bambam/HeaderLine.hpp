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
#if ! defined(LIBMAUS_BAMBAM_HEADERLINE_HPP)
#define LIBMAUS_BAMBAM_HEADERLINE_HPP

#include <libmaus/util/stringFunctions.hpp>
#include <libmaus/exception/LibMausException.hpp>

#include <set>
#include <stack>
#include <map>
#include <vector>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * SAM/BAM header line class
		 **/
		struct HeaderLine
		{
			//! line text
			std::string line;
			//! type of line (HD, SQ, ...)
			std::string type;
			//! attribute map
			std::map<std::string,std::string> M;
			
			/**
			 * constructor for invalid/empty line
			 **/
			HeaderLine()
			{
			
			}
			
			void printAttributesInOrder(std::ostream & ostr, char const ** args)
			{
				std::set<std::string> primary;
				
				for ( ; *args; ++args )
				{
					if ( M.find(*args) != M.end() )
					{
						std::map<std::string,std::string>::const_iterator ita = M.find(*args);
						ostr << '\t' << ita->first << ":" << ita->second;				
					}
					primary.insert(*args);
				}

				for ( std::map<std::string,std::string>::const_iterator ita = M.begin();
					ita != M.end(); ++ita )
					if ( primary.find(ita->first) == primary.end() )
					{
						ostr << '\t' << ita->first << ":" << ita->second;				
					}
			}
			
			void constructLine()
			{
				std::ostringstream ostr;
				ostr << '@' << type;
				
				if ( type == "HD" )
				{
					char const * args[] = { "VN", "SO", 0 };
					printAttributesInOrder(ostr,&args[0]);
				}
				else if ( type == "SQ" )
				{
					char const * args[] = { "SN", "LN", "AS", "M5", "SP", "UR", 0 };
					printAttributesInOrder(ostr,&args[0]);		
				}
				else if ( type == "RG" )
				{
					char const * args[] = { "ID", "CN", "DS", "DT", "FO", "KS", "LB", "PG", "PI", "PL", "PU", "SM", 0 };
					printAttributesInOrder(ostr,&args[0]);		
				}
				else if ( type == "PG" )
				{
					char const * args[] = { "ID", "PN", "CL", "PP", "DS", "VN", 0 };
					printAttributesInOrder(ostr,&args[0]);		
				}
				else
				{
					for ( std::map<std::string,std::string>::const_iterator ita = M.begin();
						ita != M.end(); ++ita )
						ostr << '\t' << ita->first << ":" << ita->second;
				}
				line = ostr.str();
			}
			
			/**
			 * check line for key
			 *
			 * @param key
			 * @return true iff attribute for key is present
			 **/
			bool hasKey(std::string const & key) const
			{
				return M.find(key) != M.end();
			}
			
			/**
			 * get value for key, throws exception if key is not present
			 *
			 * @param key
			 * @return value for key
			 **/
			std::string getValue(std::string const & key) const
			{
				if ( ! hasKey(key) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "bambam::HeaderLine::getValue called for non existant key: " << key << " for line of type " << type << std::endl;
					se.finish();
					throw se;				
				}
			
				return M.find(key)->second;
			}
			
			/**
			 * @return true iff line is a PG line
			 **/
			bool isProgramLine() const
			{
				return type == "PG";
			}
			
			/**
			 * remove all SQ type lines
			 * 
			 * @param headertext input header text
			 * @return text without SQ lines
			 **/
			static std::string removeSequenceLines(std::string const & headertext)
			{
				std::vector<HeaderLine> const lines = extractLines(headertext);
				std::ostringstream ostr;
				for ( uint64_t i = 0; i < lines.size(); ++i )
					if ( lines[i].type != "SQ" )
						ostr << lines[i].line << '\n';
				return ostr.str();
			}
			
			/**
			 * extract vector of lines from header text
			 *
			 * @param headertext header text
			 * @return vector of header lines
			 **/
			static std::vector<HeaderLine> extractLines(std::string const & headertext)
			{
				std::istringstream istr(headertext);
				
				std::vector<HeaderLine> lines;
				
				while ( istr )
				{
					std::string line;
					std::getline(istr,line);
					
					if ( istr && line.size() )
					{
						HeaderLine const hl(line);
						lines.push_back(hl);
					}
				}
				
				return lines;
			}

			/**
			 * extract vector of lines from header text while keeping only the lines matching the filter
			 *
			 * @param headertext header text
			 * @param filter line type filter
			 * @return vector of header lines matching filter
			 **/
			static std::vector<HeaderLine> extractLinesByType(std::string const & headertext, std::set<std::string> const & filter)
			{
				std::istringstream istr(headertext);
				
				std::vector<HeaderLine> lines;
				
				while ( istr )
				{
					std::string line;
					std::getline(istr,line);
					
					if ( istr && line.size() )
					{
						HeaderLine const hl(line);
						if ( filter.find(hl.type) != filter.end() )
							lines.push_back(hl);
					}
				}
				
				return lines;
			}
			
			/**
			 * extract vector of lines from header text; only lines of type are kept
			 *
			 * @param header header text
			 * @param type line type
			 * @return vector of header lines matching type
			 **/
			static std::vector<HeaderLine> extractLinesByType(std::string const & header, std::string const & type)
			{
				std::set<std::string> S;
				S.insert(type);
				return extractLinesByType(header,S);
			}
			
			/**
			 * extract PG lines from header text
			 *
			 * @param header header text
			 * @return vector containing PG lines
			 **/
			static std::vector<HeaderLine> extractProgramLines(std::string const & header)
			{
				std::vector<HeaderLine> const lines = extractLinesByType(header,"PG");
				
				std::vector<std::string> idvec;
				for ( uint64_t i = 0; i < lines.size(); ++i )
				{
					if ( ! lines[i].hasKey("ID") )
					{
						libmaus::exception::LibMausException se;
						se.getStream() << "PG line without ID field: " << lines[i].line << std::endl;
						se.finish();
						throw se;
					}
					
					idvec.push_back(lines[i].getValue("ID"));
				}
				
				std::sort(idvec.begin(),idvec.end());
				
				for ( uint64_t i = 1; i < idvec.size(); ++i )
					if ( idvec[i] == idvec[i-1] )
					{
						libmaus::exception::LibMausException se;
						se.getStream() << "PG ID " << idvec[i] << " is not unique." << std::endl;
						se.finish();
						throw se;					
					}

				for ( uint64_t i = 0; i < lines.size(); ++i )
					if ( 
						lines[i].hasKey("PP")
					)
					{
						std::string const PP = lines[i].getValue("PP");
						std::pair < 
							std::vector<std::string>::const_iterator,
							std::vector<std::string>::const_iterator >
							const interval = ::std::equal_range(idvec.begin(),idvec.end(),PP);
						
						if ( interval.first == interval.second )
						{
							libmaus::exception::LibMausException se;
							se.getStream() << "PG line " << lines[i].line << " references unknown PG ID via PP key." << std::endl;
							se.finish();
							throw se;					
						}
					}
				
				return lines;
			}
			
			/**
			 * construct object from text line
			 *
			 * @param rline SAM/BAM header line
			 **/
			HeaderLine(std::string const & rline) : line(rline)
			{
				std::deque<std::string> tokens = ::libmaus::util::stringFunctions::tokenize(line,std::string("\t"));
				
				if ( tokens.size() )
				{
					std::string const first = tokens[0];
					
					if ( (! first.size()) || first[0] != '@' || first.size() != 3 )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Malformed SAM header line: " << line << std::endl;
						se.finish();
						throw se;
					}
					
					type = first.substr(1);
					
					for ( uint64_t i = 1; i < tokens.size(); ++i )
					{
						std::string const token = tokens[i];

						if ( !token.size() || token.size() < 3 || token[2] != ':' )
						{
							#if defined(LIBMAUS_BAMBAM_SAMHEADER_STRICT)
							if ( type != "CO" )
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "Malformed SAM header line: " << line << std::endl;
								se.finish();
								throw se;
							}
							#else
							if ( type != "CO" )
								std::cerr << "Malformed SAM header line: " << line << std::endl;
							#endif
						}
						else
						{
							std::string const key = token.substr(0,2);
							std::string const value = token.substr(3);
							M [ key ] = value;
						}
					}
				}
			}
		};
	}
}
#endif
