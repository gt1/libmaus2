/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_UTIL_ARGINFOPARSEBASE_HPP)
#define LIBMAUS2_UTIL_ARGINFOPARSEBASE_HPP

#include <sstream>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/util/Demangle.hpp>

namespace libmaus2
{
	namespace util
	{
		struct ArgInfoParseBase
		{
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
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Unable to parse argument " <<
						arg << " as type " <<
						::libmaus2::util::Demangle::demangle(v) << std::endl;
					se.finish();
					throw se;
				}

				return v;
			}

			template<typename type>
			static type parseValueUnsignedNumeric(std::string const & key, std::string const & sval)
			{
				uint64_t l = 0;
				while ( l < sval.size() && isdigit(sval[l]) )
					++l;

				if ( ! l )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Value " << sval << " for key " << key << " is not a representation of an unsigned numerical value." << std::endl;
					se.finish();
					throw se;
				}
				// no unit suffix?
				if ( l == sval.size() )
					return parseArg<type>(sval);
				if ( sval.size() - l > 1 )
				{
					::libmaus2::exception::LibMausException se;
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
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Value " << sval << " for key " << key << " has unknown suffix " << sval.substr(sval.size()-l) << std::endl;
						se.finish();
						throw se;
					}
				}

				return parseArg<type>(sval.substr(0,l)) * mult;
			}
		};
	}
}
#endif
