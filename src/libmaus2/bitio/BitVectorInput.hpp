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

			uint64_t bitsleft;
			int shift;
			uint64_t v;
			
			static uint64_t getLength(std::string const & fn)
			{
				libmaus::aio::CheckedInputStream CIS(fn);
				CIS.seekg(-8,std::ios::end);
				libmaus::aio::SynchronousGenericInput<uint64_t> SGI(CIS,1);
				uint64_t n = 0;
				bool const ok = SGI.getNext(n);
				assert ( ok );
				return n;
			}
			
			static uint64_t getLength(std::vector<std::string> const & V)
			{
				uint64_t n = 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
					n += getLength(V[i]);
				return n;
			}
			
			static std::pair<uint64_t,uint64_t> getOffset(std::vector<std::string> const & V, uint64_t offset)
			{
				uint64_t f = 0;
				
				while ( f < V.size() )
				{
					uint64_t const n = getLength(V[f]);
					if ( offset < n )
						return std::pair<uint64_t,uint64_t>(f,offset);
					else
					{
						offset -= n;
						f++;
					}
				}
				
				return std::pair<uint64_t,uint64_t>(f,0);
			}
			
			BitVectorInput(std::vector<std::string> const & rfilenames)
			: filenames(rfilenames), nextfile(0), bitsleft(0), shift(-1), v(0)
			{
			
			}

			BitVectorInput(std::vector<std::string> const & rfilenames, uint64_t const offset)
			: filenames(rfilenames), nextfile(0), bitsleft(0), shift(-1), v(0)
			{
				std::pair<uint64_t,uint64_t> O = getOffset(filenames,offset);
				nextfile = O.first;
				
				if ( nextfile < filenames.size() )
				{
					std::string const fn = filenames[nextfile++];

					libmaus::aio::CheckedInputStream::unique_ptr_type tistr(
						new libmaus::aio::CheckedInputStream(fn)
					);
					istr = UNIQUE_PTR_MOVE(tistr);
							
					istr->seekg(-8,std::ios::end);
							
					libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type nSGI(
						new libmaus::aio::SynchronousGenericInput<uint64_t>(*istr,1)
					);
					
					bool const ok = nSGI->getNext(bitsleft);
					assert ( ok );
					
					// number of complete words to skip
					uint64_t wordskip = O.second / 64;

					istr->clear();
					istr->seekg(8*wordskip,std::ios::beg);
					
					bitsleft -= 64*wordskip;
					O.second -= 64*wordskip;

					libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type tSGI(
						new libmaus::aio::SynchronousGenericInput<uint64_t>(*istr,8*1024)
					);
					SGI = UNIQUE_PTR_MOVE(tSGI);
					
					// skip over rest of offset by reading the bits
					while ( O.second )
					{
						readBit();
						--O.second;
					}
				}
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
