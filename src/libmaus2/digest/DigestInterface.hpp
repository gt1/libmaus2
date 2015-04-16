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
#if ! defined(LIBMAUS_DIGEST_DIGESTINTERFACE_HPP)
#define LIBMAUS_DIGEST_DIGESTINTERFACE_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/types/types.hpp>
#include <sstream>
#include <iomanip>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace digest
	{
		struct DigestInterface
		{
			typedef DigestInterface this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			virtual ~DigestInterface() {}
			virtual void digest(uint8_t * digest) = 0;
			virtual void vinit() = 0;
			virtual void vupdate(uint8_t const *, size_t) = 0;			
			virtual size_t vdigestlength() = 0;

			virtual std::string vdigestAsString()
			{
				size_t const digestlength = vdigestlength();
				libmaus2::autoarray::AutoArray<uint8_t> D(digestlength,false);
				digest(D.begin());
				return vdigestToString(D.begin());
			}

			virtual std::string vdigestToString(uint8_t const * D)
                        {
                        	size_t const digestlength = vdigestlength();
                        	std::ostringstream ostr;
				for ( uint64_t i = 0; i < digestlength; ++i )
					ostr << std::hex  << std::setfill('0') << std::setw(2) << static_cast<int>(D[i]) << std::dec << std::setw(0);
				return ostr.str();
			}                                                                                                                                                                                 
		};
	}
}
#endif
