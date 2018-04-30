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
			std::string shell;
			std::string script;

			uint64_t numattempts;
			uint64_t maxattempts;
			bool completed;
			bool ignorefail;
			bool deepsleep;

			Command()
			{

			}
			Command(std::string const & rin, std::string const & rout, std::string const & rerr, std::string const & rshell, std::string const & rscript)
			: in(rin), out(rout), err(rerr), shell(rshell), script(rscript), numattempts(0), maxattempts(0), completed(false), ignorefail(false), deepsleep(false)
			{
			}
			Command(std::istream & istr)
			{
				deserialise(istr);
			}

			bool isComplete() const
			{
				return completed;
			}

			bool isFinished() const
			{
				return isComplete() || ((numattempts == maxattempts) && ignorefail);
			}

			void serialise(std::ostream & ostr) const
			{
				libmaus2::util::StringSerialisation::serialiseString(ostr,in);
				libmaus2::util::StringSerialisation::serialiseString(ostr,out);
				libmaus2::util::StringSerialisation::serialiseString(ostr,err);
				libmaus2::util::StringSerialisation::serialiseString(ostr,shell);
				libmaus2::util::StringSerialisation::serialiseString(ostr,script);
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
				shell = libmaus2::util::StringSerialisation::deserialiseString(istr);
				script = libmaus2::util::StringSerialisation::deserialiseString(istr);
				numattempts = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				maxattempts = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				completed = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				ignorefail = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				deepsleep = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
			}

			int dispatch(std::string const & fn) const;
		};

		std::ostream & operator<<(std::ostream & out, Command const & C);
	}
}

#endif
