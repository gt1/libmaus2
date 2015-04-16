/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_LZ_ZLIBCOMPRESSOROBJECTFACTORY_HPP)
#define LIBMAUS_LZ_ZLIBCOMPRESSOROBJECTFACTORY_HPP

#include <libmaus/lz/ZlibCompressorObject.hpp>
#include <libmaus/lz/CompressorObjectFactory.hpp>

namespace libmaus
{
	namespace lz
	{
		struct ZlibCompressorObjectFactory : public CompressorObjectFactory
		{
			typedef ZlibCompressorObjectFactory this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			int const level;
			
			ZlibCompressorObjectFactory(int const rlevel = Z_DEFAULT_COMPRESSION)
			: level(rlevel) {}
			virtual ~ZlibCompressorObjectFactory() {}
			virtual CompressorObject::unique_ptr_type operator()()
			{
				CompressorObject::unique_ptr_type ptr(new ZlibCompressorObject(level));
				return UNIQUE_PTR_MOVE(ptr);
			}
			virtual std::string getDescription() const
			{
				std::ostringstream ostr;
				ostr << "ZlibCompressorObjectFactory(" << level << ")";
				return ostr.str();
			}
		};
	}
}
#endif
