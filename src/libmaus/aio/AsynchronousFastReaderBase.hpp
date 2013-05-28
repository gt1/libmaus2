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

#if ! defined(ASYNCHRONOUSFASTREADERBASE_HPP)
#define ASYNCHRONOUSFASTREADERBASE_HPP

#include <libmaus/aio/AsynchronousBufferReader.hpp>

namespace libmaus
{
	namespace aio
	{
		struct AsynchronousFastReaderBase
		{
			private:
			std::vector<std::string> const filenames;
			::libmaus::aio::AsynchronousBufferReaderList reader;
			std::pair < char const *, ssize_t > data;
			u_int8_t const * text;
			u_int64_t textlength;
			u_int64_t dataptr;
			bool firstbuf;
			uint64_t c;

			static std::vector<std::string> singleToList(std::string const & filename)
			{
				std::vector<std::string> filenames;
				filenames.push_back(filename);
				return filenames;
			}

			uint64_t readNumber1()
			{
				int const v = getNextCharacter();
				if ( v < 0 )
				{
					::libmaus::exception::LibMausException ex;
					ex.getStream() << "Failed to read number in ::libmaus::aio::AsynchronousFastReaderBase::readNumber1().";
					ex.finish();
					throw ex;
				}
				return v;
			}
			uint64_t readNumber2()
			{
				uint64_t const v0 = readNumber1();
				uint64_t const v1 = readNumber1();
				return (v0<<8)|v1;
			}
			uint64_t readNumber4()
			{
				uint64_t const v0 = readNumber2();
				uint64_t const v1 = readNumber2();
				return (v0<<16)|v1;
			}
			uint64_t readNumber8()
			{
				uint64_t const v0 = readNumber4();
				uint64_t const v1 = readNumber4();
				return (v0<<32)|v1;
			}

			public:
			AsynchronousFastReaderBase(
				std::string const & rfilename, 
				unsigned int numbuffers = 16, 
				unsigned int bufsize = 1024, 
				uint64_t const offset = 0)
			: 
				filenames(singleToList(rfilename)),
				reader(filenames.begin(),filenames.end(),numbuffers,bufsize,offset), 
				data(reinterpret_cast<char const *>(0),0), 
				text(0), 
				textlength(0), 
				dataptr(0), 
				firstbuf(true), 
				c(0)
			{
			}

			AsynchronousFastReaderBase(
				std::vector<std::string> const & rfilenames,
				unsigned int numbuffers = 16, 
				unsigned int bufsize = 1024, 
				uint64_t const offset = 0)
			: 
				filenames(rfilenames),
				reader(filenames.begin(),filenames.end(),numbuffers,bufsize,offset), 
				data(reinterpret_cast<char const *>(0),0), 
				text(0), 
				textlength(0), 
				dataptr(0), 
				firstbuf(true), 
				c(0)
			{
			}

			uint64_t getC() const
			{
				return c;
			}
			
			int get()
			{
				return getNextCharacter();
			}

			int getNextCharacter()
			{
				if ( dataptr == textlength )
				{
					if ( !firstbuf )
						reader.returnBuffer();
					firstbuf = false;
					bool const readok = reader.getBuffer(data);
					if ( ! readok )
						return -1;
					text = reinterpret_cast<u_int8_t const *>(data.first);
					textlength = data.second;
					dataptr = 0;
				}
				
				c += 1;
				return text[dataptr++];
			}
		};
	}
}
#endif
