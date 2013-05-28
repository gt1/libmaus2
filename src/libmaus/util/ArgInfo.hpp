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

#if ! defined(ARGINFO_HPP)
#define ARGINFO_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <libmaus/util/stringFunctions.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/util/Demangle.hpp>
#include <climits>
#include <cerrno>
#include <stdexcept>
#include <libgen.h>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/network/GetHostName.hpp>

#if defined(_WIN32)
#include <direct.h>
#endif

namespace libmaus
{
	namespace util
	{
		struct ArgInfo
		{
			typedef ArgInfo this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			std::string commandline;		
			std::string progname;
			std::map < std::string, std::string > argmap;
			std::vector < std::string > restargs;
			
			bool helpRequested() const;
			static std::string getProgFileName(std::string const & progname);
			static std::string getDefaultTmpFileName(std::string const & progname);
			std::string getDefaultTmpFileName() const;
			static std::string getCurDir();
			static std::string getDirName(std::string absprogname);
			std::string getAbsProgName() const;
			std::string getProgDirName() const;
			void init(std::vector<std::string> const args);
			
			template<typename char_type>
			static std::vector<std::string> argsToVector(int argc, char_type * argv[])
			{
				std::vector<std::string> V;
				for ( int i = 0; i < argc; ++i )
					V.push_back(argv[i]);
				return V;
			}
			
			static std::string reconstructCommandLine(int argc, char const * argv[]);

			ArgInfo(int argc, char * argv[]);
			ArgInfo(int argc, char const * argv[]);
			ArgInfo(std::vector<std::string> const & args);
			
			typedef std::map<std::string,std::string> keymap_type;
			
			ArgInfo(
				std::string const & rprogname,
				keymap_type const & keymap = keymap_type(),
				std::vector<std::string> const & rrestargs = std::vector<std::string>() );
			
			template<typename type>
			static type parseArg(std::string const & arg)
			{
				std::istringstream istr(arg);
				type v;
				istr >> v;
				
				if ( ! istr )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Unable to parse argument " <<
						arg << " as type " <<
						::libmaus::util::Demangle::demangle(v) << std::endl;
					se.finish();
					throw se;
				}

				return v;
			}

			template<typename type>
			type getValue(std::string const & key, type const defaultVal) const
			{
				if ( argmap.find(key) == argmap.end() )
				{
					return defaultVal;
				}
				else
				{
					return parseArg<type>(argmap.find(key)->second);
				}
			}

			template<typename type>
			type getValueUnsignedNumeric(std::string const & key, type const defaultVal) const
			{
				if ( argmap.find(key) == argmap.end() )
				{
					return defaultVal;
				}
				else
				{
					std::string const & sval = argmap.find(key)->second;
					uint64_t l = 0;
					while ( l < sval.size() && isdigit(sval[l]) )
						++l;
					
					if ( ! l )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Value " << sval << " for key " << key << " is not a representation of an unsigned numerical value." << std::endl;
						se.finish();
						throw se;
					}
					// no unit suffix?
					if ( l == sval.size() )
						return parseArg<type>(sval);
					if ( sval.size() - l > 1 )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Value " << sval << " for key " << key << " has unknown suffix " << sval.substr(sval.size()-l) << std::endl;
						se.finish();
						throw se;					
					}
					
					uint64_t mult = 0;
					
					switch ( sval[sval.size()-1] )
					{
						case 'k': mult = 1024ull; break;
						case 'K': mult = 1000ull; break;
						case 'm': mult = 1024ull*1024ull; break;
						case 'M': mult = 1000ull*1000ull; break;
						case 'g': mult = 1024ull*1024ull*1024ull; break;
						case 'G': mult = 1000ull*1000ull*1000ull; break;
						case 't': mult = 1024ull*1024ull*1024ull*1024ull; break;
						case 'T': mult = 1000ull*1000ull*1000ull*1000ull; break;						
						case 'p': mult = 1024ull*1024ull*1024ull*1024ull*1024ull; break;
						case 'P': mult = 1000ull*1000ull*1000ull*1000ull*1000ull; break;
						case 'e': mult = 1024ull*1024ull*1024ull*1024ull*1024ull*1024ull; break;
						case 'E': mult = 1000ull*1000ull*1000ull*1000ull*1000ull*1000ull; break;
						default:
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "Value " << sval << " for key " << key << " has unknown suffix " << sval.substr(sval.size()-l) << std::endl;
							se.finish();
							throw se;							
						}
					}
					
					return parseArg<type>(sval.substr(0,l)) * mult;
				}
			}
			
			bool hasArg(std::string const & key) const;

			template<typename type>
			type getRestArg(uint64_t const i) const
			{
				if ( i < restargs.size() )
				{
					return parseArg<type>(restargs[i]);
				}
				else
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Argument index out of range in getRestArg()";
					se.finish();
					throw se;
				}
			}
			
			std::string stringRestArg(uint64_t const i) const;
		};
		
		std::ostream & operator<<(std::ostream & out, ArgInfo const & arginfo);
	}
}
#endif
