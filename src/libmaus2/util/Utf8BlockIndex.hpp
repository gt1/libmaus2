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
#if ! defined(LIBMAUS2_UTIL_UTF8BLOCKINDEX_HPP)
#define LIBMAUS2_UTIL_UTF8BLOCKINDEX_HPP

#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/util/utf8.hpp>
#include <libmaus2/math/numbits.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/Utf8BlockIndexDecoder.hpp>

namespace libmaus2
{
	namespace util
	{
		struct Utf8BlockIndex
		{
			typedef Utf8BlockIndex this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			uint64_t blocksize;
			uint64_t lastblocksize;
			uint64_t maxblockbytes;
			::libmaus2::autoarray::AutoArray<uint64_t> blockstarts;
			
			private:
			Utf8BlockIndex();
			
			public:
			template<typename stream_type>
			void serialise(stream_type & stream) const
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,blocksize);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,lastblocksize);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,maxblockbytes);
				::libmaus2::util::NumberSerialisation::serialiseNumber(stream,blockstarts.size()-1);
				for ( uint64_t i = 0; i < blockstarts.size(); ++i )
					::libmaus2::util::NumberSerialisation::serialiseNumber(stream,blockstarts[i]);	
			}
			std::string serialise() const;
			
			static unique_ptr_type constructFromSerialised(std::string const & fn);
			static unique_ptr_type constructFromUtf8File(std::string const & fn, uint64_t const rblocksize = 16ull*1024ull);
		};
	}
}
#endif
