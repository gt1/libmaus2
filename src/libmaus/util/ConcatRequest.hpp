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

#if ! defined(CONCATREQUEST_HPP)
#define CONCATREQUEST_HPP

#include <libmaus/util/StringSerialisation.hpp>
#include <libmaus/util/Concat.hpp>

namespace libmaus
{
	namespace util
	{
		struct ConcatRequest
		{
			typedef ConcatRequest this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			std::vector<std::string> const infilenames;
			std::string const outfilename;

			ConcatRequest(
				std::vector<std::string> const & rinfilenames,
				std::string const & routfilename);
			ConcatRequest(std::istream & in);
			
			void serialise(std::ostream & out) const;
			void serialise(std::string const & filename) const;
			static void serialise(
				std::vector<std::string> const & infilenames,
				std::string const & outfilename,
				std::string const & requestfilename);
			static void serialise(
				std::string const & infilename,
				std::string const & outfilename,
				std::string const & requestfilename);
			void execute(bool const removeFiles = true) const;
			static ::libmaus::util::ConcatRequest::unique_ptr_type load(std::string const & requestfilename);
		};
	}
}
#endif
