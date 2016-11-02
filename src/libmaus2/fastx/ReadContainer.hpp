/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_FASTX_READCONTAINER_HPP)
#define LIBMAUS2_FASTX_READCONTAINER_HPP

#include <libmaus2/fastx/LineBufferFastAReader.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct ReadContainer
		{
			struct ReadInfo
			{
				uint64_t nameoff;
				uint64_t dataoff;
				uint64_t rlen;

				ReadInfo()
				{
				}

				ReadInfo(uint64_t const rnameoff, uint64_t const rdataoff, uint64_t const rrlen) : nameoff(rnameoff), dataoff(rdataoff), rlen(rrlen) {}
			};

			libmaus2::autoarray::AutoArray<char> Adata;
			libmaus2::autoarray::AutoArray<ReadInfo> Aoff;
			uint64_t numreads;

			ReadContainer(std::string const & fn) : numreads(0)
			{
				libmaus2::aio::InputStreamInstance ISI(fn);
				libmaus2::fastx::LineBufferFastAReader LBFAR(ISI);

				uint64_t o = 0;
				uint64_t o_name = 0;
				uint64_t o_data = 0;
				uint64_t rlen = 0;

				while ( LBFAR.getNext(Adata,o,o_name,o_data,rlen,true /* term data */,false /* map data */,false /* pad data */, 0 /* padsym */, true /* add rc */) )
					Aoff.push(numreads,ReadInfo(o_name,o_data,rlen));
			}

			uint64_t getReadLength(uint64_t const id) const
			{
				return Aoff[id].rlen;
			}

			char const * getForwardRead(uint64_t const id) const
			{
				return Adata.begin() + Aoff[id].dataoff;
			}

			char const * getReverseComplementRead(uint64_t const id) const
			{
				return Adata.begin() + Aoff[id].dataoff + Aoff[id].rlen + 1 /* term */;
			}
		};
	}
}
#endif
