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
#if ! defined(LIBMAUS2_FASTX_COMPACTFASTQCONTAINERDICTIONARYCREATOR_HPP)
#define LIBMAUS2_FASTX_COMPACTFASTQCONTAINERDICTIONARYCREATOR_HPP

#include <libmaus2/rank/ImpCacheLineRank.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct CompactFastQContainerDictionaryCreator
		{
			enum codelenrun_type { codelenrun_first, codelenrun_second };
			enum { shortthres = (1ull << (sizeof(uint16_t)*8)) };

			uint64_t numlong;
			uint64_t numshort;
			uint64_t longptr;
			uint64_t shortptr;
			uint64_t longidx;
			uint64_t shortidx;
			
			::libmaus2::rank::ImpCacheLineRank::unique_ptr_type designators;
			::libmaus2::rank::ImpCacheLineRank::WriteContext desigwc;
			::libmaus2::autoarray::AutoArray<uint64_t> longptrs;
			::libmaus2::autoarray::AutoArray<uint16_t> shortptrs;
			
			uint64_t byteSize() const
			{
				uint64_t s = 0;
				if ( designators )
					s += designators->byteSize();
				s += longptrs.byteSize();
				s += shortptrs.byteSize();
				return s;
			}
			
			void reset()
			{
				longptr = 0;
				shortptr = shortthres;
			}

			CompactFastQContainerDictionaryCreator()
			: numlong(0), numshort(0), longidx(0), shortidx(0)
			{
				reset();	
			}
			
			void shiftLongPointers(uint64_t const offset)
			{
				for ( uint64_t i = 0; i < longptrs.size(); ++i )
					longptrs[i] += offset;
			}
			
			
			void setupDataStructures()
			{
				reset();
				
				designators = UNIQUE_PTR_MOVE(::libmaus2::rank::ImpCacheLineRank::unique_ptr_type(
					new ::libmaus2::rank::ImpCacheLineRank(numshort)
				));
				desigwc = designators->getWriteContext();
				
				longptrs = ::libmaus2::autoarray::AutoArray<uint64_t>(numlong);
				shortptrs = ::libmaus2::autoarray::AutoArray<uint16_t>(numshort);
			}
			
			void serialise(std::ostream & out)
			{
				assert ( designators );
				designators->serialise(out);
				longptrs.serialize(out);
				shortptrs.serialize(out);
			}
				
			void flush()
			{
				desigwc.flush();
			
				#if 0	
				std::cerr << "longidx=" << longidx << " numlong=" << numlong << std::endl;
				std::cerr << "shortidx=" << shortidx << " numshort=" << numshort << std::endl;
				#endif
				
				assert ( longidx == numlong );
				assert ( shortidx == numshort );
			}

			void operator()(CompactFastQContainerDictionaryCreator::codelenrun_type const rt, uint64_t const codelen)
			{
				if ( rt == codelenrun_first )
				{
					if ( shortptr >= shortthres )
					{
						numlong++;
						shortptr = 0;
					}
					
					longptr += codelen;
					shortptr += codelen;				
					numshort++;			
				}
				else
				{
					if ( shortptr >= shortthres )
					{
						assert ( longidx < longptrs.size() );
						desigwc.writeBit(longidx != 0);
						longptrs[longidx++] = longptr;
						shortptr = 0;
					}
					else
					{
						desigwc.writeBit(0);
					}
					
					assert ( shortidx < shortptrs.size() );
					shortptrs[shortidx++] = shortptr;
					
					longptr += codelen;
					shortptr += codelen;				
				}
			}
		};
	}
}
#endif
