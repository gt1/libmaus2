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

#if ! defined(VARLIST_HPP)
#define VARLIST_HPP

#include <cassert>
#include <list>
#include <ostream>

#include <libmaus2/types/types.hpp>

namespace libmaus2
{
	namespace util
	{
		template<typename N>
		struct VarList
		{
			std::list< N > B;
			
			VarList()
			{
			}
			
			uint64_t rank(uint64_t key, uint64_t pos) const
			{
				typename std::list<N>::const_iterator I = B.begin();
				
				uint64_t pc = 0;
				for ( uint64_t i = 0; i <= pos; ++i )
				{
					if ( *I == key )
						pc++;
					I++;
				}
				
				return pc;
			}

			uint64_t select(uint64_t key, uint64_t rank) const
			{
				std::list<bool>::const_iterator I = B.begin();
				
				uint64_t pos = 0;
				
				while ( (*I) != key )
				{
					pos++;
					I++;
				}
					
				while ( rank )
				{
					pos++;
					I++;

					while ( (*I) != key )
					{
						pos++;
						I++;
					}
					
					rank--;
				}
				
				return pos;
			}
			
			void insert(uint64_t pos, N b)
			{
				assert ( pos <= B.size() );
			
				typename std::list<N>::iterator I = B.begin();
				
				for ( uint64_t i = 0; i < pos; ++i )
					I++;
					
				B.insert(I,b);
			}
			void remove(uint64_t pos)
			{
				assert ( pos < B.size() );
			
				typename std::list<N>::iterator I = B.begin();
				
				for ( uint64_t i = 0; i < pos; ++i )
					I++;

				B.erase(I);	
			}
			void set(uint64_t pos, N b)
			{
				assert ( pos < B.size() );
			
				typename std::list<N>::iterator I = B.begin();
				
				for ( uint64_t i = 0; i < pos; ++i )
					I++;

				*I = b;	
			}
			N get(uint64_t pos)
			{
				assert ( pos < B.size() );
			
				typename std::list<N>::iterator I = B.begin();
				
				for ( uint64_t i = 0; i < pos; ++i )
					I++;

				return *I;
			}
		};

		template<typename T>
		inline std::ostream & operator<<(std::ostream & out, VarList<T> const & B)
		{
			for ( typename std::list< T >::const_iterator I = B.B.begin(); I != B.B.end(); ++I )
				out << (*I);
			return out;
		}
	}
}
#endif
