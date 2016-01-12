/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_UTIL_ARGPARSER_HPP)
#define LIBMAUS2_UTIL_ARGPARSER_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/util/ArgInfoParseBase.hpp>
#include <libmaus2/util/Demangle.hpp>
#include <map>
#include <cassert>

namespace libmaus2
{
	namespace util
	{
		struct ArgParser : public libmaus2::util::ArgInfoParseBase
		{
			typedef ArgParser this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string progname;
			std::string commandline;

			std::multimap<std::string,std::string> kvargs;
			std::vector<std::string> restargs;

			static std::string reconstructCommandLine(int argc, char * argv[])
			{
				std::ostringstream clostr;

				// "reconstruct" command line
				for ( int i = 0; i < argc; ++i )
				{
					clostr << argv[i];
					if ( i+1 < argc )
						clostr << ' ';
				}

				return clostr.str();
			}

			ArgParser()
			{

			}

			ArgParser(int const argc, char * argv[])
			{
				if ( argc )
					progname = argv[0];

				bool parseargs = true;

				for ( int i = 1; i < argc; ++i )
				{
					std::string arg = argv[i];

					if ( parseargs && arg.size() > 1 && arg[0] == '-' )
					{
						if ( arg == "--" )
						{
							parseargs = false;
						}
						else if ( arg[1] != '-' )
							kvargs.insert(std::pair<std::string,std::string>(arg.substr(1,1),arg.substr(2)));
						else
						{
							assert ( arg[0] == '-' && arg[1] == '-' && arg.size() > 2 );

							size_t e = 3;
							while ( e < arg.size() && isalpha(arg[e]) )
								++e;

							kvargs.insert(std::pair<std::string,std::string>(arg.substr(2,e-2),arg.substr(e)));
						}
					}
					else
					{
						restargs.push_back(arg);
					}
				}

				commandline = reconstructCommandLine(argc,argv);
			}

			size_t size() const
			{
				return restargs.size();
			}

			std::string const & operator[](size_t const i) const
			{
				if ( i < size() )
					return restargs[i];
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "ArgParser::operator[]: argument index " << i << " is out of range" << std::endl;
					lme.finish();
					throw lme;
				}
			}

			std::string const & operator[](std::string const & key) const
			{
				typedef std::multimap<std::string,std::string>::const_iterator it_type;
				std::pair<it_type,it_type> P = kvargs.equal_range(key);
				std::vector< std::pair<std::string,std::string> > V;
				while ( P.first != P.second )
				{
					std::pair<std::string,std::string> const & S = *(P.first);
					V.push_back(S);
					P.first++;
				}

				if ( V.size() == 1 )
				{
					assert ( V[0].first == key );
					return V[0].second;
				}
				else if ( V.size() == 0 )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "ArgParser::operator[]: no argument for key " << key << std::endl;
					lme.finish();
					throw lme;
				}
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "ArgParser::operator[]: argument for " << key << " is not unique" << std::endl;
					lme.finish();
					throw lme;
				}
			}

			bool argPresent(std::string const & key) const
			{
				return kvargs.find(key) != kvargs.end();
			}

			bool uniqueArgPresent(std::string const & key) const
			{
				typedef std::multimap<std::string,std::string>::const_iterator it_type;
				std::pair<it_type,it_type> P = kvargs.equal_range(key);
				uint64_t cnt = 0;
				while ( P.first != P.second )
				{
					++(P.first);
					++cnt;
				}
				return cnt == 1;
			}

			template<typename value_type>
			value_type getParsedRestArg(size_t const i) const
			{
				std::istringstream istr((*this)[i]);
				value_type v;
				istr >> v;

				if ( istr && istr.peek() == std::istream::traits_type::eof() )
					return v;
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "ArgParser::getParsedRestArg: cannot parse " << (*this)[i] << " as type " << ::libmaus2::util::Demangle::demangle<value_type>() << std::endl;
					lme.finish();
					throw lme;
				}
			}

			std::string getEqArg(std::string const & key) const
			{
				std::string const value = (*this)[key];
				if ( value.size() && value[0] == '=' )
					return value.substr(1);
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "ArgParser::getEqArg: argument for key " << key << " does not contain a = symbol" << std::endl;
					lme.finish();
					throw lme;
				}
			}

			template<typename value_type>
			value_type getParsedArg(std::string const & key) const
			{
				std::istringstream istr((*this)[key]);
				value_type v;
				istr >> v;

				if ( istr && istr.peek() == std::istream::traits_type::eof() )
					return v;
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "ArgParser::getParsedRestArg: cannot parse " << (*this)[key] << " as type " << ::libmaus2::util::Demangle::demangle<value_type>() << std::endl;
					lme.finish();
					throw lme;
				}
			}

			template<typename value_type>
			value_type getUnsignedNumericRestArg(size_t const i) const
			{
				std::string const s = (*this)[i];

				try
				{
					std::ostringstream ostr;
					ostr << "argument " << i;
					return libmaus2::util::ArgInfoParseBase::parseValueUnsignedNumeric<value_type>(ostr.str(),s);
				}
				catch(...)
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "ArgParser::getUnsignedNumericRestArg: cannot parse " << (*this)[i] << " as unsigned numeric" << std::endl;
					lme.finish();
					throw lme;
				}
			}

			template<typename value_type>
			value_type getUnsignedNumericArg(std::string const & key) const
			{
				std::string const s = (*this)[key];

				try
				{
					std::ostringstream ostr;
					ostr << "argument " << key;
					return libmaus2::util::ArgInfoParseBase::parseValueUnsignedNumeric<value_type>(ostr.str(),s);
				}
				catch(...)
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "ArgParser::getUnsignedNumericRestArg: cannot parse " << (*this)[key] << " as unsigned numeric" << std::endl;
					lme.finish();
					throw lme;
				}
			}
		};

		std::ostream & operator<<(std::ostream & out, ArgParser const & O);
	}
}
#endif
