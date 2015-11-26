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
#include <libmaus2/fastx/acgtnMap.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct DalignerLocalAlignment : public EditDistanceTraceContainer
		{
			typedef DalignerLocalAlignment this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef LocalEditDistanceResult result_type;

			#if defined(LIBMAUS2_HAVE_DALIGNER)
			void * data;
			libmaus2::autoarray::AutoArray<char> CA;
			libmaus2::autoarray::AutoArray<char> CB;
			#endif

			libmaus2::autoarray::AutoArray<uint8_t> UA;
			libmaus2::autoarray::AutoArray<uint8_t> UB;

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

			LocalEditDistanceResult processPreMapped(
				#if defined(LIBMAUS2_HAVE_DALIGNER)
				uint8_t const * a, uint64_t const n, uint64_t const seedposa, uint8_t const * b, uint64_t const m, uint64_t const seedposb
				#else
				uint8_t const *, uint64_t const, uint64_t const, uint8_t const *, uint64_t const, uint64_t const
				#endif
			);

			LocalEditDistanceResult process(
				#if defined(LIBMAUS2_HAVE_DALIGNER)
				uint8_t const * a, uint64_t const n, uint64_t const seedposa, uint8_t const * b, uint64_t const m, uint64_t const seedposb
				#else
				uint8_t const *, uint64_t const, uint64_t const, uint8_t const *, uint64_t const, uint64_t const
				#endif
			);

			LocalEditDistanceResult computeDenseTrace(
				#if defined(LIBMAUS2_HAVE_DALIGNER)
				uint8_t const * a, uint64_t const n, uint8_t const * b, uint64_t const m, int64_t const tspace, std::pair<uint16_t,uint16_t> const * trace, uint64_t const tracelen,
				int32_t const diffs,
				int32_t const abpos,
				int32_t const bbpos,
				int32_t const aepos,
				int32_t const bepos
				#else
				uint8_t const *, uint64_t const, uint8_t const *, uint64_t const, int64_t const, std::pair<uint16_t,uint16_t> const *, uint64_t const,
				int32_t const,
				int32_t const,
				int32_t const,
				int32_t const,
				int32_t const
				#endif
			);

			LocalEditDistanceResult computeDenseTracePreMapped(
				#if defined(LIBMAUS2_HAVE_DALIGNER)
				uint8_t const * a, uint64_t const n, uint8_t const * b, uint64_t const m, int64_t const tspace, std::pair<uint16_t,uint16_t> const * trace, uint64_t const tracelen,
				int32_t const diffs,
				int32_t const abpos,
				int32_t const bbpos,
				int32_t const aepos,
				int32_t const bepos
				#else
				uint8_t const *, uint64_t const, uint8_t const *, uint64_t const, int64_t const, std::pair<uint16_t,uint16_t> const *, uint64_t const,
				int32_t const,
				int32_t const,
				int32_t const,
				int32_t const,
				int32_t const
				#endif
			);

			template<typename iterator_a, typename iterator_b>
			LocalEditDistanceResult process(
				iterator_a aa, iterator_a ae, uint64_t const seedposa,
				iterator_b ba, iterator_b be, uint64_t const seedposb,
				typename ::std::iterator_traits<iterator_a>::value_type (*mapfunction_a)(typename ::std::iterator_traits<iterator_a>::value_type) = libmaus2::fastx::mapChar,
				typename ::std::iterator_traits<iterator_b>::value_type (*mapfunction_b)(typename ::std::iterator_traits<iterator_b>::value_type) = libmaus2::fastx::mapChar
			)
			{
				if ( ae-aa > static_cast<ptrdiff_t>(UA.size()) )
					UA.resize(ae-aa);
				if ( be-ba > static_cast<ptrdiff_t>(UB.size()) )
					UB.resize(be-ba);

				std::copy(aa,ae,UA.begin());
				std::copy(ba,be,UB.begin());

				for ( ptrdiff_t i = 0; i < (ae-aa); ++i )
					UA[i] = mapfunction_a(UA[i]);
				for ( ptrdiff_t i = 0; i < (be-ba); ++i )
					UB[i] = mapfunction_b(UB[i]);

				LocalEditDistanceResult const R = process(UA.begin(),static_cast<uint64_t>(ae-aa),seedposa,UB.begin(),static_cast<uint64_t>(be-ba),seedposb);

				return R;
			}

			template<typename iterator_a>
			LocalEditDistanceResult process(
				iterator_a aa, iterator_a ae, uint64_t const seedposa,
				iterator_a ba, iterator_a be, uint64_t const seedposb,
				typename ::std::iterator_traits<iterator_a>::value_type (*mapfunction_a)(typename ::std::iterator_traits<iterator_a>::value_type) = libmaus2::fastx::mapChar,
				typename ::std::iterator_traits<iterator_a>::value_type (*mapfunction_b)(typename ::std::iterator_traits<iterator_a>::value_type) = libmaus2::fastx::mapChar
			)
			{
				if ( (aa != ba) || (ae != be) )
				{
					if ( ae-aa > static_cast<ptrdiff_t>(UA.size()) )
						UA.resize(ae-aa);
					if ( be-ba > static_cast<ptrdiff_t>(UB.size()) )
						UB.resize(be-ba);

					std::copy(aa,ae,UA.begin());
					std::copy(ba,be,UB.begin());

					for ( ptrdiff_t i = 0; i < (ae-aa); ++i )
						UA[i] = mapfunction_a(UA[i]);
					for ( ptrdiff_t i = 0; i < (be-ba); ++i )
						UB[i] = mapfunction_b(UB[i]);

					LocalEditDistanceResult const R = process(UA.begin(),static_cast<uint64_t>(ae-aa),seedposa,UB.begin(),static_cast<uint64_t>(be-ba),seedposb);

					return R;
				}
				else
				{
					assert ( aa == ba );
					assert ( ae == be );

					if ( ae-aa > static_cast<ptrdiff_t>(UA.size()) )
						UA.resize(ae-aa);

					std::copy(aa,ae,UA.begin());

					for ( ptrdiff_t i = 0; i < (ae-aa); ++i )
						UA[i] = mapfunction_a(UA[i]);

					LocalEditDistanceResult const R = process(UA.begin(),static_cast<uint64_t>(ae-aa),seedposa,UA.begin(),static_cast<uint64_t>(be-ba),seedposb);

					return R;
				}
			}

			template<typename iterator_a>
			LocalEditDistanceResult computeDenseTrace(
				iterator_a aa, iterator_a ae,
				iterator_a ba, iterator_a be,
				int64_t const tspace,
				std::pair<uint16_t,uint16_t> const * trace, uint64_t const tracelen,
				int32_t diffs,
				int32_t abpos,
				int32_t bbpos,
				int32_t aepos,
				int32_t bepos,
				typename ::std::iterator_traits<iterator_a>::value_type (*mapfunction_a)(typename ::std::iterator_traits<iterator_a>::value_type) = libmaus2::fastx::mapChar,
				typename ::std::iterator_traits<iterator_a>::value_type (*mapfunction_b)(typename ::std::iterator_traits<iterator_a>::value_type) = libmaus2::fastx::mapChar
			)
			{
				if ( (aa != ba) || (ae != be) )
				{
					if ( ae-aa > static_cast<ptrdiff_t>(UA.size()) )
						UA.resize(ae-aa);
					if ( be-ba > static_cast<ptrdiff_t>(UB.size()) )
						UB.resize(be-ba);

					std::copy(aa,ae,UA.begin());
					std::copy(ba,be,UB.begin());

					for ( ptrdiff_t i = 0; i < (ae-aa); ++i )
						UA[i] = mapfunction_a(UA[i]);
					for ( ptrdiff_t i = 0; i < (be-ba); ++i )
						UB[i] = mapfunction_b(UB[i]);

					return computeDenseTrace(UA.begin(),static_cast<uint64_t>(ae-aa),UB.begin(),static_cast<uint64_t>(be-ba),tspace,trace,tracelen,diffs,abpos,bbpos,aepos,bepos);
				}
				else
				{
					assert ( aa == ba );
					assert ( ae == be );

					if ( ae-aa > static_cast<ptrdiff_t>(UA.size()) )
						UA.resize(ae-aa);

					std::copy(aa,ae,UA.begin());

					for ( ptrdiff_t i = 0; i < (ae-aa); ++i )
						UA[i] = mapfunction_a(UA[i]);

					return computeDenseTrace(UA.begin(),static_cast<uint64_t>(ae-aa),UA.begin(),static_cast<uint64_t>(be-ba),tspace,trace,tracelen,diffs,abpos,bbpos,aepos,bepos);
				}
			}
		};
	}
}
#endif
