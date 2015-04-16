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

#if ! defined(OUTPUTBUFFER_HPP)
#define OUTPUTBUFFER_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <fstream>
#include <cassert>
#include <iostream>

namespace libmaus2
{
        namespace bitio
        {
                template<typename _data_type>
                struct OutputBuffer
                {
                        typedef _data_type data_type;
                        
                        unsigned int const n;
                        ::libmaus2::autoarray::AutoArray<data_type> B;
                        unsigned int f;
                        std::ostream & out;
                        
                        OutputBuffer(unsigned int const rn, std::ostream & rout) : n(rn), B(n,false), f(0), out(rout) {}
                        ~OutputBuffer() { flush(); }
                        
                        void writeContents()
                        {
                                // std::cerr << "writing buffer of " << f << " words." << std::endl;
                                out.write( reinterpret_cast<char const *>(B.get()), f * sizeof(data_type) );
                                f = 0;
                        }
                        
                        void flush()
                        {
                                writeContents();
                                out.flush();
                        }
                        
                        void writeData(data_type D)
                        {
                                // std::cerr << "Writing " << D << std::endl;
                                
                                B[f++] = D;
                                
                                if ( f == n )
                                        writeContents();
                        }
                };
                
                template<typename _data_type>
                struct OutputFile : public std::ofstream, OutputBuffer<_data_type>
                {
                        OutputFile(unsigned int const rn, std::string const & filename)
                        : std::ofstream(filename.c_str(),std::ios::binary), OutputBuffer<_data_type>(rn,*this) {}
                        ~OutputFile() {}
                        
                        void flush()
                        {
                                OutputBuffer<_data_type>::flush();
                        }
                };
                
                template<typename _data_type>
                struct OutputBufferIteratorProxy
                {
                        typedef _data_type data_type;
                
                        OutputBuffer<_data_type> & buffer;
                        
                        OutputBufferIteratorProxy(OutputBuffer<_data_type> & rbuffer) : buffer(rbuffer) {}
                        
                         OutputBufferIteratorProxy<data_type> & operator=(data_type D)
                         {
                                buffer.writeData(D);
                                return *this;
                         }
                };

                template<typename _data_type>
                struct OutputBufferIterator
                {
                        typedef _data_type data_type;	
                        OutputBufferIteratorProxy<data_type> proxy;
                        
                        OutputBufferIterator(OutputBuffer<data_type> & rbuffer) : proxy(rbuffer) {}
                        ~OutputBufferIterator() {}
                        
                        OutputBufferIteratorProxy<data_type> & operator*()
                        {
                                return proxy;
                        }
                        OutputBufferIterator<data_type> & operator++(int)
                        {
                                return *this;
                        }
                };

                template<typename _data_type>
                struct InputBuffer
                {
                        typedef _data_type data_type;
                        
                        unsigned int const n;
                        ::libmaus2::autoarray::AutoArray<data_type> B;
                        unsigned int c;
                        unsigned int f;
                        std::istream & in;
                        
                        InputBuffer(unsigned int const rn, std::istream & rin) : n(rn), B(n,false), f(0), c(0), in(rin) {}
                        ~InputBuffer() { }
                        
                        void fillBuffer()
                        {
                                in.read( reinterpret_cast<char *>(B.get()), n * sizeof(data_type) );
                                assert ( in.gcount() % sizeof(data_type) == 0 );
                                c = 0;
                                f = in.gcount() / sizeof(data_type);
                        }
                        
                        data_type readData()
                        {
                                if ( c == f )
                                        fillBuffer();
                                return B[c++];
                        }
                };
        }
}
#endif
