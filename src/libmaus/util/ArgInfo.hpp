/**
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
**/

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
			
			bool helpRequested() const
			{
				return 
					(argmap.size() == 0) &&
					(restargs.size()==1) &&
					(
						restargs[0] == "-h"
						||
						restargs[0] == "--help"
					);
			}
			
			static std::string getProgFileName(std::string const & progname)
			{
				int64_t l = -1;
				
				for ( uint64_t i = 0; i < progname.size(); ++i )
					if ( progname[i] == '/' )
						l = i;
				
				if ( l < 0 )
					return progname;
				else
					return progname.substr(l+1);
			}
			
			static std::string getDefaultTmpFileName(std::string const & progname)
			{
				std::ostringstream ostr;
				ostr << getProgFileName(progname) 
					<< "_" << ::libmaus::network::GetHostName::getHostName()
					<< "_" << getpid()
					<< "_" << time(0);
				return ostr.str();
			}
			
			std::string getDefaultTmpFileName() const
			{
				return getDefaultTmpFileName(progname);
			}

			static std::string getCurDir()
			{
				size_t len = PATH_MAX;
				char * p = 0;
				::libmaus::autoarray::AutoArray<char> Acurdir;
				
				do
				{
					Acurdir = ::libmaus::autoarray::AutoArray<char>(len+1);
					#if defined(_WIN32)
					p = _getcwd(Acurdir.get(), Acurdir.size()-1);
					#else
					p = getcwd ( Acurdir.get(), Acurdir.size()-1 );
					#endif
					len *= 2;
				}
				while ( (!p) && errno==ERANGE );
				
				if ( p )
					return std::string(Acurdir.get());
				else
				{
					throw std::runtime_error("Cannot get current directory.");
				}
			}

			static std::string getDirName(std::string absprogname)
			{
				::libmaus::autoarray::AutoArray<char> Aprogname(absprogname.size()+1);
				std::copy ( absprogname.begin(), absprogname.end(), Aprogname.get() );
				char * pdirname = dirname ( Aprogname.get() );
				return std::string ( pdirname );
			}

			std::string getAbsProgName() const
			{
				if ( progname.size() > 0 && progname[0] == '/' )
					return progname;
				else
					return getCurDir() + "/" + progname;
			}
			
			std::string getProgDirName() const
			{
				std::string absprog = getAbsProgName();
				return absprog.substr(0,absprog.find_last_of("/"));
			}
			
			void init(std::vector<std::string> const args)
			{
				uint64_t i = 0;
				progname = args[i++];

				for ( ; (i < args.size()) && std::string(args[i]).find('=') != std::string::npos ; ++i )
				{
					std::pair<std::string,std::string> valpair = ::libmaus::util::stringFunctions::tokenizePair<std::string>(args[i],std::string("="));
					argmap[valpair.first] = valpair.second;
				}
				
				for ( ; i < args.size(); ++i )
					restargs.push_back(args[i]);
			}
			
			template<typename char_type>
			static std::vector<std::string> argsToVector(int argc, char_type * argv[])
			{
				std::vector<std::string> V;
				for ( int i = 0; i < argc; ++i )
					V.push_back(argv[i]);
				return V;
			}
			
			static std::string reconstructCommandLine(int argc, char const * argv[])
			{
				// "reconstruct" command line
				std::string cl;
				for ( int i = 0; i < argc; ++i )
				{
					cl += argv[i];
					if ( i+1 < argc )
						cl += " ";
				}			
				
				return cl;
			}

			ArgInfo(int argc, char * argv[])
			: commandline(reconstructCommandLine(argc,const_cast<char const **>(argv)))
			{
				init(argsToVector(argc,argv));
			}
			ArgInfo(int argc, char const * argv[])
			: commandline(reconstructCommandLine(argc,argv))
			{
				init(argsToVector(argc,argv));
			}
			ArgInfo(std::vector<std::string> const & args)
			{
				init(args);
			}
			
			typedef std::map<std::string,std::string> keymap_type;
			
			ArgInfo(
				std::string const & rprogname,
				keymap_type const & keymap = keymap_type(),
				std::vector<std::string> const & rrestargs = std::vector<std::string>() )
			{
				std::vector<std::string> V;
				V.push_back(rprogname);
				for ( std::map<std::string,std::string>::const_iterator ita = keymap.begin(); ita != keymap.end(); ++ita )
					V.push_back ( ita->first + "=" + ita->second );
				for ( uint64_t i = 0; i < rrestargs.size(); ++i )
					V.push_back ( rrestargs[i] );
				init(V);
			}
			
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
			
			bool hasArg(std::string const & key) const
			{
				return argmap.find(key) != argmap.end();
			}

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
			
			std::string stringRestArg(uint64_t const i) const
			{
				if ( i < restargs.size() )
				{
					return restargs[i];
				}
				else
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Argument index out of range in stringRestArg()";
					se.finish();
					throw se;
				}
			
			}
		};
		
		inline std::ostream & operator<<(std::ostream & out, ArgInfo const & arginfo)
		{
			out << "ArgInfo(progname=" << arginfo.progname << ",{";
			for ( 
				std::map<std::string,std::string>::const_iterator ita = arginfo.argmap.begin();
				ita != arginfo.argmap.end(); )
			{
				out << ita->first << "=" << ita->second;
				++ita;
				if ( ita != arginfo.argmap.end() )
					out << ";";
			}
			out << "},[";
			for ( uint64_t i = 0; i < arginfo.restargs.size(); ++i )
			{
				out << arginfo.restargs[i];
				if ( i+1 < arginfo.restargs.size() )
					out << ",";
			}
			out << "])";
			return out;
		}
	}
}
#endif
