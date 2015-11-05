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
#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/lcs/BandedAlignerFactory.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/lcs/SimdX86BandedGlobalAlignmentX128_16.hpp>
#include <libmaus2/lcs/SimdX86BandedGlobalAlignmentX128_8.hpp>
#include <libmaus2/lcs/SimdX86BandedGlobalAlignmentY256_16.hpp>
#include <libmaus2/lcs/SimdX86BandedGlobalAlignmentY256_8.hpp>
#include <libmaus2/util/I386CacheLineSize.hpp>
#include <libmaus2/lcs/BandedEditDistance.hpp>

std::set<libmaus2::lcs::BandedAlignerFactory::aligner_type> libmaus2::lcs::BandedAlignerFactory::getSupportedAligners()
{
	std::set<aligner_type> S;

	S.insert(libmaus2::lcs::BandedAlignerFactory::libmaus2_lcs_BandedAlignerFactory_BandedEditDistance);

	#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_X128_8) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)
	if (
		libmaus2::util::I386CacheLineSize::hasSSE2()
		&&
		libmaus2::util::I386CacheLineSize::hasSSSE3()
	)
	{
		S.insert(libmaus2_lcs_BandedAlignerFactory_x128_8);
	}
	#endif
	#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_X128_16) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)
	if (
		libmaus2::util::I386CacheLineSize::hasSSE2()
		&&
		libmaus2::util::I386CacheLineSize::hasSSSE3()
		&&
		libmaus2::util::I386CacheLineSize::hasSSE41()
	)
	{
		S.insert(libmaus2_lcs_BandedAlignerFactory_x128_16);
	}
	#endif
	#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_Y256_8) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)
	if (
		libmaus2::util::I386CacheLineSize::hasSSE2()
		&&
		libmaus2::util::I386CacheLineSize::hasSSSE3()
		&&
		libmaus2::util::I386CacheLineSize::hasSSE41()
		&&
		libmaus2::util::I386CacheLineSize::hasAVX2()
	)
	{
		S.insert(libmaus2_lcs_BandedAlignerFactory_y256_8);
	}
	#endif
	#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_Y256_16) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)
	if (
		libmaus2::util::I386CacheLineSize::hasSSE2()
		&&
		libmaus2::util::I386CacheLineSize::hasSSSE3()
		&&
		libmaus2::util::I386CacheLineSize::hasSSE41()
		&&
		libmaus2::util::I386CacheLineSize::hasAVX2()
	)
	{
		S.insert(libmaus2_lcs_BandedAlignerFactory_y256_16);
	}
	#endif

	return S;
}

