/*
    libmaus2
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
#if ! defined(LIBMAUS2_GAMMA_SPARSEGAMMAGAPDECODER_HPP)
#define LIBMAUS2_GAMMA_SPARSEGAMMAGAPDECODER_HPP

#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/math/UnsignedInteger.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type>
		struct SparseGammaGapDecoderNumberCast
		{
			static uint64_t cast(_data_type const v)
			{
				return static_cast<uint64_t>(v);
			}
		};

		template<size_t k>
		struct SparseGammaGapDecoderNumberCast< libmaus2::math::UnsignedInteger<k> >
		{
			static uint64_t cast(libmaus2::math::UnsignedInteger<k> const v)
			{
				if ( !k )
					return 0;
				else if ( k == 1 )
					return v[0];
				else
					return
						(
							static_cast<uint64_t>(v[1]) << 32
						)
						|
						(
							static_cast<uint64_t>(v[0])
						);
			}
		};

		template<typename _data_type>
		struct SparseGammaGapDecoderTemplate
		{
			typedef _data_type data_type;
			typedef SparseGammaGapDecoderTemplate<data_type> this_type;

			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef libmaus2::aio::SynchronousGenericInput<data_type> stream_type;

			stream_type SGI;
			libmaus2::gamma::GammaDecoder<stream_type> gdec;
			std::pair<data_type,data_type> p;

			struct iterator
			{
				this_type * owner;
				uint64_t v;

				iterator()
				: owner(0), v(0)
				{

				}
				iterator(this_type * rowner)
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

			SparseGammaGapDecoderTemplate(std::istream & rstream) : SGI(rstream,64*1024), gdec(SGI), p(0,0)
			{
				p.first = gdec.decode();
				p.second = gdec.decode();
			}

			uint64_t decode()
			{
				// no more non zero values
				if ( ! GammaDecoderBase<data_type>::isNonNull(p.second) )
					return 0;

				// zero value
				if ( GammaDecoderBase<data_type>::isNonNull(p.first) )
				{
					p.first -= 1;
					return 0;
				}
				// non zero value
				else
				{
					uint64_t const retval = SparseGammaGapDecoderNumberCast<data_type>::cast(p.second);

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

		typedef SparseGammaGapDecoderTemplate<uint64_t> SparseGammaGapDecoder;
		typedef SparseGammaGapDecoderTemplate< libmaus2::math::UnsignedInteger<4> > SparseGammaGapDecoder2;
	}
}
#endif
