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
#if ! defined(LIBMAUS2_GAMMA_GAMMADECODERBASE_HPP)
#define LIBMAUS2_GAMMA_GAMMADECODERBASE_HPP

#include <libmaus2/bitio/Clz.hpp>
#include <libmaus2/math/UnsignedInteger.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename stream_data_type>
		struct GammaDecoderBase
		{
		};

		template<>
		struct GammaDecoderBase<uint64_t>
		{
			typedef uint64_t stream_data_type;

			static unsigned int clz(stream_data_type const v)
			{
				return libmaus2::bitio::Clz::clz(v);
			}

			static bool isNonNull(stream_data_type const v)
			{
				return v;
			}
		};

		template<size_t k>
		struct GammaDecoderBase< libmaus2::math::UnsignedInteger<k> >
		{
			typedef libmaus2::math::UnsignedInteger<k> stream_data_type;

			static unsigned int clz(stream_data_type const v)
			{
				return v.clz();
			}

			static bool isNonNull(stream_data_type const v)
			{
				return !(v.isNull());
			}
		};

		#if defined(LIBMAUS2_HAVE_UNSIGNED_INT128)
		template<>
		struct GammaDecoderBase<libmaus2::uint128_t>
		{
			typedef libmaus2::uint128_t stream_data_type;

			static unsigned int clz(stream_data_type const v)
			{
				if ( v >> 64 )
					return libmaus2::bitio::Clz::clz(static_cast<uint64_t>(v >> 64));
				else
					return 64 + libmaus2::bitio::Clz::clz(static_cast<uint64_t>(v));
			}

			static bool isNonNull(stream_data_type const v)
			{
				return v;
			}
		};
		#endif
	}
}
#endif
