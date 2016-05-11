/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_AIO_SIZEMONITORTHREAD_HPP)
#define LIBMAUS2_AIO_SIZEMONITORTHREAD_HPP

#include <libmaus2/parallel/PosixThread.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/parallel/PosixSpinLock.hpp>
#include <ctime>
#include <libmaus2/util/UnitNum.hpp>
#include <libmaus2/aio/StreamLock.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct SizeMonitorThread : public libmaus2::parallel::PosixThread
		{
			std::string fn;
			uint64_t sleeptime;
			uint64_t maxsize;
			int volatile stop;
			libmaus2::parallel::PosixSpinLock stoplock;
			std::ostream * sizestream;

			bool getStop()
			{
				int lstop;
				{
					libmaus2::parallel::ScopePosixSpinLock slock(stoplock);
					lstop = stop;
				}
				return lstop;
			}

			void setStop(int const rstop)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(stoplock);
				stop = rstop;
			}

			SizeMonitorThread(std::string const & rfn, uint64_t const rsleeptime = 5, std::ostream * rsizestream = 0)
			: fn(rfn), sleeptime(rsleeptime), maxsize(0), stop(0), sizestream(rsizestream)
			{

			}

			void cancel()
			{
				setStop(1);
				tryJoin();
			}

			~SizeMonitorThread()
			{
				setStop(1);
				tryJoin();
			}

			void printSize(std::ostream & stream)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
				stream << "[S]\t" << fn << "\t" << libmaus2::util::UnitNum::unitNum(maxsize) << std::endl;
			}

			virtual void * run()
			{
				while ( ! getStop() )
				{
					uint64_t const s = libmaus2::util::GetFileSize::getDirSize(fn);
					// (*sizestream) << "[S]\t" << fn << "\t" << s << std::endl;

					if ( s > maxsize )
					{
						maxsize = s;
						if ( sizestream )
							printSize(*sizestream);
					}

					struct timespec req, rem;

					req.tv_sec = sleeptime;
					req.tv_nsec = 0;
					rem.tv_sec = 0;
					rem.tv_nsec = 0;

					nanosleep(&req,&rem);
				}

				return 0;
			}
		};
	}
}
#endif
