/*
    libmaus2
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
#if ! defined(LIBMAUS2_SUFFIXSORT_GAPARRAYDECODERBUFFER_HPP)
#define LIBMAUS2_SUFFIXSORT_GAPARRAYDECODERBUFFER_HPP

#include <libmaus2/suffixsort/GapArrayByteDecoder.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		struct GapArrayByteDecoderBuffer
		{
			typedef GapArrayByteDecoderBuffer this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			GapArrayByteDecoder & decoder;
			libmaus2::autoarray::AutoArray<uint64_t> B;
			uint64_t * pa;
			uint64_t * pc;
			uint64_t * pe;

			GapArrayByteDecoderBuffer(
				GapArrayByteDecoder & rdecoder,
				uint64_t const bufsize
			) : decoder(rdecoder), B(bufsize), pa(B.begin()), pc(pa), pe(pa)
			{
				assert ( bufsize );
			}

			bool getNext(uint64_t & v)
			{
				if ( pc == pe )
				{
					if ( decoder.offset == decoder.gsize )
						return false;

					uint64_t const tocopy = std::min(static_cast<ptrdiff_t>(decoder.gsize - decoder.offset), static_cast<ptrdiff_t>(B.size()));

					decoder.decode(pa, tocopy);

					pc = pa;
					pe = pa + tocopy;

					assert ( pc != pe );
				}

				v = *(pc++);
				return true;
			}

			uint64_t get()
			{
				uint64_t v = 0;
				getNext(v);
				return v;
			}

			struct iterator
			{
				GapArrayByteDecoderBuffer * owner;
				uint64_t v;

				iterator()
				: owner(0), v(0)
				{

				}
				iterator(GapArrayByteDecoderBuffer * rowner)
				: owner(rowner), v(owner->get())
				{

				}

				uint64_t operator*() const
				{
					return v;
				}

				iterator operator++(int)
				{
					iterator copy = *this;
					v = owner->get();
					return copy;
				}

				iterator operator++()
				{
					v = owner->get();
					return *this;
				}
			};

			iterator begin()
			{
				return iterator(this);
			}
		};
	}
}
#endif
