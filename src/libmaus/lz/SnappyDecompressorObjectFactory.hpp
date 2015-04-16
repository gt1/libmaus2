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
#if ! defined(LIBMAUS_LZ_SNAPPYDECOMPRESSOROBJECTFACTORY_HPP)
#define LIBMAUS_LZ_SNAPPYDECOMPRESSOROBJECTFACTORY_HPP

#include <libmaus/lz/DecompressorObjectFactory.hpp>
#include <libmaus/lz/SnappyDecompressorObject.hpp>

namespace libmaus
{
	namespace lz
	{
		struct SnappyDecompressorObjectFactory : public DecompressorObjectFactory
		{
			typedef SnappyDecompressorObjectFactory this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			virtual ~SnappyDecompressorObjectFactory() {}
			virtual DecompressorObject::unique_ptr_type operator()()
			{
				DecompressorObject::unique_ptr_type ptr(new SnappyDecompressorObject);
				return UNIQUE_PTR_MOVE(ptr);
			}
			virtual std::string getDescription() const
			{
				return "SnappyDecompressorObjectFactory";
			}
		};
	}
}
#endif
