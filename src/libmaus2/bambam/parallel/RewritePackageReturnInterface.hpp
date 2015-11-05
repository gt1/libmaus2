/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_PARALLEL_REWRITEPACKAGERETURNINTERFACE_HPP)
#define LIBMAUS2_BAMBAM_PARALLEL_REWRITEPACKAGERETURNINTERFACE_HPP

#include <libmaus2/bambam/parallel/RewriteBlockWorkPackage.hpp>

namespace libmaus2
{
	namespace bambam
	{
		namespace parallel
		{
			// return a rewrite work package
			struct RewritePackageReturnInterface
			{
				virtual ~RewritePackageReturnInterface() {}
				virtual void putReturnRewritePackage(RewriteBlockWorkPackage * package) = 0;
			};
		}
	}
}
#endif
