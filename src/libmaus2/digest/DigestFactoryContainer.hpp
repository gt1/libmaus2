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
#if ! defined(LIBMAUS2_DIGEST_DIGESTFACTORYCONTAINER_HPP)
#define LIBMAUS2_DIGEST_DIGESTFACTORYCONTAINER_HPP

#include <libmaus2/digest/DigestFactory.hpp>

namespace libmaus2
{
	namespace digest
	{
		struct DigestFactoryContainer
		{
			static std::map< std::string, DigestFactoryInterface::shared_ptr_type > factories;

			static std::set<std::string> getSupportedDigestsStatic()
			{
				std::set<std::string> S;
				for ( std::map< std::string, DigestFactoryInterface::shared_ptr_type >::const_iterator ita = factories.begin();
					ita != factories.end(); ++ita )
					S.insert(ita->first);
				return S;
			}

			static std::set<std::string> getSupportedDigests()
			{
				return getSupportedDigestsStatic();
			}

			static std::string getSupportedDigestsList()
			{
				std::set<std::string> const S = getSupportedDigests();
				std::ostringstream ostr;

				for ( std::set<std::string>::const_iterator ita = S.begin(); ita != S.end(); ++ita )
				{
					if ( ita != S.begin() )
						ostr << ",";
					ostr << *ita;
				}

				return ostr.str();
			}

			static std::map< std::string, DigestFactoryInterface::shared_ptr_type > setupFactories(DigestFactoryInterface const & factory)
			{
				std::map< std::string, DigestFactoryInterface::shared_ptr_type > M;
				DigestFactoryInterface::shared_ptr_type sfactory(factory.sclone());
				std::set<std::string> S = sfactory->getSupportedDigests();
				for ( std::set<std::string>::const_iterator ita = S.begin(); ita != S.end(); ++ita )
					M [ *ita ] = sfactory;
				return M;
			}

			static std::map< std::string, DigestFactoryInterface::shared_ptr_type > setupFactories()
			{
				DigestFactory factory;
				return setupFactories(factory);
			}

			static void addFactories(DigestFactoryInterface const & factory)
			{
				std::map< std::string, DigestFactoryInterface::shared_ptr_type > M = setupFactories(factory);
				for ( std::map< std::string, DigestFactoryInterface::shared_ptr_type >::iterator ita = M.begin(); ita != M.end(); ++ita )
					factories[ita->first] = ita->second;
			}

			static libmaus2::digest::DigestInterface::unique_ptr_type construct(std::string const & name)
			{
				std::map< std::string, DigestFactoryInterface::shared_ptr_type >::const_iterator ita = factories.find(name);

				if ( ita != factories.end() )
				{
					libmaus2::digest::DigestInterface::unique_ptr_type ptr(ita->second->construct(name));
					return UNIQUE_PTR_MOVE(ptr);
				}
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "DigestFactoryContainer::construct: digest " << name << " is not supported" << std::endl;
					lme.finish();
					throw lme;
				}
			}
		};
	}
}
#endif
