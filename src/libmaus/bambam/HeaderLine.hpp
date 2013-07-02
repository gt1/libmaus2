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
				return extractLinesByType(header,"PG");
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
