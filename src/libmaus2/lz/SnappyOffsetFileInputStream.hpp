/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_LZ_SNAPPYOFFSETFILEINPUTSTREAM_HPP)
#define LIBMAUS2_LZ_SNAPPYOFFSETFILEINPUTSTREAM_HPP

#include <libmaus2/lz/SnappyInputStream.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct SnappyOffsetFileInputStream
		{
			typedef SnappyOffsetFileInputStream this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			typedef libmaus2::aio::InputStream stream_type;
			typedef stream_type::unique_ptr_type stream_ptr_type;
		
			stream_ptr_type Pistr;
			stream_type & istr;
			SnappyInputStream instream;
			
			stream_ptr_type openFileAtOffset(std::string const & filename, uint64_t const offset)
			{
				stream_ptr_type Pistr(libmaus2::aio::InputStreamFactoryContainer::constructUnique(filename));
				Pistr->seekg(offset,std::ios::beg);
				
				return UNIQUE_PTR_MOVE(Pistr);
			}
			
			SnappyOffsetFileInputStream(std::string const & filename, uint64_t const roffset)
			: Pistr(openFileAtOffset(filename,roffset)), 
			  istr(*Pistr), instream(istr,roffset,true) {}
			SnappyOffsetFileInputStream(stream_type & ristr, uint64_t const roffset)
			: Pistr(), 
			  istr(ristr), instream(istr,roffset,true) {}
			int get() { return instream.get(); }
			int peek() { return instream.peek(); }
			uint64_t read(char * c, uint64_t const n) { return instream.read(c,n); }
			uint64_t gcount() const { return instream.gcount(); }
		};

	}
}
#endif
