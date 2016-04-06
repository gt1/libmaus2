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
#if ! defined(LIBMAUS2_UTIL_DESTRUCTABLE_HPP)
#define LIBMAUS2_UTIL_DESTRUCTABLE_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>

namespace libmaus2
{
	namespace util
	{
		struct Destructable
		{
			typedef Destructable this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			void * object;
			void (*destruct)(void *);

			Destructable(void * robject, void (*rdestruct)(void *)) : object(robject), destruct(rdestruct) {}

			public:
			static unique_ptr_type construct(void * robject, void (*rdestruct)(void *))
			{
				unique_ptr_type tptr(new Destructable(robject,rdestruct));
				return UNIQUE_PTR_MOVE(tptr);
			}

			virtual ~Destructable()
			{
				if ( destruct )
				{
					destruct(object);
					object = 0;
				}
			}

			void * getObject()
			{
				return object;
			}
		};
	}
}
#endif
