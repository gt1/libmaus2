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

#if ! defined(LIBMAUS_AIO_PUTOUTPUTITERATOR_HPP)
#define LIBMAUS_AIO_PUTOUTPUTITERATOR_HPP

#include <iterator>
                    
namespace libmaus
{
	namespace aio
	{
                template<typename _data_type, typename _owner_type>
                struct PutOutputIterator : public ::std::iterator< ::std::output_iterator_tag, _data_type>
                {
                	typedef _data_type data_type;
                	typedef _owner_type owner_type;
                	typedef PutOutputIterator<data_type,owner_type> this_type;

			typedef ::std::output_iterator_tag iterator_category;
			typedef typename ::std::iterator< ::std::output_iterator_tag, data_type>::value_type value_type;
			typedef typename ::std::iterator< ::std::output_iterator_tag, data_type>::difference_type difference_type;
			typedef typename ::std::iterator< ::std::output_iterator_tag, data_type>::reference reference;
			typedef typename ::std::iterator< ::std::output_iterator_tag, data_type>::pointer pointer;
                
                	owner_type * SGOP;

			PutOutputIterator() : SGOP(0) {}
                	PutOutputIterator(owner_type & rSGOP) : SGOP(&rSGOP) {}
                	~PutOutputIterator() {}

                	this_type & operator*() { return *this; }
                	this_type & operator++() { return *this; }
                	this_type & operator++(int) { return *this; }
                	
                	this_type & operator=(data_type const & v) { SGOP->put(v); return *this; }
                };                
	}
}
#endif
