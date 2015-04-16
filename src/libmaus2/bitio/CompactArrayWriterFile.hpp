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
#if ! defined(LIBMAUS_BITIO_COMPACTARRAYWRITERFILE_HPP)
#define LIBMAUS_BITIO_COMPACTARRAYWRITERFILE_HPP

#include <libmaus/bitio/FastWriteBitWriter.hpp>

namespace libmaus
{
	namespace bitio
	{
		/**
		 * this class writes serialised CompactArray objects
		 * element by element, without the need to have the full object
		 * in memory. The resulting stream can be read back using
		 * the CompactDecoderWrapper class
		 **/
		struct CompactArrayWriterFile
		{
			::libmaus::aio::CheckedOutputStream::unique_ptr_type COS;
			::libmaus::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type SGO;
			::libmaus::bitio::FastWriteBitWriterBuffer64Sync::unique_ptr_type FWBW;
			
			uint64_t       n;
			uint64_t const b;
			
			void setup()
			{
				SGO->put(b);
				SGO->put(0); // n
				SGO->put(0); // (n*b+63)/64
				SGO->put(0); // (n*b+63)/64

				::libmaus::aio::SynchronousGenericOutput<uint64_t>::iterator_type it(*SGO);
				::libmaus::bitio::FastWriteBitWriterBuffer64Sync::unique_ptr_type tFWBW(
                                                new ::libmaus::bitio::FastWriteBitWriterBuffer64Sync(it)
                                        );
				FWBW = UNIQUE_PTR_MOVE(tFWBW);
			}
			
			CompactArrayWriterFile(std::string const & filename, uint64_t const rb)
			:
				COS(new ::libmaus::aio::CheckedOutputStream(filename)),
				SGO(new ::libmaus::aio::SynchronousGenericOutput<uint64_t>(*COS,8*1024)),
				n(0),
				b(rb)
			{
				setup();
			}
			~CompactArrayWriterFile()
			{
				flush();
			}
			
			void put(uint64_t const v)
			{
				FWBW->write(v,b);
				n += 1;
			}
			
			template<typename iterator>
			void write(iterator it, uint64_t len)
			{
				while ( len-- )
					put(*(it++));
			}
			
			void flush()
			{
				if ( FWBW )
				{
					FWBW->flush();
					FWBW.reset();
				}
				if ( SGO )
				{
					SGO->flush();
					SGO.reset();
				}
				if ( COS )
				{
					COS->flush();
					
					// construct header
					std::ostringstream ostr;
					::libmaus::aio::SynchronousGenericOutput<uint64_t> HSGO(ostr,8);
					HSGO.put(b);
					HSGO.put(n);
					HSGO.put((n*b+63)/64);
					HSGO.put((n*b+63)/64);
					HSGO.flush();
					std::string const header = ostr.str();
					assert ( header.size() == (sizeof(uint64_t)*4) );
					
					// write header
					COS->clear();
					COS->seekp(0);
					COS->write(header.c_str(),header.size());
					
					// close file
					COS->close();
					COS.reset();
				}
			}
		};
	}
}
#endif
