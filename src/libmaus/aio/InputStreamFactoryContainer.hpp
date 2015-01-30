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
#if ! defined(LIBMAUS_AIO_INPUTSTREAMFACTORYCONTAINER_HPP)
#define LIBMAUS_AIO_INPUTSTREAMFACTORYCONTAINER_HPP

#include <libmaus/aio/InputStreamFactory.hpp>
#include <libmaus/aio/PosixFdInputStreamFactory.hpp>
#include <libmaus/network/UrlInputStreamFactory.hpp>
#include <cctype>

namespace libmaus
{
	namespace aio
	{
		struct InputStreamFactoryContainer
		{
			private:
			static std::map<std::string,libmaus::aio::InputStreamFactory::shared_ptr_type> factories;
			
			static std::map<std::string,libmaus::aio::InputStreamFactory::shared_ptr_type> setupFactories()
			{
				std::map<std::string,libmaus::aio::InputStreamFactory::shared_ptr_type> tfactories;
				
				libmaus::aio::PosixFdInputStreamFactory::shared_ptr_type tfilefact(new libmaus::aio::PosixFdInputStreamFactory);
				tfactories["file"] = tfilefact;

				libmaus::network::UrlInputStreamFactory::shared_ptr_type turlfact(new libmaus::network::UrlInputStreamFactory);
				tfactories["ftp"] = turlfact;
				tfactories["http"] = turlfact;
				tfactories["https"] = turlfact;
				
				return tfactories;
			}
			
			static bool startsWithAlphaColon(std::string const & url)
			{
				uint64_t col = url.size();			
				for ( uint64_t i = 0; i < url.size() && col == url.size(); ++i )
					if ( url[i] == ':' )
						col = i;
				
				if ( col == url.size() )
					return false;
					
				for ( uint64_t i = 0; i < col; ++i )
					if ( !isalpha(static_cast<unsigned char >(url[i])) )
						return false;
						
				return true;
			}
			
			static bool haveFactoryForProtocol(std::string const & url)
			{
				bool const hasurlprefix = startsWithAlphaColon(url);
			
				if ( hasurlprefix )
				{
					uint64_t col = url.size();	
					for ( uint64_t i = 0; i < url.size() && col == url.size(); ++i )
						if ( url[i] == ':' )
							col = i;
					
					std::string const protocol = url.substr(0,col);
											
					if ( factories.find(protocol) != factories.end() )
					{
						libmaus::aio::InputStreamFactory::shared_ptr_type factory = factories.find(protocol)->second;
						return true;
					}
					else
					{
						return false;
					}
				}
				
				return false;
			}
			
			static libmaus::aio::InputStreamFactory::shared_ptr_type getFactory(std::string const & url)
			{
				if ( haveFactoryForProtocol(url) )
				{
					uint64_t col = url.size();	
					for ( uint64_t i = 0; i < url.size() && col == url.size(); ++i )
						if ( url[i] == ':' )
							col = i;
					
					return factories.find(url.substr(0,col))->second;
				}
				else
				{
					return factories.find("file")->second;
				}
			}
			
			public:
			static libmaus::aio::InputStream::unique_ptr_type constructUnique(std::string const & url)
			{
				libmaus::aio::InputStreamFactory::shared_ptr_type factory = getFactory(url);
				
				if ( haveFactoryForProtocol(url) )
				{
					uint64_t col = url.size();	
					for ( uint64_t i = 0; i < url.size() && col == url.size(); ++i )
						if ( url[i] == ':' )
							col = i;
					
					std::string const protocol = url.substr(0,col);
					
					if ( protocol == "ftp" || protocol == "http" || protocol == "https" )
					{
						libmaus::aio::InputStream::unique_ptr_type tptr(factory->constructUnique(url));
						return UNIQUE_PTR_MOVE(tptr);
					}
					else
					{
						libmaus::aio::InputStream::unique_ptr_type tptr(factory->constructUnique(url.substr(protocol.size()+1)));
						return UNIQUE_PTR_MOVE(tptr);
					}
				}
				else
				{
					libmaus::aio::InputStream::unique_ptr_type tptr(factory->constructUnique(url));
					return UNIQUE_PTR_MOVE(tptr);				
				}
			}

			static libmaus::aio::InputStream::shared_ptr_type constructShared(std::string const & url)
			{
				libmaus::aio::InputStreamFactory::shared_ptr_type factory = getFactory(url);
				
				if ( haveFactoryForProtocol(url) )
				{
					uint64_t col = url.size();	
					for ( uint64_t i = 0; i < url.size() && col == url.size(); ++i )
						if ( url[i] == ':' )
							col = i;
					
					std::string const protocol = url.substr(0,col);
					
					if ( protocol == "ftp" || protocol == "http" || protocol == "https" )
					{
						libmaus::aio::InputStream::shared_ptr_type tptr(factory->constructShared(url));
						return tptr;
					}
					else
					{
						libmaus::aio::InputStream::shared_ptr_type tptr(factory->constructShared(url.substr(protocol.size()+1)));
						return tptr;
					}
				}
				else
				{
					libmaus::aio::InputStream::shared_ptr_type tptr(factory->constructShared(url));
					return tptr;
				}
			}
			
			static bool tryOpen(std::string const & url)
			{
				try
				{
					libmaus::aio::InputStream::shared_ptr_type tptr(constructShared(url));
					return true;
				}
				catch(...)
				{
					return false;
				}
			}
			
			static void addHandler(std::string const & protocol, libmaus::aio::InputStreamFactory::shared_ptr_type factory)
			{
				factories[protocol] = factory;			
			}
		};	
	}
}
#endif
