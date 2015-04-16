/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#include <libmaus2/digest/DigestFactory_SHA2_ASM.hpp>

std::set<std::string> libmaus2::digest::DigestFactory_SHA2_ASM::getSupportedDigestsStatic()
{
	std::set<std::string> S;

	#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_x86_64) && defined(LIBMAUS2_HAVE_i386) && defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
	if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		S.insert("sha256");
		S.insert("sha512");
	}
	#endif

	return S;
}
						
libmaus2::digest::DigestInterface::unique_ptr_type libmaus2::digest::DigestFactory_SHA2_ASM::constructStatic(std::string const & name)
{
	#if defined(LIBMAUS2_USE_ASSEMBLY) && defined(LIBMAUS2_HAVE_x86_64) && defined(LIBMAUS2_HAVE_i386) && defined(LIBMAUS2_HAVE_SHA2_ASSEMBLY)
	if ( libmaus2::util::I386CacheLineSize::hasSSE41() )
	{
		if ( name == "sha256" )
		{
			libmaus2::digest::DigestInterface::unique_ptr_type tptr(new libmaus2::digest::SHA2_256_sse4);
			return UNIQUE_PTR_MOVE(tptr);
		}
		else if ( name == "sha512" )
		{
			libmaus2::digest::DigestInterface::unique_ptr_type tptr(new libmaus2::digest::SHA2_512_sse4);
			return UNIQUE_PTR_MOVE(tptr);
		}
	}
	else
	#endif
	{
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "DigestFactory: unsupported hash " << name << std::endl;
		lme.finish();
		throw lme;
	}
}
