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
#include <libmaus/util/StringSerialisation.hpp>
#include <libmaus/util/NumberSerialisation.hpp>

#if defined(_WIN32)
#include <direct.h>
#endif

namespace libmaus
{
	namespace util
	{
		/**
		 * class for storing and processing command line arguments
		 **/
		struct ArgInfo
		{
			//! this type
			typedef ArgInfo this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			//! complete command line
			std::string commandline;
			//! program name (i.e. argv[0])
			std::string progname;
			//! argument map from key=value pairs
			std::map < std::string, std::string > argmap;
			//! argument multimap from key=value pairs
			std::multimap < std::string, std::string > argmultimap;
			//! rest of arguments behind key=value pairs
			std::vector < std::string > restargs;
			
			void removeKey(std::string const & key)
			{
				if ( argmap.find(key) != argmap.end() )
					argmap.erase(argmap.find(key));
				while ( argmultimap.find(key) != argmultimap.end() )
					argmultimap.erase(argmultimap.find(key));
			}
			
			void insertKey(std::string const & key, std::string const & value)
			{
				argmap[key] = value;
				argmultimap.insert(std::pair<std::string,std::string>(key,value));
			}
			
			void replaceKey(std::string const & key, std::string const & value)
			{
				removeKey(key);
				insertKey(key,value);
			}
			
			bool operator==(ArgInfo const & o) const
			{
				return
					commandline == o.commandline &&
					progname == o.progname &&
					argmap == o.argmap &&
					argmultimap == o.argmultimap &&
					restargs == o.restargs;
			}
			
			bool operator!=(ArgInfo const & o) const
			{
				return !(*this == o);
			}
			
			/**
			 * serialise object
			 *
			 * @param ostream output stream
			 **/
			void serialise(std::ostream & out) const
			{
				libmaus::util::StringSerialisation::serialiseString(out,commandline);
				libmaus::util::StringSerialisation::serialiseString(out,progname);

				libmaus::util::NumberSerialisation::serialiseNumber(out,argmap.size());
				for ( std::map < std::string, std::string >::const_iterator ita = argmap.begin();
					ita != argmap.end(); ++ita )
				{
					libmaus::util::StringSerialisation::serialiseString(out,ita->first);
					libmaus::util::StringSerialisation::serialiseString(out,ita->second);
				}

				libmaus::util::NumberSerialisation::serialiseNumber(out,argmultimap.size());
				for ( std::multimap < std::string, std::string >::const_iterator ita = argmultimap.begin();
					ita != argmultimap.end(); ++ita )
				{
					libmaus::util::StringSerialisation::serialiseString(out,ita->first);
					libmaus::util::StringSerialisation::serialiseString(out,ita->second);
				}
				
				libmaus::util::StringSerialisation::serialiseStringVector(out,restargs);
			}
			
			/**
			 * deserialise object from input stream
			 *
			 * @param in input stream
			 **/
			void deserialise(std::istream & in)
			{
				commandline = libmaus::util::StringSerialisation::deserialiseString(in);
				progname = libmaus::util::StringSerialisation::deserialiseString(in);
				
				uint64_t const mapsize = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				for ( uint64_t i = 0; i < mapsize; ++i )
				{
					std::string const key = libmaus::util::StringSerialisation::deserialiseString(in);
					std::string const val = libmaus::util::StringSerialisation::deserialiseString(in);
					argmap[key] = val;
				}
				uint64_t const multimapsize = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				for ( uint64_t i = 0; i < multimapsize; ++i )
				{
					std::string const key = libmaus::util::StringSerialisation::deserialiseString(in);
					std::string const val = libmaus::util::StringSerialisation::deserialiseString(in);
					argmap.insert(std::pair<std::string,std::string>(key,val));
				}

				restargs = libmaus::util::StringSerialisation::deserialiseStringVector(in);
			}
			
			/**
			 * @return true iff argument -h was given first
			 **/
			bool helpRequested() const;
			/**
			 * compute program file name
			 *
			 * @param program path name
			 * @return program file name
			 **/
			static std::string getProgFileName(std::string const & progname);
			/**
			 * compute a temp file name base from the given program name
			 *
			 * @param program name
			 * @return temp file name base
			 **/
			static std::string getDefaultTmpFileName(std::string const & progname);
			/**
			 * @return default temporary file name composed of program name,
			*          current time and host name
			 **/
			std::string getDefaultTmpFileName() const;
			/**
			 * @return current working directory
			 **/
			static std::string getCurDir();
			/**
			 * extract directory name from full path name
			 *
			 * @param full path name
			 * @return directory name part of absprogname
			 **/
			static std::string getDirName(std::string absprogname);
			/**
			 * @return absolute program path
			 **/
			std::string getAbsProgName() const;
			/**
			 * @return directory path containing program
			 **/
			std::string getProgDirName() const;
			/**
			 * initialise object from list of arguments
			 **/
			void init(std::vector<std::string> const args);
			
			/**
			 * transform argument array to string vector
			 *
			 * @param argc number of arguments
			 * @param argv argument string array
			 * @return argument array converted to a vector of strings
			 **/
			template<typename char_type>
			static std::vector<std::string> argsToVector(int argc, char_type * argv[])
			{
				std::vector<std::string> V;
				for ( int i = 0; i < argc; ++i )
					V.push_back(argv[i]);
				return V;
			}
			
			/**
			 * reconstruct command line from argument array
			 *
			 * @param argc number of arguments
			 * @param argv argument string array
			 * @return command line
			 **/
			static std::string reconstructCommandLine(int argc, char const * argv[]);

