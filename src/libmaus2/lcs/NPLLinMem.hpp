/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if !defined(LIBMAUS2_LCS_NPLLINMEM_HPP)
#define LIBMAUS2_LCS_NPLLINMEM_HPP

#include <libmaus2/lcs/NPLNoTrace.hpp>
#include <libmaus2/lcs/NPLinMem.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct NPLLinMem : public libmaus2::lcs::NPLinMem
		{
			typedef NPLLinMem this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::lcs::NPLNoTrace nplnt;

			NPLLinMem() : nplnt() {}

			template<typename iter_a, typename iter_b>
			int64_t np(iter_a const a, iter_a const ae, iter_b const b, iter_b const be)
			{
				libmaus2::lcs::NPLNoTrace::ReturnValue const RV = nplnt.np(a,ae,b,be);
				return libmaus2::lcs::NPLinMem::np(a,a+RV.alen,b,b+RV.blen);
			}

                        void align(uint8_t const * a, size_t const l_a, uint8_t const * b, size_t const l_b)
                        {
                        	np(a,a+l_a,b,b+l_b);
                        }

			uint64_t byteSize() const
			{
				return libmaus2::lcs::NPLinMem::byteSize() + nplnt.byteSize();
			}
		};
	}
}
#endif
