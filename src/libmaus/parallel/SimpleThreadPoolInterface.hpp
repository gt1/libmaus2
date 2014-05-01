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
#if ! defined(LIBMAUS_PARALLEL_SIMPLETHREADPOOLINTERFACE_HPP)
#define LIBMAUS_PARALLEL_SIMPLETHREADPOOLINTERFACE_HPP

#include <libmaus/parallel/ThreadPoolInterfaceEnqueTermInterface.hpp>
#include <libmaus/parallel/SimpleThreadPoolInterfaceEnqueTermInterface.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

namespace libmaus
{
	namespace parallel
	{		
		struct SimpleThreadPoolInterface : public SimpleThreadPoolInterfaceEnqueTermInterface
		{
			virtual ~SimpleThreadPoolInterface() {}
			
			virtual void notifyThreadStart() = 0;
			virtual void registerDispatcher(uint64_t const id, SimpleThreadWorkPackageDispatcher * D) = 0;
			virtual void panic(std::exception const &) = 0;
			virtual void panic(libmaus::exception::LibMausException const &) = 0;
			virtual bool isInPanicMode() = 0;

			virtual SimpleThreadWorkPackage * getPackage() = 0;
			virtual SimpleThreadWorkPackageDispatcher * getDispatcher(libmaus::parallel::SimpleThreadWorkPackage * P) = 0;
		};
	}
}
#endif
