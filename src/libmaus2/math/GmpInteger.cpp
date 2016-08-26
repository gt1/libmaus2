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
#include <libmaus2/math/GmpInteger.hpp>

#if defined(LIBMAUS2_HAVE_GMP)
#include <gmp.h>
#endif

#if defined(LIBMAUS2_HAVE_GMP)
struct Tmp
{
	mpz_t tv;

	Tmp(int64_t const t)
	{
		mpz_init_set_ui(tv,t);
	}
	~Tmp()
	{
		mpz_clear(tv);
	}

	std::string toString() const
	{
		int const len = gmp_snprintf (0, 0, "%Zd", tv);
		libmaus2::autoarray::AutoArray<char> A(len+1,false);
		gmp_snprintf (A.begin(), A.size(), "%Zd", tv);
		return std::string(A.begin());
	}
};
#endif

#if defined(LIBMAUS2_HAVE_GMP)
struct MpzContainer
{
	mpz_t v;

	MpzContainer()
	{

	}
	~MpzContainer()
	{

	}
};
#endif

#if defined(LIBMAUS2_HAVE_GMP)
static mpz_t & decode(void * data)
{
	return reinterpret_cast<MpzContainer *>(data)->v;
}
#endif

libmaus2::math::GmpInteger::GmpInteger(
	int64_t
	#if defined(LIBMAUS2_HAVE_GMP)
		rv
	#endif
)
	#if defined(LIBMAUS2_HAVE_GMP)
	: v(0)
	#endif
{
	#if defined(LIBMAUS2_HAVE_GMP)
	v = new MpzContainer();

	mpz_init(decode(v));

	if ( rv )
	{
		bool const neg = rv < 0;
		if ( neg )
			rv = -rv;

		uint64_t uv = static_cast<uint64_t>(rv);

		unsigned int const bytecopy = 2;
		uint64_t const mask = (1ull << (bytecopy * 8))-1;
		unsigned int const runs = sizeof(uint64_t) / bytecopy;

		for ( unsigned int i = 0; i < sizeof(uint64_t)/bytecopy; ++i )
		{
			unsigned int const shift = (runs-i-1)*8*bytecopy;

			Tmp stmp((uv >> shift) & mask);
			mpz_mul_2exp(stmp.tv,stmp.tv,shift);
			mpz_add(decode(v),decode(v),stmp.tv);
		}

		if ( neg )
			mpz_neg(decode(v),decode(v));
	}
	#else
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "libmaus2::math::GmpInteger(): library is not built with gmp support" << std::endl;
	lme.finish();
	throw lme;
	#endif
}
libmaus2::math::GmpInteger::GmpInteger(
	GmpInteger const &
	#if defined(LIBMAUS2_HAVE_GMP)
		o
	#endif
)
	#if defined(LIBMAUS2_HAVE_GMP)
	: v(0)
	#endif
{
	#if defined(LIBMAUS2_HAVE_GMP)
	v = new MpzContainer;
	mpz_init(decode(v));
	mpz_set(decode(v),decode(o.v));
	#endif
}
libmaus2::math::GmpInteger::~GmpInteger()
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_clear(decode(v));
	delete reinterpret_cast<MpzContainer *>(v);
	#endif
}

libmaus2::math::GmpInteger & libmaus2::math::GmpInteger::operator=(
	GmpInteger const &
	#if defined(LIBMAUS2_HAVE_GMP)
		o
	#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	if ( this != &o )
		mpz_set(decode(v),decode(o.v));
	#endif
	return *this;
}

std::string libmaus2::math::GmpInteger::toString() const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	int const len = gmp_snprintf (0, 0, "%Zd", decode(v));
	libmaus2::autoarray::AutoArray<char> A(len+1,false);
	gmp_snprintf (A.begin(), A.size(), "%Zd", decode(v));
	return std::string(A.begin());
	#else
	return std::string();
	#endif
}

libmaus2::math::GmpInteger & libmaus2::math::GmpInteger::operator+=(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_add(decode(v),decode(v),decode(o.v));
	#endif
	return *this;
}

libmaus2::math::GmpInteger & libmaus2::math::GmpInteger::operator-=(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_sub(decode(v),decode(v),decode(o.v));
	#endif
	return *this;
}

libmaus2::math::GmpInteger & libmaus2::math::GmpInteger::operator*=(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_mul(decode(v),decode(v),decode(o.v));
	#endif
	return *this;
}

libmaus2::math::GmpInteger & libmaus2::math::GmpInteger::operator/=(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_tdiv_q(decode(v),decode(v),decode(o.v));
	#endif
	return *this;
}

