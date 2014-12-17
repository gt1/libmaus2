/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_BGZFLINEARMEMCOMPRESSWORKPACKAGE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_BGZFLINEARMEMCOMPRESSWORKPACKAGE_HPP

#include <libmaus/bambam/parallel/SmallLinearBlockCompressionPendingObject.hpp>
#include <libmaus/lz/BgzfDeflateOutputBufferBase.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackage.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{			
			struct BgzfLinearMemCompressWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
			{
				typedef BgzfLinearMemCompressWorkPackage this_type;
				typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

				SmallLinearBlockCompressionPendingObject obj;
				libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type outbuf;

				BgzfLinearMemCompressWorkPackage() : obj(), outbuf() {}

				BgzfLinearMemCompressWorkPackage(
					uint64_t const rpriority, 
					SmallLinearBlockCompressionPendingObject robj,
					libmaus::lz::BgzfDeflateOutputBufferBase::shared_ptr_type routbuf,
					uint64_t const rlinearMemCompressDispatcherId
				) : libmaus::parallel::SimpleThreadWorkPackage(rpriority,rlinearMemCompressDispatcherId), obj(robj), outbuf(routbuf)
				{
				
				}

				char const * getPackageName() const
				{
					return "BgzfLinearMemCompressWorkPackage";
				}
			};
		}
	}
}
#endif
