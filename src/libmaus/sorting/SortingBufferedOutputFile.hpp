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
#if ! defined(LIBMAUS_SORTING_SORTINGBUFFEREDOUTPUTFILE_HPP)
#define LIBMAUS_SORTING_SORTINGBUFFEREDOUTPUTFILE_HPP

#include <libmaus/sorting/MergingReadBack.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>

namespace libmaus
{
	namespace sorting
	{
		template<typename _data_type, typename _order_type = std::less<_data_type> >
		struct SortingBufferedOutputFile
		{
			typedef _data_type data_type;
			typedef _order_type order_type;
			typedef SortingBufferedOutputFile<data_type,order_type> this_type;
			typedef typename libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::sorting::MergingReadBack<data_type,order_type> merger_type;
			typedef typename libmaus::sorting::MergingReadBack<data_type,order_type>::unique_ptr_type merger_ptr_type;
			
			std::string const filename;
			libmaus::aio::CheckedOutputStream COS;
			libmaus::aio::SortingBufferedOutput<data_type,order_type> SBO;
			
			SortingBufferedOutputFile(std::string const & rfilename, uint64_t const bufsize = 1024ull)
			: filename(rfilename), COS(filename), SBO(COS,bufsize)
			{
			}
			
			void put(data_type v)
			{
				SBO.put(v);
			}
			
			merger_ptr_type getMerger(uint64_t const backblocksize = 1024ull)
			{
				SBO.flush();
				typename libmaus::sorting::MergingReadBack<data_type,order_type>::unique_ptr_type ptr(
					new libmaus::sorting::MergingReadBack<data_type,order_type>(filename,SBO.getBlockSizes(),backblocksize)
				);
				
				return UNIQUE_PTR_MOVE(ptr);
			}
		};
	}
}
#endif
