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
#if ! defined(LIBMAUS_LZ_ISTREAMSOURCE_HPP)
#define LIBMAUS_LZ_ISTREAMSOURCE_HPP

#include <libmaus/LibMausConfig.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <istream>

#if defined(LIBMAUS_HAVE_SNAPPY)
#include <snappy-sinksource.h>
#include <snappy.h>
#define IstreamSourceBaseType ::snappy::Source
#else
namespace libmaus
{
	namespace lz
	{
		struct IstreamSourceBase
		{
			virtual ~IstreamSourceBase() {}
			virtual size_t Available() const = 0;
			virtual char const * Peek(size_t * len) = 0;
			virtual void Skip(size_t rn) = 0;
		};
	}
}
#define IstreamSourceBaseType ::libmaus::lz::IstreamSourceBase
#endif

namespace libmaus
{
	namespace lz
	{
		template<typename in_type>
		struct IstreamSource : public IstreamSourceBaseType
		{
			in_type & in;
			uint64_t avail;
			::libmaus::autoarray::AutoArray<char> A;
			char * const pa;
			char * pc;
			char * pe;

			void fillBuffer()
			{
				assert ( pc == pe );
				pc = pa;
				pe = pa + in.read(pc,std::min(avail,A.size()));
			}

			IstreamSource(
				in_type & rin, 
				uint64_t const ravail,
				uint64_t const bufsize = 64*1024
			) : in(rin), avail(ravail), A(bufsize,false), pa(A.begin()), pc(pa), pe(pc) 
			{
				fillBuffer();	
			}
			
			size_t Available() const
			{
				return avail;
			}
			
			char const * Peek(size_t * len)
			{
				*len = (pe-pc);
				return pc;
			}
			
			int get()
			{
				size_t av = 0;
				char const * c = Peek(&av);
				if ( ! av )
					return -1;
				else
				{
					uint8_t const * u = reinterpret_cast<uint8_t const *>(c);
					uint8_t const d = *u;
					Skip(1);
					return d;
				}
			}
			
			void Skip(size_t rn)
			{
				uint64_t n = std::min(static_cast<uint64_t>(rn),avail);
			
				while ( n )
				{
					// bytes available in buffer
					uint64_t const bavail = pe-pc; assert ( bavail );
					// bytes to skip
					uint64_t const lskip = std::min(n,bavail);

					pc += lskip;
					avail -= lskip;
					n -= lskip;
					
					if ( pc == pe )
						fillBuffer();
				} 

				// reached EOF or buffer is non empty
				assert ( (!avail) || (pe != pc) );
			}
		};

	}
}

#undef IstreamSourceBaseType

#endif
