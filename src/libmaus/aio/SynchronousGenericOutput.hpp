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

#if ! defined(LIBMAUS_AIO_SYNCHRONOUSGENERICOUTPUT_HPP)
#define LIBMAUS_AIO_SYNCHRONOUSGENERICOUTPUT_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>
#include <libmaus/aio/PutOutputIterator.hpp>
#include <string>
#include <fstream>

namespace libmaus
{
	namespace aio
	{
		template<typename data_type>
                struct SynchronousGenericOutput
                {
                        typedef SynchronousGenericOutput<data_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::unique_ptr< std::ofstream >::type ofstream_ptr_type;
			typedef ::libmaus::util::unique_ptr< std::fstream  >::type  fstream_ptr_type;
			typedef PutOutputIterator<data_type,this_type> iterator_type;
                
                        ::libmaus::autoarray::AutoArray<data_type> B;
                        data_type * const pa;
                        data_type * pc;
                        data_type * const pe;
                        
                        ofstream_ptr_type PW;
                         fstream_ptr_type PF;
                        std::ostream & W;
                        
                        uint64_t datawrittentofile;

			template< ::libmaus::autoarray::alloc_type atype >
			static void writeArray(::libmaus::autoarray::AutoArray<data_type,atype> const & A, 
				std::string const & outputfilename)
			{
				this_type out(outputfilename,64*1024);
				
				for ( uint64_t i = 0; i < A.getN(); ++i )
					out.put(A[i]);
				
				out.flush();
			}

			static bool const append = false;

                        SynchronousGenericOutput(std::string const & filename, uint64_t const bufsize, bool const truncate = true, uint64_t const offset = 0, bool const /* metasync */ = true)
                        : B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), 
                          PW ( truncate ? new std::ofstream(filename.c_str(),std::ios::binary|std::ios::out|std::ios::trunc) : 0),
                          PF ( truncate ? 0 : new std::fstream(filename.c_str(), std::ios::binary|std::ios::in|std::ios::out|std::ios::ate) ),
                          W  ( truncate ? (static_cast<std::ostream &>(*PW)) : (static_cast<std::ostream &>(*PF)) ),
                          datawrittentofile(0)
                        {
                        	W.seekp(offset,std::ios::beg);
                        }

                        SynchronousGenericOutput(std::ostream & out, uint64_t const bufsize)
                        : B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), 
                          W(out)
                        {

                        }

                        void flush()
                        {
                                writeBuffer();
                                W.flush();

                                if ( ! W )
                                {
                                        ::libmaus::exception::LibMausException se;
                                        se.getStream() << "Failed to flush in SynchronousGenericOutput::flush()";
                                        se.finish();
                                        throw se;
                                }
                        }

                        void writeBuffer()
                        {
                                char const * ca = reinterpret_cast<char const *>(pa);
                                char const * cc = reinterpret_cast<char const *>(pc);
                                W.write ( ca, cc-ca );
                                
                                if ( ! W )
                                {
                                        ::libmaus::exception::LibMausException se;
                                        se.getStream() << "Failed to write in SynchronousGenericOutput::writeBuffer()";
                                        se.finish();
                                        throw se;
                                }

                               	datawrittentofile += (pc-pa);
                                pc = pa;
                        }

                        void put(uint64_t const c)
                        {
                                *(pc++) = c;
                                if ( pc == pe )
                                        writeBuffer();
                        }
                        
                        uint64_t getWrittenWords() const
                        {
                        	return datawrittentofile + (pc-pa);
                        }
                        
                        uint64_t getWrittenBytes() const
                        {
                        	return getWrittenWords() * sizeof(data_type);
                        }
                        
                        template<typename iterator>
                        void put(iterator A, uint64_t n)
                        {
                        	while ( n )
                        	{
	                        	uint64_t const space = pe-pc;
	                        	uint64_t const towrite = std::min(space,n);
	                        	std::copy ( A, A + towrite, pc );
					A += towrite;
					pc += towrite;
					n -= towrite;
					
					if ( pc == pe )
						writeBuffer();
        			}
                        }
                };
	}
}
#endif
