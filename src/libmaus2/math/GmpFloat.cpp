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
#include <libmaus2/math/GmpFloat.hpp>

#if defined(LIBMAUS2_HAVE_GMP)
#include <gmp.h>
#endif

#if defined(LIBMAUS2_HAVE_GMP)
struct Tmp
{
	mpf_t tv;

	Tmp(double const t)
	{
		mpf_init_set_d(tv,t);
	}
	~Tmp()
	{
		mpf_clear(tv);
	}

	std::string toString() const
	{
		int const len = gmp_snprintf (0, 0, "%Ff", tv);
		libmaus2::autoarray::AutoArray<char> A(len+1,false);
		gmp_snprintf (A.begin(), A.size(), "%Ff", tv);
		return std::string(A.begin());
	}
};
#endif

#if defined(LIBMAUS2_HAVE_GMP)
struct MpfContainer
{
	mpf_t v;

	MpfContainer()
	{

	}
	~MpfContainer()
	{

	}
};
#endif

#if defined(LIBMAUS2_HAVE_GMP)
static mpf_t & decode(void * data)
{
	return reinterpret_cast<MpfContainer *>(data)->v;
}
#endif

libmaus2::math::GmpFloat::GmpFloat(
	double
	#if defined(LIBMAUS2_HAVE_GMP)
		rv
	#endif
	,
	unsigned int
	#if defined(LIBMAUS2_HAVE_GMP)
		prec
	#endif
)
: v(0)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	v = new MpfContainer();
	mpf_init2(decode(v),prec);
	mpf_set_d(decode(v),rv);
	#else
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "libmaus2::math::GmpFloat(): library is not built with gmp support" << std::endl;
	lme.finish();
	throw lme;
	#endif
}
libmaus2::math::GmpFloat::GmpFloat(
	GmpFloat const &
	#if defined(LIBMAUS2_HAVE_GMP)
		o
	#endif
)
: v(0)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	v = new MpfContainer;
	mpf_init_set(decode(v),decode(o.v));
	//mpf_init2(decode(v),mpf_get_prec(decode(o.v)));
	//mpf_set(decode(v),decode(o.v));
	#endif
}
libmaus2::math::GmpFloat::~GmpFloat()
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpf_clear(decode(v));
	delete reinterpret_cast<MpfContainer *>(v);
	#endif
}

libmaus2::math::GmpFloat & libmaus2::math::GmpFloat::operator=(
	GmpFloat const &
	#if defined(LIBMAUS2_HAVE_GMP)
		o
	#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	if ( this != &o )
	{
		mpf_set_prec(decode(v),mpf_get_prec(decode(o.v)));
		mpf_set(decode(v),decode(o.v));
	}
	#endif
	return *this;
}

std::string libmaus2::math::GmpFloat::toString() const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	int const len = gmp_snprintf (0, 0, "%Ff", decode(v));
	libmaus2::autoarray::AutoArray<char> A(len+1,false);
	gmp_snprintf (A.begin(), A.size(), "%Ff", decode(v));
	return std::string(A.begin());
	#else
	return std::string();
	#endif
}

libmaus2::math::GmpFloat & libmaus2::math::GmpFloat::operator+=(
	GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpf_add(decode(v),decode(v),decode(o.v));
	#endif
	return *this;
}

libmaus2::math::GmpFloat & libmaus2::math::GmpFloat::operator-=(
	GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpf_sub(decode(v),decode(v),decode(o.v));
	#endif
	return *this;
}

libmaus2::math::GmpFloat & libmaus2::math::GmpFloat::operator*=(
	GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpf_mul(decode(v),decode(v),decode(o.v));
	#endif
	return *this;
}

libmaus2::math::GmpFloat & libmaus2::math::GmpFloat::operator/=(
	GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
)
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpf_div(decode(v),decode(v),decode(o.v));
	#endif
	return *this;
}

libmaus2::math::GmpFloat & libmaus2::math::GmpFloat::operator-()
{
	#if defined(LIBMAUS2_HAVE_GMP)
	mpf_neg(decode(v),decode(v));
	#endif
	return *this;
}

