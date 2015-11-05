/*
    libmaus2
    Copyright (C) 2011-2014 Genome Research Limited
    Copyright (C) 2009-2015 German Tischler

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
#if ! defined(LIBMAUS2_SUFFIXSORT_BWTMERGEBLOCKSORTREQUESTBASE_HPP)
#define LIBMAUS2_SUFFIXSORT_BWTMERGEBLOCKSORTREQUESTBASE_HPP

#include <libmaus2/suffixsort/BwtMergeEnumBase.hpp>
#include <libmaus2/util/KMP.hpp>

namespace libmaus2
{
	namespace suffixsort
	{
		struct BwtMergeBlockSortRequestBase : public libmaus2::suffixsort::BwtMergeEnumBase
		{
			template<typename input_types_type>
			static uint64_t findSplitCommon(
				std::string const & fn,
				// position of textblock
				uint64_t const t,
				// length of textblock
				uint64_t const n,
				// position of pattern
				uint64_t const p,
				// length of file
				uint64_t const m
			)
			{
				typedef typename input_types_type::base_input_stream base_input_stream;
				typedef typename input_types_type::circular_wrapper circular_wrapper;

				circular_wrapper textstr(fn,t);
				circular_wrapper patstr(fn,p);

				// dynamically growing best prefix table
				::libmaus2::util::KMP::BestPrefix<base_input_stream> BP(patstr,m);
				// adapter for accessing pattern in BP
				typename ::libmaus2::util::KMP::BestPrefix<base_input_stream>::BestPrefixXAdapter xadapter = BP.getXAdapter();
				// call KMP adaption
				std::pair<uint64_t, uint64_t> Q = ::libmaus2::util::KMP::PREFIX_SEARCH_INTERNAL_RESTRICTED(
					// pattern
					xadapter,m,BP,
					// text
					textstr,m,
					// restriction for position
					n
				);

				return Q.second;
			}

			template<typename input_types_type>
			static uint64_t findSplitCommonBounded(
				std::string const & fn,
				// position of textblock
				uint64_t const t,
				// length of textblock
				uint64_t const n,
				// position of pattern
				uint64_t const p,
				// length of file
				uint64_t const m,
				// bound
				uint64_t const bound
			)
			{
				typedef typename input_types_type::base_input_stream base_input_stream;
				typedef typename input_types_type::circular_wrapper circular_wrapper;

				circular_wrapper textstr(fn,t);
				circular_wrapper patstr(fn,p);

				// dynamically growing best prefix table
				::libmaus2::util::KMP::BestPrefix<base_input_stream> BP(patstr,m);
				// adapter for accessing pattern in BP
				typename ::libmaus2::util::KMP::BestPrefix<base_input_stream>::BestPrefixXAdapter xadapter = BP.getXAdapter();
				// call KMP adaption
				std::pair<uint64_t, uint64_t> Q = ::libmaus2::util::KMP::PREFIX_SEARCH_INTERNAL_RESTRICTED_BOUNDED(
					// pattern
					xadapter,m,BP,
					// text
					textstr,m,
					// restriction for position
					n,
					// bound on length
					bound
				);

				return Q.second;
			}
		};
	}
}
#endif
