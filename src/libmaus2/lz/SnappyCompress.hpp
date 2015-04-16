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
#if ! defined(LIBMAUS2_LZ_SNAPPYCOMPRESS_HPP)
#define LIBMAUS2_LZ_SNAPPYCOMPRESS_HPP

#include <libmaus2/lz/IstreamSource.hpp>
#include <libmaus2/aio/IStreamWrapper.hpp>
#include <istream>
#include <ostream>
#include <string>
#include <sstream>

namespace libmaus2
{
	namespace lz
	{
		struct SnappyCompress
		{
			static uint64_t compress(std::istream & in, uint64_t const n, std::ostream & out);
			static uint64_t compress(char const * in, uint64_t const n, std::ostream & out);
			static uint64_t compress(::libmaus2::lz::IstreamSource< ::libmaus2::aio::IStreamWrapper> & in, std::ostream & out);

			static void uncompress(char const * in, uint64_t const insize, std::string & out);
			static void uncompress(std::istream & in, uint64_t const insize, char * out);
			static void uncompress(::libmaus2::lz::IstreamSource< ::libmaus2::aio::IStreamWrapper> & in, char * out, int64_t const length = -1);

			static std::string compress(std::istream & in, uint64_t const n)
			{
				std::ostringstream out;
				compress(in,n,out);
				return out.str();
			}
			static std::string compress(char const * in, uint64_t const n)
			{
				std::ostringstream ostr;
				compress(in,n,ostr);
				return ostr.str();
			}
			static std::string compress(std::string const & in)
			{
				return compress(in.c_str(),in.size());
			}
			static void uncompress(std::string const & in, std::string & out)
			{
				uncompress(in.c_str(),in.size(),out);
			}
			static std::string uncompress(char const * in, uint64_t const insize)
			{
				std::string out;
				uncompress(in,insize,out);
				return out;
			}
			static std::string uncompress(std::string const & in)
			{
				return uncompress(in.c_str(),in.size());
			}

			static bool rawuncompress(char const * compressed, uint64_t compressed_length, char * uncompressed);
			static uint64_t compressBound(uint64_t length);
			static uint64_t rawcompress(
				char const * uncompressed, 
				uint64_t uncompressed_length, 
				char * compressed);
		};
	}
}
#endif
