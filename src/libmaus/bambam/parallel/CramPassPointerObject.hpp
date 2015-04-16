/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_CRAMPASSPOINTEROBJECT_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_CRAMPASSPOINTEROBJECT_HPP

#include <libmaus/bambam/parallel/FragmentAlignmentBuffer.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{

			struct CramPassPointerObject
			{
				typedef CramPassPointerObject this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type block;
				libmaus::autoarray::AutoArray<char const *>::shared_ptr_type D;
				libmaus::autoarray::AutoArray<size_t>::shared_ptr_type S;
				libmaus::autoarray::AutoArray<size_t>::shared_ptr_type L;
				size_t numblocks;
				
				CramPassPointerObject() {}
								
				void set(libmaus::bambam::parallel::FragmentAlignmentBuffer::shared_ptr_type rblock)
				{
					block = rblock;
					
					std::vector<std::pair<uint8_t *,uint8_t *> > V;
					block->getLinearOutputFragments(V);
					std::vector<size_t> const fillVector = block->getFillVector();
					assert ( fillVector.size() == V.size() );

					if ( V.size() > (D?D->size():0) )
					{
						libmaus::autoarray::AutoArray<char const *>::shared_ptr_type T(new libmaus::autoarray::AutoArray<char const *>(V.size(),false));
						D = T;
					}
					if ( V.size() > (S?S->size():0) )
					{						
						libmaus::autoarray::AutoArray<size_t>::shared_ptr_type T(new libmaus::autoarray::AutoArray<size_t>(V.size(),false));
						S = T;
					}
					if ( V.size() > (L?L->size():0) )
					{						
						libmaus::autoarray::AutoArray<size_t>::shared_ptr_type T(new libmaus::autoarray::AutoArray<size_t>(V.size(),false));
						L = T;
					}
					for ( uint64_t i = 0; i < V.size(); ++i )
					{
						D->at(i) = reinterpret_cast<char const *>(V[i].first);
						S->at(i) = V[i].second-V[i].first;
						L->at(i) = fillVector.at(i);
					}
					numblocks = V.size();
				}
			};
		}
	}
}
#endif
