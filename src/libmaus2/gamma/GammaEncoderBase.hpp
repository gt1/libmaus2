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
#if ! defined(LIBMAUS2_GAMMA_GAMMAENCODERBASE_HPP)
#define LIBMAUS2_GAMMA_GAMMAENCODERBASE_HPP

#include <libmaus2/bitio/Clz.hpp>
#include <libmaus2/math/UnsignedInteger.hpp>
#include <climits>

namespace libmaus2
{
	namespace gamma
	{
		template<typename stream_data_type>
		struct GammaEncoderBase
		{
		};

		template<>
		struct GammaEncoderBase<uint64_t> : public libmaus2::bitio::Clz
		{
			typedef uint64_t stream_data_type;

			static inline unsigned int getLengthCode(stream_data_type const code)
			{
				unsigned int const lz = clz(code); // number of leading zero bits
				unsigned int const nd = ((CHAR_BIT*sizeof(stream_data_type))-1)-lz;
				return nd;
			}

			static inline unsigned int getCodeLen(stream_data_type const code)
			{
				unsigned int const nd = getLengthCode(code);
				return 1 + (nd<<1);
			}
		};

		template<size_t k>
		struct GammaEncoderBase< libmaus2::math::UnsignedInteger<k> >
		{
			typedef libmaus2::math::UnsignedInteger<k> stream_data_type;

			static inline unsigned int getLengthCode(stream_data_type const code)
			{
				unsigned int const lz = code.clz(); // number of leading zero bits
				unsigned int const nd = ((CHAR_BIT*sizeof(stream_data_type))-1)-lz;
				return nd;
			}
			static inline unsigned int getCodeLen(stream_data_type const code)
			{
				unsigned int const nd = getLengthCode(code);
				return 1 + (nd<<1);
			}
		};

		#if defined(LIBMAUS2_HAVE_UNSIGNED_INT128)
		template<>
		struct GammaEncoderBase<libmaus2::uint128_t>
		{
			static inline unsigned int getLengthCode(libmaus2::uint128_t const code)
			{
				unsigned int const lz =
					(code >> 64) ?
						libmaus2::bitio::Clz::clz(static_cast<uint64_t>(code>>64))
						:
						(64+libmaus2::bitio::Clz::clz(static_cast<uint64_t>(code)))
						; // number of leading zero bits
				unsigned int const nd = ((CHAR_BIT*sizeof(libmaus2::uint128_t))-1)-lz;
				return nd;
			}
			static inline unsigned int getCodeLen(libmaus2::uint128_t const code)
			{
				unsigned int const nd = getLengthCode(code);
				return 1 + (nd<<1);
			}
		};
		#endif
	}
}
#endif
