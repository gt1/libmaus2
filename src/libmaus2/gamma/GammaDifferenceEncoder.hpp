/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_GAMMA_GAMMADIFFERENCEENCODER_HPP)
#define LIBMAUS2_GAMMA_GAMMADIFFERENCEENCODER_HPP

#include <libmaus2/gamma/GammaEncoder.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type>
		struct GammaDifferenceEncoderNumberCast
		{
			static int64_t numberCast(_data_type const v)
			{
				return static_cast<int64_t>(v);
			}
		};

		template<size_t _k>
		struct GammaDifferenceEncoderNumberCast< libmaus2::math::UnsignedInteger<_k> >
		{
			static int64_t numberCast(libmaus2::math::UnsignedInteger<_k> const v)
			{
				if ( _k == 0 )
					return 0;
				else if ( _k == 1 )
					return v[0];
				else
					return
						(static_cast<uint64_t>(v[1]) << 32)
						|
						(static_cast<uint64_t>(v[0]) <<  0);

			}
		};

		template<typename _data_type, int mindif = 1>
		struct GammaDifferenceEncoder
		{
			typedef _data_type data_type;
			typedef GammaDifferenceEncoder<data_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::aio::OutputStreamInstance::unique_ptr_type Pout;
			std::ostream & out;
			int64_t prim;
			int64_t prev;
			uint64_t n;

			typename libmaus2::aio::SynchronousGenericOutput<data_type>::unique_ptr_type Sout;
			typename libmaus2::gamma::GammaEncoder< libmaus2::aio::SynchronousGenericOutput<data_type> >::unique_ptr_type Genc;

			GammaDifferenceEncoder(std::ostream & rout, int64_t const rprim = -1)
			: Pout(), out(rout), prim(rprim), prev(prim), n(0), Sout(new libmaus2::aio::SynchronousGenericOutput<data_type>(out,8*1024)),
			  Genc(new libmaus2::gamma::GammaEncoder< libmaus2::aio::SynchronousGenericOutput<data_type> >(*Sout))
			{

			}

			GammaDifferenceEncoder(std::string const & rfn, int64_t const rprim = -1)
			: Pout(new libmaus2::aio::OutputStreamInstance(rfn)), out(*Pout), prim(rprim), prev(prim), n(0), Sout(new libmaus2::aio::SynchronousGenericOutput<data_type>(out,8*1024)),
			  Genc(new libmaus2::gamma::GammaEncoder< libmaus2::aio::SynchronousGenericOutput<data_type> >(*Sout))
			{

			}

			~GammaDifferenceEncoder()
			{
				flush();
			}

			void encode(int64_t const v)
			{
				assert ( v > prev );
				int64_t const dif = v - prev;
				int64_t const difenc = dif-static_cast<int64_t>(mindif);
				Genc->encode(difenc);
				prev = v;
				n += 1;
			}

			void encode(data_type const v)
			{
				encode(GammaDifferenceEncoderNumberCast<data_type>::numberCast(v));
			}

			void flush()
			{
				if ( Genc )
				{
					Genc->flush();
					Genc.reset();
					Sout->flush();
					uint64_t const indexoff = Sout->getWrittenWords() * sizeof(data_type);
					Sout.reset();
					libmaus2::util::NumberSerialisation::serialiseNumber(out,n);
					libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,prim);
					libmaus2::util::NumberSerialisation::serialiseNumber(out,indexoff);
					out.flush();
					Pout.reset();
				}
			}
		};
	}
}
#endif
