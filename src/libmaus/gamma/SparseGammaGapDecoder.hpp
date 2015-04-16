/*
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
*/
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPDECODER_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPDECODER_HPP

#include <libmaus/gamma/GammaEncoder.hpp>
#include <libmaus/gamma/GammaDecoder.hpp>
#include <libmaus/aio/SynchronousGenericInput.hpp>
#include <libmaus/util/shared_ptr.hpp>

namespace libmaus
{
	namespace gamma
	{
		struct SparseGammaGapDecoder
		{
			typedef SparseGammaGapDecoder this_type;
			
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef libmaus::aio::SynchronousGenericInput<uint64_t> stream_type;
			
			libmaus::aio::SynchronousGenericInput<uint64_t> SGI;
			libmaus::gamma::GammaDecoder<stream_type> gdec;
			std::pair<uint64_t,uint64_t> p;
			
			struct iterator
			{
				SparseGammaGapDecoder * owner;
				uint64_t v;
				
				iterator()
				: owner(0), v(0)
				{
				
				}
				iterator(SparseGammaGapDecoder * rowner)
				: owner(rowner), v(owner->decode())
				{
				
				}
				
				uint64_t operator*() const
				{
					return v;
				}
				
				iterator operator++(int)
				{
					iterator copy = *this;
					v = owner->decode();
					return copy;
				}
				
				iterator operator++()
				{
					v = owner->decode();
					return *this;
				}
			};
		
			SparseGammaGapDecoder(std::istream & rstream) : SGI(rstream,64*1024), gdec(SGI), p(0,0)
			{
				p.first = gdec.decode();
				p.second = gdec.decode();				
			}
			
			uint64_t decode()
			{
				// no more non zero values
				if ( !p.second )
					return 0;
				
				// zero value
				if ( p.first )
				{
					p.first -= 1;
					return 0;
				}
				// non zero value
				else
				{
					uint64_t const retval = p.second;

					// get information about next non zero value
					p.first = gdec.decode();
					p.second = gdec.decode();
					
					return retval;
				}
			}			
			
			iterator begin()
			{
				return iterator(this);
			}
		};	
	}
}
#endif
