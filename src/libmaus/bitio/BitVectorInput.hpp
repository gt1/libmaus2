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
#if ! defined(LIBMAUS_BITIO_BITVECTORINPUT_HPP)
#define LIBMAUS_BITIO_BITVECTORINPUT_HPP

#include <libmaus/aio/SynchronousGenericInput.hpp>

namespace libmaus
{
	namespace bitio
	{
		struct BitVectorInput
		{
			libmaus::aio::CheckedInputStream::unique_ptr_type istr;
			libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type SGI;
			
			std::vector<std::string> filenames;
			uint64_t nextfile;

			uint64_t bitsleft = 0;
			int shift = -1;
			uint64_t v = 0;
			
			BitVectorInput(std::vector<std::string> const & rfilenames)
			: filenames(rfilenames), nextfile(0), bitsleft(0), shift(-1), v(0)
			{
			
			}

			bool readBit()
			{
				if ( shift < 0 )
				{
					while ( ! bitsleft )
					{
						if ( nextfile < filenames.size() )
						{
							SGI.reset();
							istr.reset();

							std::string const fn = filenames[nextfile++];
							
							libmaus::aio::CheckedInputStream::unique_ptr_type tistr(
								new libmaus::aio::CheckedInputStream(fn)
							);
							istr = UNIQUE_PTR_MOVE(tistr);
							
							istr->seekg(-8,std::ios::end);
							
							libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type nSGI(
								new libmaus::aio::SynchronousGenericInput<uint64_t>(*istr,1)
							);
							
							int64_t const numbits = nSGI->get();
							assert ( numbits >= 0 );
							bitsleft = numbits;
							
							istr->clear();
							istr->seekg(0,std::ios::beg);

							libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type tSGI(
								new libmaus::aio::SynchronousGenericInput<uint64_t>(*istr,8*1024)
							);
							SGI = UNIQUE_PTR_MOVE(tSGI);
						}
						else
						{
							libmaus::exception::LibMausException se;
							se.getStream() << "BitVectorInput::readBit(): EOF" << std::endl;
							se.finish();
							throw se;
						}
					}
					
					assert ( bitsleft );
				
					int const bitsinword = static_cast<int>(std::min(bitsleft,static_cast<uint64_t>(64)));
					assert ( bitsinword );
					shift = static_cast<int>(bitsinword)-1;
					bitsleft -= bitsinword;
								
					bool const ok = SGI->getNext(v);
					assert ( ok );
				}

				return (v >> (shift--)) & 1ull;
			}
		};
	}
}
#endif
