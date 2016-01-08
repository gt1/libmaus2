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

#if ! defined(LIBMAUS2_GAMMA_SIMPLEGAMMAENCODER_HPP)
#define LIBMAUS2_GAMMA_SIMPLEGAMMAENCODER_HPP

#include <libmaus2/gamma/SimpleGammaEncoderData.hpp>
#include <libmaus2/gamma/GammaEncoder.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type>
		struct SimpleGammaEncoder :
			public SimpleGammaEncoderData<_data_type>,
			public GammaEncoder< typename SimpleGammaEncoderData<_data_type>::stream_type >
		{
			typedef _data_type data_type;
			typedef SimpleGammaEncoder<data_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			SimpleGammaEncoder(std::string const & fn, uint64_t const bs = 8*1024)
			: SimpleGammaEncoderData<_data_type>(fn,bs),
			  GammaEncoder< typename SimpleGammaEncoderData<_data_type>::stream_type >(SimpleGammaEncoderData<_data_type>::SGO) {}

			SimpleGammaEncoder(std::ostream & out, uint64_t const bs = 8*1024)
			: SimpleGammaEncoderData<_data_type>(out,bs),
			  GammaEncoder< typename SimpleGammaEncoderData<_data_type>::stream_type >(SimpleGammaEncoderData<_data_type>::SGO) {}

			~SimpleGammaEncoder()
			{
				flush();
			}

			void flush()
			{
				GammaEncoder< typename SimpleGammaEncoderData<_data_type>::stream_type >::flush();
				SimpleGammaEncoderData<_data_type>::flush();
			}
		};
	}
}
#endif
