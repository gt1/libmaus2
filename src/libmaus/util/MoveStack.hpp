/**
    libmaus
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
**/

#if ! defined(MOVESTACK_HPP)
#define MOVESTACK_HPP

#include <libmaus/autoarray/AutoArray.hpp>

namespace libmaus
{
        namespace util
        {
                template<typename _type>
                struct MoveStack
                {
                        typedef _type type;
                        typedef typename type::unique_ptr_type type_ptr;
                        typedef ::libmaus::autoarray::AutoArray<type_ptr> array_type;
                        typedef typename array_type::unique_ptr_type array_ptr_type;
                        typedef MoveStack<type> this_type;
                        typedef typename ::libmaus::util::unique_ptr< this_type>::type unique_ptr_type;
                        
                        array_ptr_type array;
                        uint64_t fill;

                        private:
                        void expand(uint64_t const newsize)
                        {
                        	array_ptr_type newarray(new array_type(newsize));
                                for ( uint64_t i = 0; i < fill; ++i )
                                	(*newarray)[i] = UNIQUE_PTR_MOVE((*array)[i]);
				array = UNIQUE_PTR_MOVE(newarray);
                        }
                        
                        public:
                        MoveStack()
                        : array(new array_type(0)), fill(0)
                        {
                        }
                        
                        void reserve(uint64_t const size)
                        {
                        	assert ( size >= array->size() );
                        	expand(size);
                        }
                        
                        uint64_t push_back(type_ptr & ptr)
                        {
                                if ( fill == array->size() )
                                {
                                	uint64_t const newsize = std::max(2*fill,static_cast<uint64_t>(1));
					expand(newsize);
                                }
                                
                                uint64_t id = fill++;
                                (*array)[id] = UNIQUE_PTR_MOVE(ptr);

                                return id;
                        }
                        
                        void pop_back()
                        {
                        	if ( fill )
                        	{
                        		fill -= 1;
                        		(*array)[fill].reset();
                        	}
                        }
                        
                        type * back()
                        {
                        	assert ( fill );
                        	return (*array)[fill-1].get();
                        }
                        
                        uint64_t size() const
                        {
                        	return fill;
                        }
                        
                        type * operator[](uint64_t const i)
                        {
                                return (*array)[i].get();
                        }

                        type const * operator[](uint64_t const i) const
                        {
                                return (*array)[i].get();
                        }
                        
                        void reset(uint64_t i)
                        {
                                (*array)[i].reset();
                        }
                };
        }
}
#endif