bool libmaus2::math::GmpFloat::operator<(
	GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpf_cmp(decode(v),decode(o.v)) < 0;
	#else
	return 0;
	#endif
}

bool libmaus2::math::GmpFloat::operator<=(
	GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpf_cmp(decode(v),decode(o.v)) <= 0;
	#else
	return 0;
	#endif
}

bool libmaus2::math::GmpFloat::operator==(
	GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpf_cmp(decode(v),decode(o.v)) == 0;
	#else
	return 0;
	#endif
}

bool libmaus2::math::GmpFloat::operator!=(
	GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpf_cmp(decode(v),decode(o.v)) != 0;
	#else
	return 0;
	#endif
}

bool libmaus2::math::GmpFloat::operator>(
	GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpf_cmp(decode(v),decode(o.v)) > 0;
	#else
	return 0;
	#endif
}

bool libmaus2::math::GmpFloat::operator>=(
	GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		o
		#endif
) const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpf_cmp(decode(v),decode(o.v)) >= 0;
	#else
	return 0;
	#endif
}

libmaus2::math::GmpFloat::operator double() const
{
	#if defined(LIBMAUS2_HAVE_GMP)
	return mpf_get_d(decode(v));
	#else
	return 0;
	#endif
}

libmaus2::math::GmpFloat libmaus2::math::operator+(
	libmaus2::math::GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		A
		#endif
		,
	libmaus2::math::GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		B
		#endif
)
{
	unsigned int const outprec =
	#if defined(LIBMAUS2_HAVE_GMP)
		std::max(mpf_get_prec(decode(A.v)),mpf_get_prec(decode(B.v)));
	#else
		0;
	#endif
	libmaus2::math::GmpFloat R(0,outprec);
	#if defined(LIBMAUS2_HAVE_GMP)
	mpf_add(decode(R.v),decode(A.v),decode(B.v));
	#endif
	return R;
}
libmaus2::math::GmpFloat libmaus2::math::operator-(
	libmaus2::math::GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		A
		#endif
		,
	libmaus2::math::GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		B
		#endif
)
{
	unsigned int const outprec =
	#if defined(LIBMAUS2_HAVE_GMP)
		std::max(mpf_get_prec(decode(A.v)),mpf_get_prec(decode(B.v)));
	#else
		0;
	#endif
	libmaus2::math::GmpFloat R(0,outprec);
	#if defined(LIBMAUS2_HAVE_GMP)
	mpf_sub(decode(R.v),decode(A.v),decode(B.v));
	#endif
	return R;
}
libmaus2::math::GmpFloat libmaus2::math::operator*(
	libmaus2::math::GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		A
		#endif
		,
	libmaus2::math::GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		B
		#endif
)
{
	unsigned int const outprec =
	#if defined(LIBMAUS2_HAVE_GMP)
		std::max(mpf_get_prec(decode(A.v)),mpf_get_prec(decode(B.v)));
	#else
		0;
	#endif
	libmaus2::math::GmpFloat R(0,outprec);
	#if defined(LIBMAUS2_HAVE_GMP)
	mpf_mul(decode(R.v),decode(A.v),decode(B.v));
	#endif
	return R;
}
libmaus2::math::GmpFloat libmaus2::math::operator/(
	libmaus2::math::GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		A
		#endif
		,
	libmaus2::math::GmpFloat const &
		#if defined(LIBMAUS2_HAVE_GMP)
		B
		#endif
)
{
	unsigned int const outprec =
	#if defined(LIBMAUS2_HAVE_GMP)
		std::max(mpf_get_prec(decode(A.v)),mpf_get_prec(decode(B.v)));
	#else
		0;
	#endif
	libmaus2::math::GmpFloat R(0,outprec);
	#if defined(LIBMAUS2_HAVE_GMP)
	mpf_div(decode(R.v),decode(A.v),decode(B.v));
	#endif
	return R;
}

std::ostream & libmaus2::math::operator<<(std::ostream & out, libmaus2::math::GmpFloat const & G)
{
	return out << G.toString();
}
