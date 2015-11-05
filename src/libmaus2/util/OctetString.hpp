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
#if ! defined(LIBMAUS2_UTIL_OCTETSTRING_HPP)
#define LIBMAUS2_UTIL_OCTETSTRING_HPP

#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/suffixsort/divsufsort.hpp>

namespace libmaus2
{
	namespace util
	{
		struct OctetString
		{
			typedef OctetString this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef uint8_t const * const_iterator;

			::libmaus2::autoarray::AutoArray<uint8_t> A;

			const_iterator begin() const
			{
				return A.begin();
			}

			const_iterator end() const
			{
				return A.end();
			}

			template<typename stream_type>
			static uint64_t computeOctetLengthFromFile(std::string const & fn, uint64_t const len)
			{
				stream_type stream(fn);
				return computeOctetLength(stream,len);
			}

			static uint64_t computeOctetLength(std::istream &, uint64_t const len);

			static shared_ptr_type constructRaw(
				std::string const & filename,
				uint64_t const offset = 0,
				uint64_t const blength = std::numeric_limits<uint64_t>::max()
			);

			OctetString(
				std::string const & filename,
				uint64_t offset = 0,
				uint64_t blength = std::numeric_limits<uint64_t>::max());
			OctetString(std::istream & CIS, uint64_t blength);
			OctetString(std::istream & CIS, uint64_t const octetlength, uint64_t const symlength);

			uint64_t size() const
			{
				return A.size();
			}

			uint8_t operator[](uint64_t const i) const
			{
				return A[i];
			}
			uint8_t get(uint64_t const i) const
			{
				return (*this)[i];
			}

			::libmaus2::util::Histogram::unique_ptr_type getHistogram() const;
			std::map<int64_t,uint64_t> getHistogramAsMap() const;

			typedef ::libmaus2::suffixsort::DivSufSort<32,uint8_t *,uint8_t const *,int32_t *,int32_t const *,256,true> sort_type_parallel;
			typedef ::libmaus2::suffixsort::DivSufSort<32,uint8_t *,uint8_t const *,int32_t *,int32_t const *,256,false> sort_type_serial;
			typedef sort_type_serial::saidx_t saidx_t;

			::libmaus2::autoarray::AutoArray<saidx_t,::libmaus2::autoarray::alloc_type_c>
				computeSuffixArray32(bool const parallel = false) const;
		};
	}
}
#endif
