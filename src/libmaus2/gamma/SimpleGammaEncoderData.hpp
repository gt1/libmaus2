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
#if ! defined(LIBMAUS2_GAMMA_SIMPLEGAMMAENCODERDATA_HPP)
#define LIBMAUS2_GAMMA_SIMPLEGAMMAENCODERDATA_HPP

#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type>
		struct SimpleGammaEncoderData
		{
			typedef _data_type data_type;
			typedef SimpleGammaEncoderData<data_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef libmaus2::aio::SynchronousGenericOutput<data_type> stream_type;

			libmaus2::aio::OutputStreamInstance::unique_ptr_type POSI;
			std::ostream & OSI;
			typename stream_type::unique_ptr_type PSGO;
			stream_type & SGO;

			SimpleGammaEncoderData(std::string const & fn, uint64_t const bs = 4*1024ull)
			: POSI(new libmaus2::aio::OutputStreamInstance(fn)), OSI(*POSI), PSGO(new stream_type(OSI,bs)), SGO(*PSGO) {}

			SimpleGammaEncoderData(std::ostream & out, uint64_t const bs = 4*1024ull)
			: POSI(), OSI(out), PSGO(new stream_type(OSI,bs)), SGO(*PSGO) {}

			~SimpleGammaEncoderData()
			{
				flush();
			}

			void flush()
			{
				SGO.flush();
				OSI.flush();
			}
		};
	}
}
#endif
