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
#if ! defined(CHECKOVERLAPRESULTMERGEINPUT_HPP)
#define CHECKOVERLAPRESULTMERGEINPUT_HPP

#include <libmaus2/lcs/CheckOverlapResult.hpp>
#include <libmaus2/lcs/CheckOverlapResultInput.hpp>
#include <libmaus2/lcs/CheckOverlapResultIdHeapComparator.hpp>
#include <queue>

namespace libmaus2
{
	namespace lcs
	{
		struct CheckOverlapResultMergeInput
		{
			::libmaus2::autoarray::AutoArray< CheckOverlapResultInput::unique_ptr_type > in;
			
			std::priority_queue <
				std::pair<uint64_t,CheckOverlapResult::shared_ptr_type>, 
				std::vector< std::pair<uint64_t,CheckOverlapResult::shared_ptr_type>  >,
				CheckOverlapResultIdHeapComparator > heap;
		
			CheckOverlapResultMergeInput(std::vector<std::string> const & inputfilenames)
			: in(inputfilenames.size())
			{
				for ( uint64_t i = 0; i < in.size(); ++i )
					in[i] = UNIQUE_PTR_MOVE(CheckOverlapResultInput::unique_ptr_type(new CheckOverlapResultInput(inputfilenames[i])));

				for ( uint64_t i = 0; i < in.size(); ++i )
				{
					CheckOverlapResult::shared_ptr_type ptr = in[i]->get();
					
					if ( ptr )
						heap.push ( 
							std::pair<uint64_t,CheckOverlapResult::shared_ptr_type>(i,ptr) 
						);
				}
			}

			CheckOverlapResult::shared_ptr_type get()
			{
				if ( heap.empty() )
					return CheckOverlapResult::shared_ptr_type();
			
				std::pair<uint64_t,CheckOverlapResult::shared_ptr_type> top = heap.top();
				heap.pop();
				
				CheckOverlapResult::shared_ptr_type newptr = in[top.first]->get();
				
				if ( newptr )
				{
					heap.push ( 
						std::pair<uint64_t,CheckOverlapResult::shared_ptr_type>(top.first,newptr) 
					);
				
				}
		
				return top.second;
			}
			
			static void merge(std::vector<std::string> const & inputfilenames, std::string const & outputfilename)
			{
				CheckOverlapResultMergeInput in(inputfilenames);
				libmaus2::aio::OutputStreamInstance ostr(outputfilename);
				
				CheckOverlapResult::shared_ptr_type ptr;
				
				while ( (ptr=in.get()) )
					ptr->serialise(ostr);
					
				ostr.flush();
				assert ( ostr );
			}
		};
	}
}
#endif
