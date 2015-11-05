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
#if ! defined(LIBMAUS2_PARALLEL_THREADPOOLTHREAD_HPP)
#define LIBMAUS2_PARALLEL_THREADPOOLTHREAD_HPP

#include <libmaus2/parallel/PosixThread.hpp>
#include <libmaus2/parallel/ThreadPoolInterface.hpp>

namespace libmaus2
{
	namespace parallel
	{
		struct ThreadPoolThread : libmaus2::parallel::PosixThread
		{
			typedef ThreadPoolThread this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			ThreadPoolInterface & tpi;

			ThreadPoolThread(ThreadPoolInterface & rtpi) : tpi(rtpi)
			{
			}
			virtual ~ThreadPoolThread() {}

			void * run()
			{
				try
				{
					// notify pool this thread is now running
					tpi.notifyThreadStart();

					while ( true )
					{
						libmaus2::parallel::ThreadWorkPackage * P = tpi.getPackage();
						ThreadWorkPackageDispatcher * disp = tpi.getDispatcher(P);
						disp->dispatch(P,tpi);
						tpi.freePackage(P);
					}
				}
				catch(std::exception const & ex)
				{
					// std::cerr << ex.what() << std::endl;
				}

				return 0;
			}
		};
	}
}
#endif
