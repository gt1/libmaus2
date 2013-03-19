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


#if ! defined(LIBMAUS_AIO_SYNCHRONOUSOUTPUTBUFFER8_HPP)
#define LIBMAUS_AIO_SYNCHRONOUSOUTPUTBUFFER8_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/aio/AsynchronousWriter.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <string>

namespace libmaus
{
	namespace aio
	{
                struct SynchronousOutputBuffer8
                {
                        typedef SynchronousOutputBuffer8 this_type;
                        typedef ::libmaus::util::unique_ptr < this_type > :: type unique_ptr_type;
                
			std::string const filename;
                        ::libmaus::autoarray::AutoArray<uint64_t> B;
                        uint64_t * const pa;
                        uint64_t * pc;
                        uint64_t * const pe;

                        SynchronousOutputBuffer8(std::string const & rfilename, uint64_t const bufsize, bool truncate = true)
                        : filename(rfilename), B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN())
                        {
                                if ( truncate )
                                {
        				std::ofstream ostr(filename.c_str(), std::ios::binary);
	        			ostr.flush();
                                }

                        }
			~SynchronousOutputBuffer8()
			{
				flush();
			}

                        void flush()
                        {
                                writeBuffer();
                        }

                        void writeBuffer()
                        {
				std::ofstream ostr(filename.c_str(), std::ios::binary | std::ios::app);
                                ostr.write ( 
                                        reinterpret_cast<char const *>(pa),
                                        reinterpret_cast<char const *>(pc)-reinterpret_cast<char const *>(pa)
				);
				ostr.flush();
				
				if ( ! ostr )
				{
				        ::libmaus::exception::LibMausException se;
				        se.getStream() << "Failed to flush buffer in SynchronousOutputBuffer8::writeBuffer()";
				        se.finish();
				        throw se;
				}

				ostr.close();
                                pc = pa;
                        }

                        void put(uint64_t const c)
                        {
                                *(pc++) = c;
                                if ( pc == pe )
                                        writeBuffer();
                        }
                };
	}
}
#endif

