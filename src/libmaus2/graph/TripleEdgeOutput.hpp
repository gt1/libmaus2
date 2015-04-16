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

#if ! defined(TRIPLEEDGEOUTPUT_HPP)
#define TRIPLEEDGEOUTPUT_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/graph/TripleEdge.hpp>
#include <fstream>

namespace libmaus
{
	namespace graph
	{
		struct TripleEdgeOutput
		{
			typedef TripleEdgeOutput this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			typedef ::libmaus::autoarray::AutoArray<TripleEdge> buffer_type;
			buffer_type B;
			TripleEdge * const pa;
			TripleEdge * pc;
			TripleEdge * const pe;

			std::string const filename;

			void writeOut()
			{
				if ( pc != pa )
				{
					std::ofstream ostr(filename.c_str(), std::ios::binary | std::ios::app );
					ostr.write ( 
						reinterpret_cast<char const *>(pa), 
						reinterpret_cast<char const *>(pc) - reinterpret_cast<char const *>(pa)
					);
					ostr.flush();
					ostr.close();
					pc = pa;
				}
			}

			void sortBuffer()
			{
				if ( pc != pa )
					std::sort ( pa, pc );
			}
			
			void unify()
			{
				#if 0
				uint64_t const uin = pc-pa;
				#endif
			
				sortBuffer();

				TripleEdge prevtrip;
				TripleEdge * po = pa;

				for ( TripleEdge const * pp = pa; pp != pc; ++pp )
				{
					TripleEdge const & T = *pp;
									
					if ( (T.a != prevtrip.a) || (T.b != prevtrip.b) )
					{
						if ( prevtrip.a != prevtrip.b )
							*(po++) = prevtrip;

						prevtrip = T;
					}
					else
					{
						prevtrip.c += T.c;
					}
				}
				if ( prevtrip.a != prevtrip.b )
					*(po++) = prevtrip;
					
				pc = po;
				
				#if 0
				uint64_t const uout = pc-pa;
				
				std::cerr << "uin=" << uin << " uout=" << uout << std::endl;			
				#endif
			}

			static uint64_t byteSize(uint64_t const bufsize)
			{
				return buffer_type::byteSize(bufsize) + 3 * sizeof(uint64_t*);
			}

			TripleEdgeOutput(std::string const & rfilename, uint64_t const bufsize)
			: B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), filename(rfilename)
			{
				std::ofstream ostr(filename.c_str(), std::ios::binary);
				ostr.flush();
				ostr.close();
			}
			~TripleEdgeOutput()
			{
				flush();
			}
			
			void flush()
			{
				writeOut();
			}
			void flushUnified()
			{
				unify();
				writeOut();
			}

			
			bool write(TripleEdge const & T)
			{
				(*pc++) = T;
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
			bool writeUnified(TripleEdge const & T)
			{
				(*pc++) = T;
				if ( pc == pe )
				{
					flushUnified();
					return true;
				}
				else
				{
					return false;
				}
			}
		};
	}
}
#endif
