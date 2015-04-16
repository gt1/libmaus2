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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_CRAMOUTPUTBLOCK_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_CRAMOUTPUTBLOCK_HPP

#include <libmaus/bambam/parallel/CramInterface.h>
#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct CramOutputBlock
			{
				typedef CramOutputBlock this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
				ssize_t blockid;
				size_t subblockid;
				libmaus::autoarray::AutoArray<char>::shared_ptr_type A;
				size_t fill;
				cram_data_write_block_type blocktype;
				
				CramOutputBlock()
				: blockid(std::numeric_limits<ssize_t>::min()), subblockid(0), A(), fill(0)
				{
				
				}
				
				size_t size() const
				{
					if ( A )
						return A->size();
					else
						return 0;
				}
				
				void resize(size_t const s)
				{
					if ( ! A )
					{
						libmaus::autoarray::AutoArray<char>::shared_ptr_type T(new libmaus::autoarray::AutoArray<char>(s,false));
						A = T;
					}
					else
					{
						*A = libmaus::autoarray::AutoArray<char>(s,false);
					}
				}
				
				void ensure(size_t const s)
				{
					if ( s > size() )
						resize(s);
				}
				
				void set(
					char const * p, size_t const n, ssize_t const rblockid, size_t const rsubblockid,
					cram_data_write_block_type const rblocktype
				)
				{
					ensure(n);
					if ( n )
						std::copy(p,p+n,A->begin());
					blockid = rblockid;
					subblockid = rsubblockid;
					fill = n;
					blocktype = rblocktype;
				}
			};
		}
	}
}
#endif
