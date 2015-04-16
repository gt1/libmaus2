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
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SynchronousCounter.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>

namespace libmaus
{
	namespace parallel
	{
		struct DummyThreadWorkPackageMeta;
				
		struct DummyThreadWorkPackage : public SimpleThreadWorkPackage
		{
			typedef DummyThreadWorkPackage this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			libmaus::parallel::PosixMutex * mutex;
			
			DummyThreadWorkPackageMeta * meta;
		
			DummyThreadWorkPackage() : mutex(0), meta(0) {}
			DummyThreadWorkPackage(
				uint64_t const rpriority, 
				uint64_t const rdispatcherid, 
				libmaus::parallel::PosixMutex * rmutex,
				DummyThreadWorkPackageMeta * rmeta,
				uint64_t const rpackageid = 0
			)
			: SimpleThreadWorkPackage(rpriority,rdispatcherid,rpackageid), mutex(rmutex), meta(rmeta)
			{
			
			}			

			virtual char const * getPackageName() const
			{
				return "DummyThreadWorkPackage";
			}
		};
		
		struct DummyThreadWorkPackageMeta
		{
			libmaus::parallel::PosixMutex lock;

			SimpleThreadPoolWorkPackageFreeList<DummyThreadWorkPackage> freelist;
			libmaus::parallel::SynchronousCounter<uint64_t> finished;
			
			DummyThreadWorkPackageMeta() : finished(0)
			{
			
			}			
		};

		struct DummyThreadWorkPackageDispatcher : public SimpleThreadWorkPackageDispatcher
		{
			virtual ~DummyThreadWorkPackageDispatcher() {}
			virtual void dispatch(
				SimpleThreadWorkPackage * P, 
				SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				DummyThreadWorkPackage * DP = dynamic_cast<DummyThreadWorkPackage *>(P);
				assert ( DP );
				DummyThreadWorkPackageMeta * meta = DP->meta;

				static const uint64_t numpacks = 1024;
				static const uint64_t spawn = 3;

				try
				{
					{
					libmaus::parallel::ScopePosixMutex mutex(*(DP->mutex));
					std::cerr << DP << std::endl;
					}

					for ( uint64_t i = 1; i <= spawn; ++i )
					{
						if ( spawn*DP->packageid+i < numpacks )
						{
							libmaus::parallel::DummyThreadWorkPackage * pack0 = meta->freelist.getPackage();
							*pack0 = libmaus::parallel::DummyThreadWorkPackage(
								DP->priority,
								DP->dispatcherid,
								DP->mutex,
								DP->meta
							);
							tpi.enque(pack0);
						}
					}
				}
				catch(...)
				{
				
				}
				
				if ( ++ meta->finished == numpacks )
					tpi.terminate();
				
				meta->freelist.returnPackage(DP);
			}
		};

		struct DummyThreadWorkPackageRandomExceptionDispatcher : public SimpleThreadWorkPackageDispatcher
		{
			virtual ~DummyThreadWorkPackageRandomExceptionDispatcher() {}
			virtual void dispatch(
				SimpleThreadWorkPackage * P, 
				SimpleThreadPoolInterfaceEnqueTermInterface & tpi
			)
			{
				DummyThreadWorkPackage * DP = dynamic_cast<DummyThreadWorkPackage *>(P);
				assert ( DP );
				DummyThreadWorkPackageMeta * meta = DP->meta;

				static const uint64_t numpacks = 1024;
				static const uint64_t spawn = 3;

				{
				libmaus::parallel::ScopePosixMutex mutex(*(DP->mutex));
				std::cerr << DP << std::endl;
				}

				if ( rand() % 16 == 4 )
				{
					throw std::runtime_error("Random exception");
				}

				for ( uint64_t i = 1; i <= spawn; ++i )
				{
					if ( spawn*DP->packageid+i < numpacks )
					{
						libmaus::parallel::DummyThreadWorkPackage * pack0 = meta->freelist.getPackage();
						*pack0 = libmaus::parallel::DummyThreadWorkPackage(
							DP->priority,
							DP->dispatcherid,
							DP->mutex,
							DP->meta
						);
						tpi.enque(pack0);
					}
				}
				
				if ( ++ meta->finished == numpacks )
					tpi.terminate();
				
				meta->freelist.returnPackage(DP);
			}
		};
	}
}

void testDummyPackages()
{
	libmaus::parallel::SimpleThreadPool TP(8);

	uint64_t const dispid = 0;

	libmaus::parallel::DummyThreadWorkPackageDispatcher dummydisp;
	TP.registerDispatcher(dispid,&dummydisp);
	
	libmaus::parallel::DummyThreadWorkPackageMeta meta;
	libmaus::parallel::PosixMutex printmutex;
	
	libmaus::parallel::DummyThreadWorkPackage * pack = meta.freelist.getPackage(); //(0,dispid,&printmutex,&meta);
	*pack = libmaus::parallel::DummyThreadWorkPackage(0 /* priority */, dispid, &printmutex, &meta);

	TP.enque(pack);
	
	TP.join();	
}

void testDummyRandomExceptionPackages()
{
	try
	{
		libmaus::parallel::SimpleThreadPool TP(8);

		uint64_t const dispid = 0;

		libmaus::parallel::DummyThreadWorkPackageRandomExceptionDispatcher dummydisp;
		TP.registerDispatcher(dispid,&dummydisp);
		
		libmaus::parallel::DummyThreadWorkPackageMeta meta;
		libmaus::parallel::PosixMutex printmutex;
		
		libmaus::parallel::DummyThreadWorkPackage * pack = meta.freelist.getPackage(); //(0,dispid,&printmutex,&meta);
		*pack = libmaus::parallel::DummyThreadWorkPackage(0 /* priority */, dispid, &printmutex, &meta);

		TP.enque(pack);
		
		TP.join();	
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

int main()
{
	// testDummyPackages();
	testDummyRandomExceptionPackages();
}
