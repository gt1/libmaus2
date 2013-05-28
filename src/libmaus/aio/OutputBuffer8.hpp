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


#if ! defined(LIBMAUS_AIO_OUTPUTBUFFER8_HPP)
#define LIBMAUS_AIO_OUTPUTBUFFER8_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/aio/AsynchronousWriter.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <string>

namespace libmaus
{
	namespace aio
	{
                struct OutputBuffer8
                {
                        typedef OutputBuffer8 this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
                
                        ::libmaus::autoarray::AutoArray<uint64_t> B;
                        uint64_t * const pa;
                        uint64_t * pc;
                        uint64_t * const pe;
                        ::libmaus::aio::AsynchronousWriter W;

			static void writeArray(::libmaus::autoarray::AutoArray<uint64_t> const & A, 
				std::string const & outputfilename)
			{
				::libmaus::aio::OutputBuffer8 out(outputfilename,64*1024);
				
				for ( uint64_t i = 0; i < A.getN(); ++i )
					out.put(A[i]);
				
				out.flush();
			}


                        OutputBuffer8(std::string const & filename, uint64_t const bufsize)
                        : B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), W(filename,16)
                        {

                        }

                        void flush()
                        {
                                writeBuffer();
                                W.flush();
                        }

                        void writeBuffer()
                        {
                                W.write ( 
                                        reinterpret_cast<char const *>(pa),
                                        reinterpret_cast<char const *>(pc));
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
