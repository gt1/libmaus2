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

#if !defined(LIBMAUS2_LCS_DALIGNERLOCALALIGNMENT_HPP)
#define LIBMAUS2_LCS_DALIGNERLOCALALIGNMENT_HPP

#include <libmaus2/lcs/LocalEditDistanceResult.hpp>
#include <libmaus2/lcs/EditDistanceTraceContainer.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct DalignerLocalAlignment : public EditDistanceTraceContainer
		{
			typedef DalignerLocalAlignment this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef LocalEditDistanceResult result_type;

			#if defined(LIBMAUS2_HAVE_DALIGNER)
			void * data;
			libmaus2::autoarray::AutoArray<char> A;
			libmaus2::autoarray::AutoArray<char> B;
			#endif

			public:
			DalignerLocalAlignment(
				#if defined(LIBMAUS2_HAVE_DALIGNER)
				double const correlation = 0.70,
				int64_t const tspace = 100,
				float const afreq = 0.25,
				float const cfreq = 0.25,
				float const gfreq = 0.25,
				float const tfreq = 0.25
				#else
				double const = 0.70,
				int64_t const = 100,
				float const = 0.25,
				float const = 0.25,
				float const = 0.25,
				float const = 0.25
				#endif
			);
			~DalignerLocalAlignment();
			
			LocalEditDistanceResult process(
				#if defined(LIBMAUS2_HAVE_DALIGNER)
				uint8_t const * a, uint64_t const n, uint8_t const * b, uint64_t const m
				#else
				uint8_t const *, uint64_t const, uint8_t const *, uint64_t const
				#endif
			);
		};
	}
}
#endif
