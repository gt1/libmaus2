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
#if !defined(LIBMAUS2_LZ_BGZFINFLATDECOMPRESSPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_LZ_BGZFINFLATDECOMPRESSPACKAGEDISPATCHER_HPP

#include <libmaus2/lz/BgzfInflateReadPackageDispatcher.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateDecompressPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
		{
			typedef BgzfInflateDecompressPackageDispatcher this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			struct BgzfInflateDecompressPackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef BgzfInflateDecompressPackage this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult BIRP;

				BgzfInflateDecompressPackage() : SimpleThreadWorkPackage() {}
				BgzfInflateDecompressPackage(
					libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult rBIRP,
					uint64_t const rpriority,
					uint64_t const rdispatcherid,
					uint64_t const rpackageid = 0,
					uint64_t const rsubid = 0
				)
				: SimpleThreadWorkPackage(rpriority,rdispatcherid,rpackageid,rsubid), BIRP(rBIRP) {}

				char const * getPackageName() const
				{
					return "BgzfInflateDecompressPackage";
				}
			};

			struct BgzfInflateDecompressPackageReturn
			{
				virtual ~BgzfInflateDecompressPackageReturn() {}
				virtual void bgzfInflateDecompressPackageReturn(BgzfInflateDecompressPackage * package) = 0;
			};

			struct BgzfInflateDecompressPackageFinished
			{
				virtual ~BgzfInflateDecompressPackageFinished() {}
				virtual void bgzfInflateDecompressPackageFinished(libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult res) = 0;
			};


			BgzfInflateDecompressPackageReturn * packret;
			BgzfInflateDecompressPackageFinished * fin;

			BgzfInflateDecompressPackageDispatcher(
				BgzfInflateDecompressPackageReturn * rpackret,
				BgzfInflateDecompressPackageFinished * rfin
			)
			: libmaus2::parallel::SimpleThreadWorkPackageDispatcher(), packret(rpackret), fin(rfin)
			{}

			void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
			{
				BgzfInflateDecompressPackage * BIRP = dynamic_cast<BgzfInflateDecompressPackage *>(P);
				libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult res = BIRP->BIRP;
				assert ( BIRP );

				res.ubytes = BIRP->BIRP.block->decompressBlock(res.ablock->begin(),res.BBI);

				fin->bgzfInflateDecompressPackageFinished(res);
				packret->bgzfInflateDecompressPackageReturn(BIRP);
			}
		};
	}
}
#endif
