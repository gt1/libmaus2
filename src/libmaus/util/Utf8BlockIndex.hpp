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
#if ! defined(LIBMAUS_UTIL_UTF8BLOCKINDEX_HPP)
#define LIBMAUS_UTIL_UTF8BLOCKINDEX_HPP

#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/util/utf8.hpp>
#include <libmaus/math/numbits.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/util/Utf8BlockIndexDecoder.hpp>

namespace libmaus
{
	namespace util
	{
		struct Utf8BlockIndex
		{
			typedef Utf8BlockIndex this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			uint64_t blocksize;
			uint64_t lastblocksize;
			uint64_t maxblockbytes;
			::libmaus::autoarray::AutoArray<uint64_t> blockstarts;
			
			private:
			Utf8BlockIndex();
			
			public:
			template<typename stream_type>
			void serialise(stream_type & stream) const
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,blocksize);
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,lastblocksize);
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,maxblockbytes);
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,blockstarts.size()-1);
				for ( uint64_t i = 0; i < blockstarts.size(); ++i )
					::libmaus::util::NumberSerialisation::serialiseNumber(stream,blockstarts[i]);	
			}
			std::string serialise() const;
			
			static unique_ptr_type constructFromSerialised(std::string const & fn);
			static unique_ptr_type constructFromUtf8File(std::string const & fn, uint64_t const rblocksize = 16ull*1024ull);
		};
	}
}
#endif
