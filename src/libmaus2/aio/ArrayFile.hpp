/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_AIO_ARRAYFILE_HPP)
#define LIBMAUS2_AIO_ARRAYFILE_HPP

#include <libmaus2/aio/ArrayInputStream.hpp>
#include <libmaus2/aio/ArrayFileContainer.hpp>
#include <libmaus2/aio/ArrayInputStreamFactory.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace aio
	{
		template<typename iterator>
		struct ArrayFile
		{
			private:
			// array
			iterator ita;
			iterator ite;
			// protocol
			std::string const prot;
			// container
			libmaus2::aio::ArrayFileContainer<iterator> container;
			// url
			std::string const url;

			static std::string pointerToString(void * vp)
			{
				std::ostringstream protstr;
				protstr << vp;
				std::string prot = protstr.str();
				assert ( prot.size() >= 2 && prot.substr(0,2) == "0x" );
				prot = prot.substr(2);
				for ( uint64_t i = 0; i < prot.size(); ++i )
					if ( ::std::isalpha(prot[i]) )
					{
						prot[i] = ::std::tolower(prot[i]);
						assert ( prot[i] >= 'a' );
						assert ( prot[i] <= 'f' );
						char const dig = prot[i] - 'a' + 10;
						prot[i] = dig + 'a';
					}
					else
					{
						assert ( ::std::isdigit(prot[i]) );
						char const dig = prot[i] - '0';
						prot[i] = dig + 'a';
					}
				return prot;
			}

			public:
			ArrayFile(iterator rita, iterator rite)
			: ita(rita), ite(rite), prot(std::string("array") + pointerToString(this)), container(), url(prot + "file")
			{
				// add file
				container.add("file",ita,ite);
				// set up factory
				typename libmaus2::aio::ArrayInputStreamFactory<iterator>::shared_ptr_type factory(
					new libmaus2::aio::ArrayInputStreamFactory<iterator>(container));
				// add protocol handler
				libmaus2::aio::InputStreamFactoryContainer::addHandler(prot, factory);
			}
			
			~ArrayFile()
			{
				libmaus2::aio::InputStreamFactoryContainer::removeHandler(prot);	
			}
			
			libmaus2::aio::InputStreamInstance::unique_ptr_type open() const
			{
				libmaus2::aio::InputStreamInstance::unique_ptr_type tptr(new libmaus2::aio::InputStreamInstance(url));
				return UNIQUE_PTR_MOVE(tptr);
			}
			
			std::string const & getURL() const
			{
				return url;
			}
		};
	}
}
#endif
