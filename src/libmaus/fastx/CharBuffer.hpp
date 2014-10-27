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

#if ! defined(CHARBUFFER_HPP)
#define CHARBUFFER_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/fastx/acgtnMap.hpp>
#include <algorithm>

namespace libmaus
{
	namespace fastx
	{
		template<typename _value_type, ::libmaus::autoarray::alloc_type _atype>
                struct EntityBuffer
                {
                	typedef _value_type value_type;
                	static const ::libmaus::autoarray::alloc_type atype = _atype;
                	typedef EntityBuffer<value_type,_atype> this_type;
                	
                	private:
                	this_type operator=(this_type const &);
                	EntityBuffer(this_type const &);
                
                	public:
                        uint64_t buffersize;
                        uint64_t length;
			::libmaus::autoarray::AutoArray<value_type,atype> abuffer;
			value_type * buffer;
			
			uint64_t swapBuffer(::libmaus::autoarray::AutoArray<value_type,atype> & obuffer)
			{
				uint64_t const rlength = length;

				abuffer.swap(obuffer);
				length = 0;
				buffersize = abuffer.size();
				buffer = abuffer.begin();
				
				return rlength;
			}

                        void bufferPush(value_type c)
                        {
                                if ( length == buffersize )
                                        expandBuffer();
                                buffer[length++] = c;
                        }
                        
                        void put(value_type c)
                        {
                        	bufferPush(c);
                        }
                        
                        void expandBuffer()
                        {
                                uint64_t newbuffersize = std::max(2*buffersize,static_cast<uint64_t>(1u));
                                ::libmaus::autoarray::AutoArray<value_type,atype> newabuffer(newbuffersize);
                        
                                std::copy ( abuffer.get(), abuffer.get()+buffersize, newabuffer.get() );
                                
                                buffersize = newbuffersize;
                                abuffer = newabuffer;
                                buffer = abuffer.get();
                                
                                // std::cerr << "expanded buffer size to " << buffersize << std::endl;
                        }
                        
                        EntityBuffer(
                        	::libmaus::autoarray::AutoArray<value_type,atype> & rbuffer,
                        	uint64_t const blocksize)
			: buffersize(rbuffer.size()), length(blocksize), abuffer(rbuffer), buffer(abuffer.get())
			{
			
			}

                        EntityBuffer(uint64_t const initialsize = 128)
                        : buffersize(initialsize), length(0), abuffer(buffersize), buffer(abuffer.get())
                        {
                        
                        }
                        
                        void assign(std::string & s)
                        {
                        	s.assign ( buffer, buffer + length );
			}
			void reset()
			{
				length = 0;
			}
			
			value_type const * begin() const
			{
				return buffer;
			}
			
			value_type const * end() const
			{
				return buffer + length;
			}
			
			void reverseComplementUnmapped()
			{
				std::reverse(buffer,buffer+length);
				for ( uint64_t i = 0; i < length; ++i )
					buffer[i] = ::libmaus::fastx::invertUnmapped(buffer[i]);
			}
                };
                
                typedef EntityBuffer<char,::libmaus::autoarray::alloc_type_cxx> CharBuffer;
                typedef EntityBuffer<uint8_t,::libmaus::autoarray::alloc_type_cxx> UCharBuffer;
                typedef EntityBuffer<uint8_t,::libmaus::autoarray::alloc_type_memalign_cacheline> UCharBufferC;
	}
}
#endif
