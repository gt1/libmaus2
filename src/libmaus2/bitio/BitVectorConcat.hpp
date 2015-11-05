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

#if ! defined(BITVECTORCONCAT_HPP)
#define BITVECTORCONCAT_HPP

#include <libmaus2/serialize/Serialize.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/bitio/OutputBuffer.hpp>
#include <libmaus2/bitio/FastWriteBitWriter.hpp>

namespace libmaus2
{
	namespace bitio
	{
		struct BitVectorConcat
		{
			static uint64_t const bufsize = 64*1024;

			static uint64_t concatenateBitVectors(
				std::vector< std::pair<std::string,uint64_t> > const & filenames,
				std::ostream & out,
				uint64_t const outmod = 1
			)
			{
				uint64_t tbits = 0;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					tbits += filenames[i].second;
				// number of output words
				uint64_t n = (tbits + 63)/64;

				uint64_t padwords = 0;
				while ( (n+padwords) % outmod )
					padwords++;

				// std::cerr << "bits=" << tbits << " n=" << n << " padwords=" << padwords << std::endl;

				// number of code words
				::libmaus2::serialize::Serialize<uint64_t>::serialize(out,n+padwords);

				::libmaus2::bitio::OutputBuffer<uint64_t> OB(bufsize,out);
				::libmaus2::bitio::OutputBufferIterator<uint64_t> outputiterator(OB);
				::libmaus2::bitio::FastWriteBitWriterBuffer64 writer(outputiterator);

				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					// std::cerr << filenames[i].first << "\t" << filenames[i].second << std::endl;

					::libmaus2::aio::SynchronousGenericInput<uint64_t> in(filenames[i].first,bufsize);
					uint64_t const ibits = filenames[i].second;
					uint64_t const fwords = ibits / (8*sizeof(uint64_t));
					uint64_t const rbits = ibits - fwords * 8*sizeof(uint64_t);

					#if 0
					std::cerr << "ibits=" << ibits << " fwords=" << fwords << " rbits=" << rbits << std::endl;
					#endif

					for ( uint64_t j = 0; j < fwords; ++j )
					{
						uint64_t w;
						bool const ok = in.getNext(w);
						assert ( ok );
						writer.write(w,sizeof(uint64_t)*8);
					}

					if ( rbits )
					{
						uint64_t w;
						bool const ok = in.getNext(w);
						assert ( ok );
						unsigned int const shift = 8*sizeof(uint64_t)-rbits;
						// std::cerr << "shift=" << shift << std::endl;
						writer.write ( w >> shift, rbits );
					}
				}

				writer.flush();

				for ( uint64_t i = 0; i < padwords; ++i )
					for ( uint64_t j = 0; j < 64; ++j )
						writer.writeBit(0);

				writer.flush();

				OB.flush();
				out.flush();

				return n + padwords;
			}
		};
	}
}
#endif
