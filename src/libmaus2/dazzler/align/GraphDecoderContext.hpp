/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_GRAPHDECODERCONTEXT_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_GRAPHDECODERCONTEXT_HPP

#include <libmaus2/dazzler/align/OverlapHeader.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct GraphDecoderContext
			{
				typedef GraphDecoderContext this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::autoarray::AutoArray<uint64_t> absort;
				libmaus2::autoarray::AutoArray<uint64_t> bbsort;
				libmaus2::autoarray::AutoArray<uint64_t> arangesort;
				libmaus2::autoarray::AutoArray<uint64_t> brangesort;
				libmaus2::autoarray::AutoArray<uint64_t> O;
				libmaus2::autoarray::AutoArray<libmaus2::dazzler::align::OverlapHeader> A;
				libmaus2::autoarray::AutoArray<int64_t> F;
				uint64_t n;

				GraphDecoderContext() : n(0) {}

				uint64_t size() const
				{
					return n;
				}

				libmaus2::dazzler::align::OverlapHeader const & operator[](uint64_t const i) const
				{
					if ( ! ( i < n ) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "GraphDecoderContext::operator[](" << i << "): index " << i << " is out of range [" << 0 << "," << n << ")";
						lme.finish();
						throw lme;
					}
					return A[i];
				}

				void setup(uint64_t const rn)
				{
					n = rn;
					absort.ensureSize(n);
					bbsort.ensureSize(n);
					arangesort.ensureSize(n);
					brangesort.ensureSize(n);
					O.ensureSize(n);
					A.ensureSize(n);
					F.ensureSize(n);
				}
			};
		}
	}
}
#endif
