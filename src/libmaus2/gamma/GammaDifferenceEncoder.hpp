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
				Genc->encode(dif-1);
				prev = v;
				n += 1;
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
