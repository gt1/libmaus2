/*
    libmaus2
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

#include <libmaus2/util/StringSerialisation.hpp>
#include <libmaus2/util/Concat.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * concatenation request class
		 **/
		struct ConcatRequest
		{
			//! this type
			typedef ConcatRequest this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			//! list of input file names
			std::vector<std::string> const infilenames;
			//! output file name
			std::string const outfilename;

			/**
			 * constructor by parameters
			 *
			 * @param rinfilenames list of input file names
			 * @param routfilename output file name
			 **/
			ConcatRequest(
				std::vector<std::string> const & rinfilenames,
				std::string const & routfilename);
			/**
			 * constructor from serialised object
			 *
			 * @param in input stream
			 **/
			ConcatRequest(std::istream & in);

			/**
			 * serialise object
			 *
			 * @param out output stream
			 **/
			void serialise(std::ostream & out) const;
			/**
			 * serialise object to file named filename
			 *
			 * @param filename output file name
			 **/
			void serialise(std::string const & filename) const;
			/**
			 * given the construction parameters serialise a ConcatRequest object to file requestfilename
			 *
			 * @param infilenames input file names for concatenation
			 * @param outfilename output file name for concatenation
			 * @param requestfilename file name for storing the construct ConcatRequest object
			 **/
			static void serialise(
				std::vector<std::string> const & infilenames,
				std::string const & outfilename,
				std::string const & requestfilename);
			/**
			 * given the construction parameters serialise a ConcatRequest object to file requestfilename
			 *
			 * @param infilename single input file name for concatenation
			 * @param outfilename output file name for concatenation
			 * @param requestfilename file name for storing the construct ConcatRequest object
			 **/
			static void serialise(
				std::string const & infilename,
				std::string const & outfilename,
				std::string const & requestfilename);
			/**
			 * execute a concatenation request
			 *
			 * @param removeFiles if true then remove input files after concatenation
			 **/
			void execute(bool const removeFiles = true) const;
			/**
			 * load a ConcatRequest object from a file
			 *
			 * @param requestfilename file name of file containing the serialised request
			 * @return deserialised ConcatRequest object wrapped in unique pointer class
			 **/
			static ::libmaus2::util::ConcatRequest::unique_ptr_type load(std::string const & requestfilename);
		};
	}
}
#endif
