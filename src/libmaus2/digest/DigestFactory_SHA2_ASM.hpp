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
#if ! defined(LIBMAUS_DIGEST_DIGESTFACTORY_SHA2_ASM_HPP)
#define LIBMAUS_DIGEST_DIGESTFACTORY_SHA2_ASM_HPP

#include <libmaus2/digest/DigestFactoryInterface.hpp>

namespace libmaus2
{
	namespace digest
	{
		struct DigestFactory_SHA2_ASM : DigestFactoryInterface
		{
			static std::set<std::string> getSupportedDigestsStatic();
			
			std::set<std::string> getSupportedDigests() const
			{
				return getSupportedDigestsStatic();
			}
			
			static libmaus2::digest::DigestInterface::unique_ptr_type constructStatic(std::string const & name);

			libmaus2::digest::DigestInterface::unique_ptr_type construct(std::string const & name) const
			{
				libmaus2::digest::DigestInterface::unique_ptr_type tptr(constructStatic(name));
				return UNIQUE_PTR_MOVE(tptr);
			}

			DigestFactoryInterface::shared_ptr_type sclone() const
			{
				DigestFactoryInterface::shared_ptr_type ptr(new DigestFactory_SHA2_ASM);
				return ptr;
			}
		};
	}
}
#endif

