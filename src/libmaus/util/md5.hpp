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

#if ! defined(LIBMAUS_UTIL_MD5_HPP)
#define LIBMAUS_UTIL_MD5_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/md5.h>
#include <libmaus/math/UnsignedInteger.hpp>
#include <libmaus/digest/DigestBase.hpp>

#include <iomanip>
#include <sstream>

namespace libmaus
{
	namespace util
	{
		struct MD5 : public libmaus::digest::DigestBase<16>
		{
			void * ctx;
		
			static bool md5(std::string const & input, std::string & output);
			static bool md5(std::vector<std::string> const & V, std::string & output);
			static bool md5(std::vector<std::string> const & V, uint64_t const k, std::string & output);
			static void md5(uint8_t const * in, size_t const len, uint8_t digest[digestlength]);
			static void md5(uint8_t const * in, size_t const len, libmaus::math::UnsignedInteger<digestlength/4> & digest);
			static libmaus::math::UnsignedInteger<digestlength/4> md5(uint8_t const * in, size_t const len);
			
			MD5();
			~MD5();

			void init();
			void update(uint8_t const * t, size_t l);
			void copyFrom(MD5 const & O);
			void digest(uint8_t * digest);
		};
	}
}
#endif
