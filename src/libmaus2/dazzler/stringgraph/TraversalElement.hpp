/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_STRINGGRAPH_TRAVERSALELEMENT_HPP)
#define LIBMAUS2_DAZZLER_STRINGGRAPH_TRAVERSALELEMENT_HPP

#include <libmaus2/types/types.hpp>
#include <ostream>

namespace libmaus2
{
	namespace dazzler
	{
		namespace stringgraph
		{
			struct TraversalElement
			{
				uint64_t cb;
				uint64_t ce;
				uint64_t bread;
				uint64_t bb;
				uint64_t be;
				bool bi;

				TraversalElement() {}
				TraversalElement(
					uint64_t const rcb,
					uint64_t const rce,
					uint64_t const rbread,
					uint64_t const rbb,
					uint64_t const rbe,
					bool const rbi
				) : cb(rcb), ce(rce), bread(rbread), bb(rbb), be(rbe), bi(rbi)
				{

				}
			};

			inline std::ostream & operator<<(std::ostream & out, TraversalElement const & T)
			{
				out << "TraversalElement("
					<<  "cb=" << T.cb
					<< ",ce=" << T.ce
					<< ",bread=" << T.bread
					<< ",bb=" << T.bb
					<< ",be=" << T.be
					<< ",bi=" << T.bi
					<< ")";
				return out;
			}
		}
	}
}
#endif