			/**
			 * constructor from serialised object
			 *
			 * @param istr input stream
			 **/
			ArgInfo(std::istream & in)
			{
				deserialise(in);
			}

			/**
			 * constructor from argument array
			 *
			 * @param argc number of arguments
			 * @param argv argument string array
			 **/
			ArgInfo(int argc, char * argv[]);
			/**
			 * constructor from const argument array
			 *
			 * @param argc number of arguments
			 * @param argv argument string array
			 **/
			ArgInfo(int argc, char const * argv[]);
			/**
			 * constructor from string vector
			 *
			 * @param args string vector
			 **/
			ArgInfo(std::vector<std::string> const & args);
			
			//! key map type
			typedef std::map<std::string,std::string> keymap_type;
			
			/**
			 * constructor from pre parsed argument information
			 *
			 * @param rprogname program name
			 * @param keymap key=value pair map
			 * @param rrestargs non/post key=value type arguments
			 **/
			ArgInfo(
				std::string const & rprogname,
				keymap_type const & keymap = keymap_type(),
				std::vector<std::string> const & rrestargs = std::vector<std::string>() );
			
			/**
			 * parse given string type argument as type using the corresponding istream operator
			 *
			 * @param arg argument
			 * @return parsed argument
			 **/
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
			
			/**
			 * get unparsed (string) value for key
			 *
			 * @param key id of key=value pair
			 * @param defaultVal default value if no key=value pair is present
			 * @return value
			 **/
			std::string getUnparsedValue(std::string const & key, std::string const defaultVal) const
			{
				if ( argmap.find(key) == argmap.end() )
				{
					return defaultVal;
				}
				else
				{
					return argmap.find(key)->second;
				}	
			}

			/**
			 * get value for key. if there is no key=value pair, use defaultVal instead
			 *
			 * @param key id of key=value pair
			 * @param defaultVal default value if no key=value pair is present
			 * @return value parsed as type
			 **/
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

			/**
			 * get value for key; if there is no key=value pair, use defaultVal instead;
			 * the value is parsed as an unsigned number with a unit like
			 * k=1024, K=1000, m=1024*1024, M=1000*1000, etc.
			 *
			 * @param key id of key=value pair
			 * @param defaultVal default value if no key=value pair is present
			 * @return value parsed as type
			 **/
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
			
			/**
			 * check whether a key=value pair is present for key
			 *
			 * @param key id of key
			 * @return true iff a key=value pair is present
			 **/
			bool hasArg(std::string const & key) const;

			/**
			 * get i'th post key=value argument
			 *
			 * @param i rank of post key=value argument
			 * @return i'th post key=value argument parsed as type
			 **/
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

			/**
			 * get i'th post key=value argument
			 *
			 * @param i rank of post key=value argument
			 * @return i'th post key=value argument
			 **/
			std::string getUnparsedRestArg(uint64_t const i) const
			{
				if ( i < restargs.size() )
				{
					return restargs[i];
				}
				else
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Argument index out of range in getUnparsedRestArg()";
					se.finish();
					throw se;
				}
			}
			
			/**
			 * get i'th post key=value argument as string
			 *
			 * @param i rank of post key=value argument
			 * @return i'th post key=value argument as string
			 **/
			std::string stringRestArg(uint64_t const i) const;
			
			/**
			 * get number of key=value pairs for key
			 *
			 * @param key
			 * @return number of key=value pairs
			 **/
			uint64_t getPairCount(std::string const & key) const
			{
				std::pair <
					std::multimap < std::string, std::string >::const_iterator,
					std::multimap < std::string, std::string >::const_iterator
				> P = argmultimap.equal_range(key);
				
				uint64_t cnt = 0;
				while ( P.first != P.second )
				{
					++cnt;
					++P.first;
				}
				
				return cnt;
			}
			
			/**
			 * get values for key=value pairs
			 *
			 * @param key
			 * @return vector of values
			 **/
			std::vector<std::string> getPairValues(std::string const & key) const
			{
				std::pair <
					std::multimap < std::string, std::string >::const_iterator,
					std::multimap < std::string, std::string >::const_iterator
				> P = argmultimap.equal_range(key);
				
				std::vector<std::string> V;
				
				while ( P.first != P.second )
				{
					V.push_back(P.first->second);
					P.first++;
				}
				
				return V;
			
			}

			/**
			 * format a number using a unit which can be parsed by getValueUnsignedNumeric
			 *
			 * @param n number
			 * @return reformatted number as a string
			 **/
			static std::string numToUnitNum(uint64_t n)
			{
				std::ostringstream ostr;
				
				if ( ! n )
				{
					ostr << n;
				}
				if ( n % 1024 == 0 )
				{
					char u[] = {'k','m','g','t','p','e',0};
					
					unsigned int i = 0;
					while ( u[i] && (n % 1024 == 0) )
					{
						++i;
						n /= 1024;
					}
					
					ostr << n << u[i-1];
				}
				else if ( n % 1000 == 0 )
				{
					char u[] = {'K','M','G','T','P','E',0};
					
					unsigned int i = 0;
					while ( u[i] && (n % 1000 == 0) )
					{
						++i;
						n /= 1000;
					}
					
					ostr << n << u[i-1];
				
				}
				else
				{
					ostr << n;
				}
				
				return ostr.str();
			}
		};
		
		/**
		 * format ArgInfo object for output
		 *
		 * @param out output stream
		 * @param arginfo object to be formatted
		 * @return out
		 **/
		std::ostream & operator<<(std::ostream & out, ArgInfo const & arginfo);
	}
}
#endif
