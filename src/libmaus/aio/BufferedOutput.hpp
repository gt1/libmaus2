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

#if ! defined(BUFFEREDOUTPUT_HPP)
#define BUFFEREDOUTPUT_HPP

#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace aio
	{
		/**
		 * base class for block buffer output
		 **/
		template<typename _data_type>
		struct BufferedOutputBase
		{
			typedef _data_type data_type;
			typedef BufferedOutputBase<data_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			//! block buffer
			::libmaus::autoarray::AutoArray<data_type> B;
			//! start of buffer pointer
			data_type * const pa;
			//! current pointer
			data_type * pc;
			//! end of buffer pointer
			data_type * const pe;
			
			//! total bytes written
			uint64_t totalwrittenbytes;
			//! total words written
			uint64_t totalwrittenwords;
			
			/**
			 * construct buffer of size bufsize
			 *
			 * @param bufsize buffer size (in elements)
			 **/
			BufferedOutputBase(uint64_t const bufsize)
			: B(bufsize), pa(B.begin()), pc(pa), pe(B.end()), totalwrittenbytes(0), totalwrittenwords(0)
			{
			
			}
			/**
			 * destructor
			 **/
			virtual ~BufferedOutputBase() {}

			/**
			 * flush buffer
			 **/
			void flush()
			{
				writeBuffer();
			}
			
			/**
			 * pure virtual buffer writing function
			 **/
			virtual void writeBuffer() = 0;

			/**
			 * put one element
			 *
			 * @param v element to be put in buffer
			 * @return true if buffer is full after putting the element
			 **/
			bool put(data_type const & v)
			{
				*(pc++) = v;
				if ( pc == pe )
				{
					flush();
					return true;
				}
				else
				{
					return false;
				}
			}
			
			/**
			 * put a list of elements
			 *
			 * @param A list start pointer
			 * @param n number of elements to be put in buffer
			 **/
			void put(data_type const * A, uint64_t n)
			{
				while ( n )
				{
					uint64_t const towrite = std::min(n,static_cast<uint64_t>(pe-pc));
					std::copy(A,A+towrite,pc);
					pc += towrite;
					A += towrite;
					n -= towrite;
					if ( pc == pe )
						flush();
				}
			}
			
			/**
			 * put an element without checking whether buffer fills up
			 *
			 * @param v element to be put in buffer
			 **/
			void uncheckedPut(data_type const & v)
			{
				*(pc++) = v;
			}
			
			/**
			 * @return space available in buffer (in elements)
			 **/
			uint64_t spaceLeft() const
			{
				return pe-pc;
			}
			
			/**
			 * @return number of words written (including those still in the buffer)
			 **/
			uint64_t getWrittenWords() const
			{
				return totalwrittenwords + (pc-pa);
			}
			
			/**
			 * @return number of bytes written (including those still in the buffer)
			 **/
			uint64_t getWrittenBytes() const
			{
				return getWrittenWords() * sizeof(data_type);
			}
			
			/**
			 * increase written accus by number of elements curently in buffer
			 **/
			void increaseWritten()
			{
				totalwrittenwords += (pc-pa);
				totalwrittenbytes += (pc-pa)*sizeof(data_type);
			}
			
			/**
			 * @return number of elements currently in buffer
			 **/
			uint64_t fill()
			{
				return pc-pa;
			}
			
			/**
			 * reset buffer
			 **/
			void reset()
			{
				increaseWritten();
				pc = pa;
			}
		};
		
		/**
		 * class for buffer file output
		 **/
		template<typename _data_type>
		struct BufferedOutput : public BufferedOutputBase<_data_type>
		{
			typedef _data_type data_type;
			typedef BufferedOutput<data_type> this_type;
			typedef BufferedOutputBase<data_type> base_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			std::ostream & out;
			
			public:
			/**
			 * constructor
			 *
			 * @param rout output stream
			 * @param bufsize size of buffer in elements
			 **/
			BufferedOutput(std::ostream & rout, uint64_t const bufsize)
			: BufferedOutputBase<_data_type>(bufsize), out(rout)
			{
			
			}
			/**
			 * destructor
			 **/
			~BufferedOutput()
			{
				base_type::flush();
			}

			/**
			 * write buffer to output stream
			 **/
			virtual void writeBuffer()
			{
				if ( base_type::fill() )
				{
					out.write ( reinterpret_cast<char const *>(base_type::pa), (base_type::pc-base_type::pa)*sizeof(data_type) );
					
					if ( ! out )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to write " << (base_type::pc-base_type::pa)*sizeof(data_type) << " bytes." << std::endl;
						se.finish();
						throw se;
					}
					
					base_type::reset();
				}
			}
		};

		/**
		 * null write output buffer. data put in buffer is written nowhere.
		 * this is useful for code directly using the buffer base.
		 **/
		template<typename _data_type>
		struct BufferedOutputNull : public BufferedOutputBase<_data_type>
		{
			typedef _data_type data_type;
			typedef BufferedOutputNull<data_type> this_type;
			typedef BufferedOutputBase<data_type> base_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			/**
			 * constructor
			 *
			 * @param bufsize size of buffer
			 **/
			BufferedOutputNull(uint64_t const bufsize)
			: BufferedOutputBase<_data_type>(bufsize)
			{
			
			}
			/**
			 * destructor
			 **/
			~BufferedOutputNull()
			{
				base_type::flush();
			}
			/**
			 * null writeBuffer function (does nothing)
			 **/
			virtual void writeBuffer()
			{
			}
		};
	}
}
#endif
