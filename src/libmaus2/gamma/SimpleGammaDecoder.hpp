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
#if ! defined(LIBMAUS2_GAMMA_SIMPLEGAMMADECODER_HPP)
#define LIBMAUS2_GAMMA_SIMPLEGAMMADECODER_HPP

#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/gamma/SimpleGammaDecoderData.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type>
		struct SimpleGammaDecoder :
			SimpleGammaDecoderData<_data_type>,
			GammaDecoder<typename SimpleGammaDecoderData<_data_type>::stream_type>
		{
			typedef _data_type data_type;
			typedef SimpleGammaDecoder<data_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			SimpleGammaDecoder(std::string const & fn, uint64_t const bs = 4*1024)
			: SimpleGammaDecoderData<_data_type>(fn,bs), GammaDecoder<typename SimpleGammaDecoderData<_data_type>::stream_type>(SimpleGammaDecoderData<_data_type>::SGI) {}
			SimpleGammaDecoder(std::istream & in, uint64_t const bs = 4*1024)
			: SimpleGammaDecoderData<_data_type>(in,bs), GammaDecoder<typename SimpleGammaDecoderData<_data_type>::stream_type>(SimpleGammaDecoderData<_data_type>::SGI) {}
		};
	}
}
#endif