libmaus2::lcs::BandedAligner::unique_ptr_type libmaus2::lcs::BandedAlignerFactory::construct(libmaus2::lcs::BandedAlignerFactory::aligner_type const type)
{
	switch ( type )
	{
		case libmaus2_lcs_BandedAlignerFactory_BandedEditDistance:
		{
			libmaus2::lcs::BandedAligner::unique_ptr_type T(new libmaus2::lcs::BandedEditDistance<>);
			return UNIQUE_PTR_MOVE(T);
		}
		case libmaus2_lcs_BandedAlignerFactory_x128_8:
		{
			#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_X128_8) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)
			if (
				libmaus2::util::I386CacheLineSize::hasSSE2()
				&&
				libmaus2::util::I386CacheLineSize::hasSSSE3()
			)
			{
				libmaus2::lcs::BandedAligner::unique_ptr_type T(new SimdX86BandedGlobalAlignmentX128_8);
				return UNIQUE_PTR_MOVE(T);
			}
			else
			{
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "libmaus2::lcs::BandedAlignerFactory::construct: unsupported aligner type X128_8 (insufficient instruction set)" << std::endl;
				lme.finish();
				throw lme;
			}
			#else
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "libmaus2::lcs::BandedAlignerFactory::construct: unsupported aligner type X128_8" << std::endl;
			lme.finish();
			throw lme;
			#endif
		}
		case libmaus2_lcs_BandedAlignerFactory_x128_16:
		{
			#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_X128_16) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)
			if (
				libmaus2::util::I386CacheLineSize::hasSSE2()
				&&
				libmaus2::util::I386CacheLineSize::hasSSSE3()
				&&
				libmaus2::util::I386CacheLineSize::hasSSE41()
			)
			{
				libmaus2::lcs::BandedAligner::unique_ptr_type T(new libmaus2::lcs::SimdX86BandedGlobalAlignmentX128_16);
				return UNIQUE_PTR_MOVE(T);
			}
			else
			{
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "libmaus2::lcs::BandedAlignerFactory::construct: unsupported aligner type X128_16 (insufficient instruction set)" << std::endl;
				lme.finish();
				throw lme;
			}
			#else
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "libmaus2::lcs::BandedAlignerFactory::construct: unsupported aligner type X128_16" << std::endl;
			lme.finish();
			throw lme;
			#endif
		}
		case libmaus2_lcs_BandedAlignerFactory_y256_8:
		{
			#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_Y256_8) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)
			if (
				libmaus2::util::I386CacheLineSize::hasSSE2()
				&&
				libmaus2::util::I386CacheLineSize::hasSSSE3()
				&&
				libmaus2::util::I386CacheLineSize::hasSSE41()
				&&
				libmaus2::util::I386CacheLineSize::hasAVX2()
			)
			{
				libmaus2::lcs::BandedAligner::unique_ptr_type T(new libmaus2::lcs::SimdX86BandedGlobalAlignmentY256_8);
				return UNIQUE_PTR_MOVE(T);
			}
			else
			{
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "libmaus2::lcs::BandedAlignerFactory::construct: unsupported aligner type Y256_8 (insufficient instruction set)" << std::endl;
				lme.finish();
				throw lme;
			}
			#else
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "libmaus2::lcs::BandedAlignerFactory::construct: unsupported aligner type Y256_8" << std::endl;
			lme.finish();
			throw lme;
			break;
			#endif
		}
		case libmaus2_lcs_BandedAlignerFactory_y256_16:
		{
			#if defined(LIBMAUS2_HAVE_GLOBAL_ALIGNMENT_Y256_16) && defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_i386)
			if (
				libmaus2::util::I386CacheLineSize::hasSSE2()
				&&
				libmaus2::util::I386CacheLineSize::hasSSSE3()
				&&
				libmaus2::util::I386CacheLineSize::hasSSE41()
				&&
				libmaus2::util::I386CacheLineSize::hasAVX2()
			)
			{
				libmaus2::lcs::BandedAligner::unique_ptr_type T(new libmaus2::lcs::SimdX86BandedGlobalAlignmentY256_16);
				return UNIQUE_PTR_MOVE(T);
			}
			else
			{
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "libmaus2::lcs::BandedAlignerFactory::construct: unsupported aligner type Y256_16 (insufficient instruction set)" << std::endl;
				lme.finish();
				throw lme;
			}
			#else
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "libmaus2::lcs::BandedAlignerFactory::construct: unsupported aligner type Y256_16" << std::endl;
			lme.finish();
			throw lme;
			break;
			#endif

		}
		default:
		{
			libmaus2::exception::LibMausException lme;
			lme.getStream() << "libmaus2::lcs::BandedAlignerFactory::construct: unsupported aligner type" << std::endl;
			lme.finish();
			throw lme;
			break;
		}
	}
}

std::ostream & libmaus2::lcs::operator<<(std::ostream & out, BandedAlignerFactory::aligner_type const & A)
{
	switch ( A )
	{
		case ::libmaus2::lcs::BandedAlignerFactory::libmaus2_lcs_BandedAlignerFactory_BandedEditDistance:
			out << "libmaus2_lcs_BandedAlignerFactory_BandedEditDistance";
			break;
		case ::libmaus2::lcs::BandedAlignerFactory::libmaus2_lcs_BandedAlignerFactory_x128_8:
			out << "libmaus2_lcs_BandedAlignerFactory_x128_8";
			break;
		case ::libmaus2::lcs::BandedAlignerFactory::libmaus2_lcs_BandedAlignerFactory_x128_16:
			out << "libmaus2_lcs_BandedAlignerFactory_x128_16";
			break;
		case ::libmaus2::lcs::BandedAlignerFactory::libmaus2_lcs_BandedAlignerFactory_y256_8:
			out << "libmaus2_lcs_BandedAlignerFactory_y256_8";
			break;
		case ::libmaus2::lcs::BandedAlignerFactory::libmaus2_lcs_BandedAlignerFactory_y256_16:
			out << "libmaus2_lcs_BandedAlignerFactory_y256_16";
			break;
	}

	return out;
}
