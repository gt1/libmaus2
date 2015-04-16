/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS2_LZ_BGZFINFLATEPARALLELWRAPPER_HPP)
#define LIBMAUS2_LZ_BGZFINFLATEPARALLELWRAPPER_HPP

#include <libmaus2/lz/BgzfInflateParallel.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateParallelWrapper
		{
			BgzfInflateParallel bgzf;
			
			BgzfInflateParallelWrapper(std::istream & rin)
			: bgzf(rin) {}
			BgzfInflateParallelWrapper(std::istream & rin, uint64_t const rnumthreads)
			: bgzf(rin,rnumthreads) {}
			BgzfInflateParallelWrapper(std::istream & rin, uint64_t const rnumthreads, uint64_t const rnumblocks)
			: bgzf(rin,rnumthreads,rnumblocks) {}

			BgzfInflateParallelWrapper(std::istream & rin, std::ostream & out)
			: bgzf(rin,out) {}
			BgzfInflateParallelWrapper(std::istream & rin, std::ostream & out, uint64_t const rnumthreads)
			: bgzf(rin,out,rnumthreads) {}
			BgzfInflateParallelWrapper(std::istream & rin, std::ostream & out, uint64_t const rnumthreads, uint64_t const rnumblocks)
			: bgzf(rin,out,rnumthreads,rnumblocks) {}
		};
	}
}
#endif
