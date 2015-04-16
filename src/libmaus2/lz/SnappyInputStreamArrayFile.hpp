/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_LZ_SNAPPYINPUTSTREAMARRAYFILE_HPP)
#define LIBMAUS_LZ_SNAPPYINPUTSTREAMARRAYFILE_HPP

#include <libmaus2/lz/SnappyInputStreamArray.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct SnappyInputStreamArrayFile
		{
			typedef SnappyInputStreamArrayFile this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			libmaus2::aio::CheckedInputStream istr;
			SnappyInputStreamArray array;
			
			template<typename iterator>
			SnappyInputStreamArrayFile(std::string const & filename, iterator offa, iterator offe)
			: istr(filename), array(istr,offa,offe)
			{
			
			}
			
			template<typename iterator>
			static unique_ptr_type construct(std::string const & filename, iterator offa, iterator offe)
			{
				unique_ptr_type ptr(new this_type(filename,offa,offe));
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			static unique_ptr_type construct(std::string const & filename, std::vector<uint64_t> const & V)
			{
				unique_ptr_type ptr(construct(filename,V.begin(),V.end()));
				return UNIQUE_PTR_MOVE(ptr);
			}

			SnappyInputStream & operator[](uint64_t const i)
			{
				return array[i];
			}

			SnappyInputStream const & operator[](uint64_t const i) const
			{
				return array[i];
			}

			void setEofThrow(bool const v)
			{
				array.setEofThrow(v);
			}
		};
	}
}
#endif
