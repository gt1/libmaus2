/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if !defined(LIBMAUS2_UTIL_COMMAND_HPP)
#define LIBMAUS2_UTIL_COMMAND_HPP

#include <libmaus2/util/StringSerialisation.hpp>
#include <libmaus2/util/GetFileSize.hpp>

namespace libmaus2
{
	namespace util
	{
		struct Command
		{
			std::string in;
			std::string out;
			std::string err;
			std::string returncode;
			std::vector<std::string> args;

			uint64_t numattempts;
			uint64_t maxattempts;
			bool completed;
			bool ignorefail;
			bool deepsleep;

			Command()
			{

			}
			Command(std::string const & rin, std::string const & rout, std::string const & rerr, std::string const & rreturncode, std::vector<std::string> const & rargs)
			: in(rin), out(rout), err(rerr), returncode(rreturncode), args(rargs), numattempts(0), maxattempts(0), completed(false), ignorefail(false), deepsleep(false)
			{
			}
			Command(std::istream & istr)
			{
				deserialise(istr);
			}

			void serialise(std::ostream & ostr) const
			{
				libmaus2::util::StringSerialisation::serialiseString(ostr,in);
				libmaus2::util::StringSerialisation::serialiseString(ostr,out);
				libmaus2::util::StringSerialisation::serialiseString(ostr,err);
				libmaus2::util::StringSerialisation::serialiseString(ostr,returncode);
				libmaus2::util::StringSerialisation::serialiseStringVector(ostr,args);
				libmaus2::util::NumberSerialisation::serialiseNumber(ostr,numattempts);
				libmaus2::util::NumberSerialisation::serialiseNumber(ostr,maxattempts);
				libmaus2::util::NumberSerialisation::serialiseNumber(ostr,completed);
				libmaus2::util::NumberSerialisation::serialiseNumber(ostr,ignorefail);
				libmaus2::util::NumberSerialisation::serialiseNumber(ostr,deepsleep);
			}

			void deserialise(std::istream & istr)
			{
				in = libmaus2::util::StringSerialisation::deserialiseString(istr);
				out = libmaus2::util::StringSerialisation::deserialiseString(istr);
				err = libmaus2::util::StringSerialisation::deserialiseString(istr);
				returncode = libmaus2::util::StringSerialisation::deserialiseString(istr);
				args = libmaus2::util::StringSerialisation::deserialiseStringVector(istr);
				numattempts = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				maxattempts = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				completed = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				ignorefail = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				deepsleep = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
			}

			// check return code
			bool check() const
			{
				try
				{
					if ( libmaus2::util::GetFileSize::fileExists(returncode) )
					{
						libmaus2::aio::InputStreamInstance ISI(returncode);

						uint64_t num = 0;
						unsigned int numcnt = 0;

						while ( ISI && ISI.peek() != std::istream::traits_type::eof() && isdigit(ISI.peek()) )
						{
							int c = ISI.get();

							num *= 10;
							num += c-'0';
							++numcnt;
						}

						if ( ! numcnt )
							return false;

						if (
							! ISI
							||
							ISI.peek() == std::istream::traits_type::eof()
							||
							ISI.peek() != '\n'
						)
							return false;

						ISI.get();

						return
							ISI.peek() == std::istream::traits_type::eof()
							&&
							num == 0;
					}
					else
					{
						return false;
					}
				}
				catch(...)
				{
					return false;
				}
			}

			int dispatch() const;
		};

		std::ostream & operator<<(std::ostream & out, Command const & C);
	}
}

#endif
