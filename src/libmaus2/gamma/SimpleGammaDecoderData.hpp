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
#if ! defined(LIBMAUS2_GAMMA_SIMPLEGAMMADECODERDATA_HPP)
#define LIBMAUS2_GAMMA_SIMPLEGAMMADECODERDATA_HPP

#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type>
		struct SimpleGammaDecoderData
		{
			typedef _data_type data_type;
			typedef SimpleGammaDecoderData<data_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef libmaus2::aio::SynchronousGenericInput<data_type> stream_type;
			typedef typename stream_type::unique_ptr_type stream_ptr_type;

			libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
			std::istream & ISI;
			stream_ptr_type PSGI;
			stream_type & SGI;

			SimpleGammaDecoderData(std::string const & fn, uint64_t const bs)
			: PISI(new libmaus2::aio::InputStreamInstance(fn)), ISI(*PISI), PSGI(new stream_type(ISI,bs)), SGI(*PSGI)
			{

			}
			SimpleGammaDecoderData(std::istream & in, uint64_t const bs)
			: PISI(), ISI(in), PSGI(new stream_type(ISI,bs)), SGI(*PSGI)
			{

			}
		};
	}
}
#endif
