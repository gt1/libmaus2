/*
    libmaus2
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
#if ! defined(LIBMAUS2_INDEX_EXTERNALMEMORYINDEXRECORD_HPP)
#define LIBMAUS2_INDEX_EXTERNALMEMORYINDEXRECORD_HPP

#include <utility>
#include <libmaus2/types/types.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace index
	{		
		template<typename _data_type>
		struct ExternalMemoryIndexRecord
		{
			typedef _data_type data_type;
			typedef ExternalMemoryIndexRecord<data_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			std::pair<uint64_t,uint64_t> P;
			data_type D;
			
			ExternalMemoryIndexRecord()
			: P(), D()
			{
			
			}
			
			ExternalMemoryIndexRecord(data_type const & rD)
			: P(), D(rD) {}
			
			ExternalMemoryIndexRecord(std::pair<uint64_t,uint64_t> const & rP, data_type const & rD)
			: P(rP), D(rD) {}
			
			template<typename stream_type>
			void deserialise(stream_type & stream)
			{
				P.first = libmaus2::util::NumberSerialisation::deserialiseNumber(stream);
				P.second = libmaus2::util::NumberSerialisation::deserialiseNumber(stream);
				D.deserialise(stream);
			}
			
			bool operator<(this_type const & O) const
			{
				return D < O.D;
			}
			
			bool equal(ExternalMemoryIndexRecord const & O) const
			{
				return (P == O.P) && (D == O.D);
			}
		};
	}
}
#endif
