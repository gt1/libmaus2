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
#if ! defined(LIBMAUS2_GAMMA_GAMMADIFFERENCEDECODER_HPP)
#define LIBMAUS2_GAMMA_GAMMADIFFERENCEDECODER_HPP

#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type>
		struct GammaDifferenceDecoder
		{
			typedef _data_type data_type;
			typedef GammaDifferenceDecoder<data_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			libmaus2::aio::InputStreamInstance::unique_ptr_type Pin;
			std::istream & in;

			int64_t prim;
			int64_t prev;
			uint64_t n;

			typename libmaus2::aio::SynchronousGenericInput<data_type>::unique_ptr_type Sin;
			typename libmaus2::gamma::GammaDecoder< libmaus2::aio::SynchronousGenericInput<data_type> >::unique_ptr_type Gdec;

			void setup()
			{
				in.clear();
				in.seekg(-8,std::ios::end);
				uint64_t const indexpos = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				in.clear();
				in.seekg(indexpos,std::ios::beg);
				n = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				prim = libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in);
				prev = prim;
				in.clear();
				in.seekg(0,std::ios::beg);

				typename libmaus2::aio::SynchronousGenericInput<data_type>::unique_ptr_type Tin(new libmaus2::aio::SynchronousGenericInput<data_type>(in,8*1024,std::numeric_limits<uint64_t>::max(),false /* check mod */));
				Sin = UNIQUE_PTR_MOVE(Tin);

				typename libmaus2::gamma::GammaDecoder< libmaus2::aio::SynchronousGenericInput<data_type> >::unique_ptr_type Tdec(
					new libmaus2::gamma::GammaDecoder< libmaus2::aio::SynchronousGenericInput<data_type> >(*Sin)
				);
				Gdec = UNIQUE_PTR_MOVE(Tdec);
			}

			public:
			GammaDifferenceDecoder(std::istream & rin) : Pin(), in(rin)
			{
				setup();
			}

			GammaDifferenceDecoder(std::string const & rfn) : Pin(new libmaus2::aio::InputStreamInstance(rfn)), in(*Pin)
			{
				setup();
			}

			bool decode(data_type & v)
			{
				if ( n )
				{
					data_type dif = Gdec->decode() + 1;
					v = prev + dif;
					prev = v;
					n -= 1;
					return true;
				}
				else
				{
					Gdec.reset();
					Sin.reset();
					Pin.reset();
					return false;
				}
			}
		};
	}
}
#endif
