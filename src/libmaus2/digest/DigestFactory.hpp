/*
    libmaus
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
#if ! defined(LIBMAUS_DIGEST_DIGESTFACTORY_HPP)
#define LIBMAUS_DIGEST_DIGESTFACTORY_HPP

#include <libmaus/digest/DigestFactoryInterface.hpp>

namespace libmaus
{
	namespace digest
	{
		struct DigestFactory : DigestFactoryInterface
		{
			static std::set<std::string> getSupportedDigestsStatic()
			{
				std::set<std::string> S;
				S.insert("crc32");
				S.insert("crc32c");
				S.insert("md5");
				S.insert("null");
				#if defined(LIBMAUS_HAVE_NETTLE)
				S.insert("sha1");
				S.insert("sha224");
				S.insert("sha256");
				S.insert("sha384");
				S.insert("sha512");
				#endif
				return S;
			}
			
			std::set<std::string> getSupportedDigests() const
			{
				return getSupportedDigestsStatic();
			}
			
			static libmaus::digest::DigestInterface::unique_ptr_type constructStatic(std::string const & name)
			{
				if ( name == "crc32" )
				{
					libmaus::digest::DigestInterface::unique_ptr_type tptr(new libmaus::digest::CRC32);
					return UNIQUE_PTR_MOVE(tptr);
				}
				else if ( name == "crc32c" )
				{
					libmaus::digest::DigestInterface::unique_ptr_type tptr(new libmaus::digest::CRC32C);
					return UNIQUE_PTR_MOVE(tptr);
				}
				else if ( name == "md5" )
				{
					libmaus::digest::DigestInterface::unique_ptr_type tptr(new libmaus::util::MD5);
					return UNIQUE_PTR_MOVE(tptr);		
				}
				else if ( name == "null" )
				{
					libmaus::digest::DigestInterface::unique_ptr_type tptr(new libmaus::digest::Null);
					return UNIQUE_PTR_MOVE(tptr);		
				}
				#if defined(LIBMAUS_HAVE_NETTLE)
				else if ( name == "sha1" )
				{
					libmaus::digest::DigestInterface::unique_ptr_type tptr(new libmaus::digest::SHA1);
					return UNIQUE_PTR_MOVE(tptr);				
				}
				else if ( name == "sha224" )
				{
					libmaus::digest::DigestInterface::unique_ptr_type tptr(new libmaus::digest::SHA2_224);
					return UNIQUE_PTR_MOVE(tptr);				
				}
				else if ( name == "sha256" )
				{
					libmaus::digest::DigestInterface::unique_ptr_type tptr(new libmaus::digest::SHA2_256);
					return UNIQUE_PTR_MOVE(tptr);				
				}
				else if ( name == "sha384" )
				{
					libmaus::digest::DigestInterface::unique_ptr_type tptr(new libmaus::digest::SHA2_384);
					return UNIQUE_PTR_MOVE(tptr);				
				}
				else if ( name == "sha512" )
				{
					libmaus::digest::DigestInterface::unique_ptr_type tptr(new libmaus::digest::SHA2_512);
					return UNIQUE_PTR_MOVE(tptr);				
				}
				#endif
				else
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "DigestFactory: unsupported hash " << name << std::endl;
					lme.finish();
					throw lme;
				}
			}

			libmaus::digest::DigestInterface::unique_ptr_type construct(std::string const & name) const
			{
				libmaus::digest::DigestInterface::unique_ptr_type tptr(constructStatic(name));
				return UNIQUE_PTR_MOVE(tptr);
			}

			DigestFactoryInterface::shared_ptr_type sclone() const
			{
				DigestFactoryInterface::shared_ptr_type ptr(new DigestFactory);
				return ptr;
			}
		};
	}
}
#endif