libmaus2::math::GmpInteger & libmaus2::math::GmpInteger::operator%=(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_tdiv_r(decode(v),decode(v),decode(o.v));
	#endif
	return *this;
}

libmaus2::math::GmpInteger & libmaus2::math::GmpInteger::operator-()
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_neg(decode(v),decode(v));
	#endif
	return *this;
}

bool libmaus2::math::GmpInteger::operator<(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpz_cmp(decode(v),decode(o.v)) < 0;
	#else
	return 0;
	#endif
}

bool libmaus2::math::GmpInteger::operator<=(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpz_cmp(decode(v),decode(o.v)) <= 0;
	#else
	return 0;
	#endif
}

bool libmaus2::math::GmpInteger::operator==(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpz_cmp(decode(v),decode(o.v)) == 0;
	#else
	return 0;
	#endif
}

bool libmaus2::math::GmpInteger::operator!=(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpz_cmp(decode(v),decode(o.v)) != 0;
	#else
	return 0;
	#endif
}

bool libmaus2::math::GmpInteger::operator>(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpz_cmp(decode(v),decode(o.v)) > 0;
	#else
	return 0;
	#endif
}

bool libmaus2::math::GmpInteger::operator>=(
	GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpz_cmp(decode(v),decode(o.v)) >= 0;
	#else
	return 0;
	#endif
}

libmaus2::math::GmpInteger::operator double() const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpz_get_d(decode(v));
	#else
	return 0;
	#endif
}

libmaus2::math::GmpInteger::operator uint64_t() const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	libmaus2::math::GmpInteger const mask(1ull << 32);

	libmaus2::math::GmpInteger const low = *this % mask;
	libmaus2::math::GmpInteger const high = (*this / mask)%mask;

	uint64_t const ulow = mpz_get_ui(decode(low.v));
	uint64_t const uhigh = mpz_get_ui(decode(high.v));

	return (uhigh<<32) | ulow;
	#else
	return 0;
	#endif
}

libmaus2::math::GmpInteger::operator int64_t() const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	libmaus2::math::GmpInteger R = *this;
	bool neg = false;
	if ( R < libmaus2::math::GmpInteger() )
	{
		R = -R;
		neg = true;
	}

	libmaus2::math::GmpInteger const mask(1ull << 32);

	libmaus2::math::GmpInteger const low = *this % mask;
	libmaus2::math::GmpInteger const high = (*this / mask)%mask;

	uint64_t const ulow = mpz_get_ui(decode(low.v));
	uint64_t const uhigh = mpz_get_ui(decode(high.v));

	int64_t const v = static_cast<int64_t>((uhigh<<32) | ulow);
	return neg ? -v : v;
	#else
	return 0;
	#endif
}

libmaus2::math::GmpInteger libmaus2::math::operator+(
	libmaus2::math::GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		A
		#endif
		,
	libmaus2::math::GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		B
		#endif
)
{
	libmaus2::math::GmpInteger R;
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_add(decode(R.v),decode(A.v),decode(B.v));
	#endif
	return R;
}
libmaus2::math::GmpInteger libmaus2::math::operator-(
	libmaus2::math::GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		A
		#endif
		,
	libmaus2::math::GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		B
		#endif
)
{
	libmaus2::math::GmpInteger R;
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_sub(decode(R.v),decode(A.v),decode(B.v));
	#endif
	return R;
}
libmaus2::math::GmpInteger libmaus2::math::operator*(
	libmaus2::math::GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		A
		#endif
		,
	libmaus2::math::GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		B
		#endif
)
{
	libmaus2::math::GmpInteger R;
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_mul(decode(R.v),decode(A.v),decode(B.v));
	#endif
	return R;
}
libmaus2::math::GmpInteger libmaus2::math::operator/(
	libmaus2::math::GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		A
		#endif
		,
	libmaus2::math::GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		B
		#endif
)
{
	libmaus2::math::GmpInteger R;
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_tdiv_q(decode(R.v),decode(A.v),decode(B.v));
	#endif
	return R;
}
libmaus2::math::GmpInteger libmaus2::math::operator%(
	libmaus2::math::GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		A
		#endif
		,
	libmaus2::math::GmpInteger const &
		#if defined(LIBMAUS2_HAVE_GMP)
		B
		#endif
)
{
	libmaus2::math::GmpInteger R;
	#if defined(LIBMAUS2_HAVE_GMP)
	mpz_tdiv_r(decode(R.v),decode(A.v),decode(B.v));
	#endif
	return R;
}

std::ostream & libmaus2::math::operator<<(std::ostream & out, libmaus2::math::GmpInteger const & G)
{
	return out << G.toString();
}
