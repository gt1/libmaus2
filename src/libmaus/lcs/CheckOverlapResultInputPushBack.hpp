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
#if ! defined(CHECKOVERLAPRESULTINPUTPUSHBACK_HPP)
#define CHECKOVERLAPRESULTINPUTPUSHBACK_HPP

#include <libmaus/lcs/CheckOverlapResult.hpp>
#include <libmaus/lcs/CheckOverlapResultInput.hpp>
#include <stack>

namespace libmaus
{
	namespace lcs
	{
		struct CheckOverlapResultInputPushBack
		{
			typedef CheckOverlapResultInputPushBack this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			CheckOverlapResultInput CORI;
			std::stack < CheckOverlapResult::shared_ptr_type > S;
		
			CheckOverlapResultInputPushBack(std::string const & filename) : CORI(filename), S()
			{
			
			}
			
			CheckOverlapResult::shared_ptr_type get()
			{
				if ( S.size() )
				{
					CheckOverlapResult::shared_ptr_type C = S.top();
					S.pop();
					return C;
				}
				else
				{
					return CORI.get();
				}
			}
			
			void putback(CheckOverlapResult::shared_ptr_type C)
			{
				S.push(C);
			}
			
			bool fillSourceVector(std::vector < CheckOverlapResult::shared_ptr_type > & V)
			{
				V.clear();
				
				CheckOverlapResult::shared_ptr_type C = get();
				
				if ( C )
				{
					V.push_back(C);
					
					while ( (C = get()) && (C->ia == V.front()->ia) )
						V.push_back(C);
					
					if ( C )
					{
						putback(C);
						assert ( C->ia != V.front()->ia );
					}
				}
				
				return V.size() != 0;
			}
		};
	}
}
#endif
