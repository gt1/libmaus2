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
		template<typename _data_type>
		struct BufferedOutputBase
		{
			typedef _data_type data_type;
			typedef BufferedOutputBase<data_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			::libmaus::autoarray::AutoArray<data_type> B;
			data_type * const pa;
			data_type * pc;
			data_type * const pe;
			
			uint64_t totalwrittenbytes;
			uint64_t totalwrittenwords;
			
			BufferedOutputBase(uint64_t const bufsize)
			: B(bufsize), pa(B.begin()), pc(pa), pe(B.end()), totalwrittenbytes(0), totalwrittenwords(0)
			{
			
			}
			virtual ~BufferedOutputBase() {}

			void flush()
			{
				writeBuffer();
			}
			
			virtual void writeBuffer() = 0;

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
			
			void uncheckedPut(data_type const & v)
			{
				*(pc++) = v;
			}
			
			uint64_t spaceLeft() const
			{
				return pe-pc;
			}
			
			uint64_t getWrittenWords() const
			{
				return totalwrittenwords + (pc-pa);
			}
			
			uint64_t getWrittenBytes() const
			{
				return getWrittenWords() * sizeof(data_type);
			}
			
			void increaseWritten()
			{
				totalwrittenwords += (pc-pa);
				totalwrittenbytes += (pc-pa)*sizeof(data_type);
			}
			
			uint64_t fill()
			{
				return pc-pa;
			}
			
			void reset()
			{
				increaseWritten();
				pc = pa;
			}
		};
		template<typename _data_type>
		struct BufferedOutput : public BufferedOutputBase<_data_type>
		{
			typedef _data_type data_type;
			typedef BufferedOutput<data_type> this_type;
			typedef BufferedOutputBase<data_type> base_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			std::ostream & out;
			
			BufferedOutput(std::ostream & rout, uint64_t const bufsize)
			: BufferedOutputBase<_data_type>(bufsize), out(rout)
			{
			
			}
			~BufferedOutput()
			{
				base_type::flush();
			}

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

		template<typename _data_type>
		struct BufferedOutputNull : public BufferedOutputBase<_data_type>
		{
			typedef _data_type data_type;
			typedef BufferedOutputNull<data_type> this_type;
			typedef BufferedOutputBase<data_type> base_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			BufferedOutputNull(uint64_t const bufsize)
			: BufferedOutputBase<_data_type>(bufsize)
			{
			
			}
			~BufferedOutputNull()
			{
				base_type::flush();
			}
			virtual void writeBuffer()
			{
			}
		};
	}
}
#endif
