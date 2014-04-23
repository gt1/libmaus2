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
		/**
		 * class for synchronous buffered output
		 **/
		template<typename data_type>
                struct SynchronousGenericOutput
                {
                	//! this type
                        typedef SynchronousGenericOutput<data_type> this_type;
                        //! unique pointer type
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! output file stream type
			typedef ::libmaus::util::unique_ptr< std::ofstream >::type ofstream_ptr_type;
			//! file stream type
			typedef ::libmaus::util::unique_ptr< std::fstream  >::type  fstream_ptr_type;
			//! iterator type
			typedef PutOutputIterator<data_type,this_type> iterator_type;

			private:                
			//! default append is off
			static bool const append = false;
			
			//! output buffer
                        ::libmaus::autoarray::AutoArray<data_type> B;
                        //! output buffer start pointer
                        data_type * const pa;
                        //! output buffer current pointer
                        data_type * pc;
                        //! output buffer end pointer
                        data_type * const pe;
                        
                        //! output stream pointer (for truncating)
                        ofstream_ptr_type PW;
                        //! file stream pointer (for appending)
                         fstream_ptr_type PF;
                        //! ostream reference
                        std::ostream & W;
                        
                        //! number of elements written to file
                        uint64_t datawrittentofile;

                        /**
                         * write buffer to stream
                         **/
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

			SynchronousGenericOutput<data_type> & operator=(SynchronousGenericOutput<data_type> const &);
			SynchronousGenericOutput(SynchronousGenericOutput<data_type> const &);

                        public:
                        /**
                         * write the array A to the file outputfilename
                         *
                         * @param A array to be written
                         * @param outputfilename name of output file
                         **/
			template< ::libmaus::autoarray::alloc_type atype >
			static void writeArray(::libmaus::autoarray::AutoArray<data_type,atype> const & A, 
				std::string const & outputfilename)
			{
				this_type out(outputfilename,64*1024);
				
				for ( uint64_t i = 0; i < A.getN(); ++i )
					out.put(A[i]);
				
				out.flush();
			}

			/**
			 * constructor by file name
			 *
			 * @param filename name of output file
			 * @param bufsize size of output buffer
			 * @param truncate true if file should be truncated false data should be appended
			 * @param offset write offset in bytes
			 **/
                        SynchronousGenericOutput(std::string const & filename, uint64_t const bufsize, bool const truncate = true, uint64_t const offset = 0, bool const /* metasync */ = true)
                        : B(bufsize,false), pa(B.get()), pc(pa), pe(pa+B.getN()), 
                          PW ( truncate ? new std::ofstream(filename.c_str(),std::ios::binary|std::ios::out|std::ios::trunc) : 0),
                          PF ( truncate ? 0 : new std::fstream(filename.c_str(), std::ios::binary|std::ios::in|std::ios::out|std::ios::ate) ),
                          W  ( truncate ? (static_cast<std::ostream &>(*PW)) : (static_cast<std::ostream &>(*PF)) ),
                          datawrittentofile(0)
                        {
                        	W.seekp(offset,std::ios::beg);
                        }

                        /**
                         * constructor by output stream
                         *
                         * @param out output stream
                         * @param bufsize output buffer size
                         **/
                        SynchronousGenericOutput(std::ostream & out, uint64_t const bufsize)
                        : B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), 
                          W(out),
                          datawrittentofile(0)
                        {

                        }

                        /**
                         * flush the internal buffer but not the file stream
                         **/
                        void shallowFlush()
                        {
                                writeBuffer();
                        }

                        /**
                         * flush the buffer
                         **/
                        void flush()
                        {
                        	// empty buffer
                        	shallowFlush();
                        	// flush file stream
                                W.flush();

                                if ( ! W )
                                {
                                        ::libmaus::exception::LibMausException se;
                                        se.getStream() << "Failed to flush in SynchronousGenericOutput::flush()";
                                        se.finish();
                                        throw se;
                                }
                        }

                        /**
                         * put an element in the buffer
                         *
                         * @param c element to be put in buffer
                         **/
                        void put(data_type const c)
                        {
                                *(pc++) = c;
                                if ( pc == pe )
                                        writeBuffer();
                        }
                        
                        /**
                         * @return number of words written
                         **/
                        uint64_t getWrittenWords() const
                        {
                        	return datawrittentofile + (pc-pa);
                        }
                        
                        /**
                         * @return number of bytes written
                         **/
                        uint64_t getWrittenBytes() const
                        {
                        	return getWrittenWords() * sizeof(data_type);
                        }
                        
                        /**
                         * put an array of elements
                         *
                         * @param A sequence iterator
                         * @param n number of elements
                         **/
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
