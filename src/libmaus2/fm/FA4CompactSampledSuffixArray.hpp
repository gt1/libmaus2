/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_FM_FA4COMPACTSAMPLEDSUFFIXARRAY_HPP)
#define LIBMAUS2_FM_FA4COMPACTSAMPLEDSUFFIXARRAY_HPP

#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>

namespace libmaus2
{
	namespace fm
	{
		struct FA4CompactSampledSuffixArray
		{
			typedef FA4CompactSampledSuffixArray this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			uint64_t samplingrate;
			libmaus2::bitio::CompactArray CA;
			
			FA4CompactSampledSuffixArray(std::istream & in)
			: samplingrate(libmaus2::util::NumberSerialisation::deserialiseNumber(in)), CA(in)
			{
			
			}
			
			static unique_ptr_type load(std::istream & in)
			{
				unique_ptr_type tptr(new this_type(in));
				return UNIQUE_PTR_MOVE(tptr);
			}

			static unique_ptr_type load(std::string const & s)
			{
				libmaus2::aio::InputStreamInstance ISI(s);
				unique_ptr_type tptr(new this_type(ISI));
				return UNIQUE_PTR_MOVE(tptr);
			}
			
			uint64_t operator[](uint64_t const i) const
			{
				return CA[i];
			}
			
			uint64_t size() const
			{
				return CA.size();
			}
		};
	}
}
#endif
