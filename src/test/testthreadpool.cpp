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
#include <libmaus2/parallel/ThreadPool.hpp>

namespace libmaus2
{
	namespace parallel
	{
		struct DummyThreadWorkPackageMeta
		{
			uint64_t unfinished;
			uint64_t finished;

			DummyThreadWorkPackageMeta() : unfinished(0), finished(0)
			{

			}
		};

		struct DummyThreadWorkPackage : public ThreadWorkPackage
		{
			libmaus2::parallel::PosixMutex::shared_ptr_type mutex;
			DummyThreadWorkPackageMeta * meta;

			DummyThreadWorkPackage() : mutex(), meta(0) {}
			DummyThreadWorkPackage(
				uint64_t const rpriority,
				uint64_t const rdispatcherid,
				libmaus2::parallel::PosixMutex::shared_ptr_type rmutex,
				DummyThreadWorkPackageMeta * rmeta,
				uint64_t const rpackageid = 0
			)
			: ThreadWorkPackage(rpriority,rdispatcherid,rpackageid), mutex(rmutex), meta(rmeta)
			{

			}

			virtual ThreadWorkPackage::unique_ptr_type uclone() const
			{
				ThreadWorkPackage::unique_ptr_type tptr(new DummyThreadWorkPackage(priority,dispatcherid,mutex,meta,packageid));
				return UNIQUE_PTR_MOVE(tptr);
			}
			virtual ThreadWorkPackage::shared_ptr_type sclone() const
			{
				ThreadWorkPackage::shared_ptr_type sptr(new DummyThreadWorkPackage(priority,dispatcherid,mutex,meta,packageid));
				return sptr;
			}
		};

		struct DummyThreadWorkPackageDispatcher : public ThreadWorkPackageDispatcher
		{
			virtual ~DummyThreadWorkPackageDispatcher() {}
			virtual void dispatch(ThreadWorkPackage * P, ThreadPoolInterfaceEnqueTermInterface & tpi)
			{
				DummyThreadWorkPackage * DP = dynamic_cast<DummyThreadWorkPackage *>(P);
				assert ( DP );

				libmaus2::parallel::ScopePosixMutex mutex(*(DP->mutex));
				std::cerr << DP << std::endl;

				if ( DP->meta->unfinished < 1024 )
					tpi.enque(libmaus2::parallel::DummyThreadWorkPackage(
						DP->meta->unfinished++,DP->dispatcherid,DP->mutex,DP->meta)
					);
				if ( DP->meta->unfinished < 1024 )
					tpi.enque(libmaus2::parallel::DummyThreadWorkPackage(
						DP->meta->unfinished++,DP->dispatcherid,DP->mutex,DP->meta)
					);

				if ( ++DP->meta->finished == 1024 )
					tpi.terminate();
			}
		};
	}
}

int main()
{
	libmaus2::parallel::ThreadPool TP(8);

	uint64_t const dispid = 0;

	libmaus2::parallel::DummyThreadWorkPackageDispatcher dummydisp;
	TP.registerDispatcher(dispid,&dummydisp);

	libmaus2::parallel::DummyThreadWorkPackageMeta meta;
	libmaus2::parallel::PosixMutex::shared_ptr_type printmutex(new libmaus2::parallel::PosixMutex);

	TP.enque(libmaus2::parallel::DummyThreadWorkPackage(0,dispid,printmutex,&meta));

	TP.join();
}
