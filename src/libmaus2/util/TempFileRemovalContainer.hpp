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
#if ! defined(LIBMAUS2_UTIL_TEMPFILEREMOVALCONTAINER_HPP)
#define LIBMAUS2_UTIL_TEMPFILEREMOVALCONTAINER_HPP

#include <csignal>
#include <cstdio>
#include <vector>
#include <string>
#include <cstdlib>
#include <map>
#include <set>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/parallel/PosixSpinLock.hpp>
#include <semaphore.h>
#include <unistd.h>

namespace libmaus2
{
	namespace util
	{
		struct SignalHandler
		{
			virtual ~SignalHandler()
			{

			}

			virtual void operator()(int const sig) = 0;
		};

		struct SignalHandlerContainer
		{
			#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__)
			typedef sig_t sighandler_t;
			#else
			typedef ::sighandler_t sighandler_t;
			#endif

			static ::libmaus2::parallel::PosixSpinLock lock;

			static sighandler_t siginthandler;
			static sighandler_t sigtermhandler;
			static sighandler_t sighuphandler;
			static sighandler_t sigpipehandler;

			static bool setupComplete;

			static std::map < uint64_t, SignalHandler * > siginthandlers;
			static std::map < uint64_t, SignalHandler * > sigtermhandlers;
			static std::map < uint64_t, SignalHandler * > sighuphandlers;
			static std::map < uint64_t, SignalHandler * > sigpipehandlers;

			static uint64_t addIntHandler(SignalHandler * handler)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				uint64_t const id = siginthandlers.size() ?  (siginthandlers.rbegin()->first+1) : 0;
				siginthandlers[id] = handler;
				return id;
			}

			static uint64_t addTermHandler(SignalHandler * handler)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				uint64_t const id = sigtermhandlers.size() ?  (sigtermhandlers.rbegin()->first+1) : 0;
				sigtermhandlers[id] = handler;
				return id;
			}

			static uint64_t addHupHandler(SignalHandler * handler)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				uint64_t const id = sighuphandlers.size() ?  (sighuphandlers.rbegin()->first+1) : 0;
				sighuphandlers[id] = handler;
				return id;
			}

			static uint64_t addPipeHandler(SignalHandler * handler)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				uint64_t const id = sigpipehandlers.size() ?  (sigpipehandlers.rbegin()->first+1) : 0;
				sigpipehandlers[id] = handler;
				return id;
			}

			static uint64_t addAllHandler(SignalHandler * handler)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);

				uint64_t const pipeid = sigpipehandlers.size() ?  (sigpipehandlers.rbegin()->first+1) : 0;
				uint64_t const hupid = sighuphandlers.size() ?  (sighuphandlers.rbegin()->first+1) : 0;
				uint64_t const termid = sigtermhandlers.size() ?  (sigtermhandlers.rbegin()->first+1) : 0;
				uint64_t const intid = siginthandlers.size() ?  (siginthandlers.rbegin()->first+1) : 0;
				uint64_t const allid = std::max(std::max(pipeid,hupid),std::max(termid,intid));

				siginthandlers[allid] = handler;
				sigtermhandlers[allid] = handler;
				sighuphandlers[allid] = handler;
				sigpipehandlers[allid] = handler;

				return allid;
			}

			static void removeIntHandler(uint64_t const id)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				siginthandlers.erase(siginthandlers.find(id));
			}

			static void removeTermHandler(uint64_t const id)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				sigtermhandlers.erase(sigtermhandlers.find(id));
			}

			static void removeHupHandler(uint64_t const id)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				sighuphandlers.erase(sighuphandlers.find(id));
			}

			static void removePipeHandler(uint64_t const id)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				sigpipehandlers.erase(sigpipehandlers.find(id));
			}

			static void removeAllHandler(uint64_t const id)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				siginthandlers.erase(siginthandlers.find(id));
				sigtermhandlers.erase(sigtermhandlers.find(id));
				sighuphandlers.erase(sighuphandlers.find(id));
				sigpipehandlers.erase(sigpipehandlers.find(id));
			}

			static void sigIntHandler(int arg)
			{
				for ( std::map < uint64_t, SignalHandler * >::iterator ita = siginthandlers.begin();
					ita != siginthandlers.end(); ++ita )
					(*(ita->second))(arg);

				if ( siginthandler )
				{
					siginthandler(arg);
				}
				else
				{
					signal(SIGINT,SIG_DFL);
					raise(SIGINT);
				}
			}

			static void sigHupHandler(int arg)
			{
				for ( std::map < uint64_t, SignalHandler * >::iterator ita = sighuphandlers.begin();
					ita != sighuphandlers.end(); ++ita )
					(*(ita->second))(arg);

				if ( sighuphandler )
				{
					sighuphandler(arg);
				}
				else
				{
					signal(SIGHUP,SIG_DFL);
					raise(SIGHUP);
				}
			}

			static void sigTermHandler(int arg)
			{
				for ( std::map < uint64_t, SignalHandler * >::iterator ita = sigtermhandlers.begin();
					ita != sigtermhandlers.end(); ++ita )
					(*(ita->second))(arg);

				if ( sigtermhandler )
				{
					sigtermhandler(arg);
				}
				else
				{
					signal(SIGTERM,SIG_DFL);
					raise(SIGTERM);
				}
			}

			static void sigPipeHandler(int arg)
			{
				for ( std::map < uint64_t, SignalHandler * >::iterator ita = sigpipehandlers.begin();
					ita != sigpipehandlers.end(); ++ita )
					(*(ita->second))(arg);

				if ( sigpipehandler )
				{
					sigpipehandler(arg);
				}
				else
				{
					signal(SIGPIPE,SIG_DFL);
					raise(SIGPIPE);
				}
			}

			static void setupTempFileRemovalRoutines()
			{
				siginthandler = signal(SIGINT,sigIntHandler);
				sigtermhandler = signal(SIGTERM,sigTermHandler);
				sigpipehandler = signal(SIGPIPE,sigPipeHandler);
				sighuphandler = signal(SIGHUP,sigHupHandler);
			}

			static void setup()
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				if ( ! setupComplete )
				{
					setupTempFileRemovalRoutines();
					setupComplete = true;
				}
			}
		};

		struct TempFileRemovalContainer
		{
			#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__)
			typedef sig_t sighandler_t;
			#else
			typedef ::sighandler_t sighandler_t;
			#endif

			static ::libmaus2::parallel::PosixSpinLock lock;
			static std::set < std::string > tmpfilenames;
			static std::vector < std::string > tmpsemaphores;
			static std::vector < std::string > tmpdirectories;

			static sighandler_t siginthandler;
			static sighandler_t sigtermhandler;
			static sighandler_t sighuphandler;
			static sighandler_t sigpipehandler;

			static bool setupComplete;

			static void cleanup()
			{
				removeTempFiles();
				removeTempDirectories();
				removeSemaphores();
			}

			static void removeTempDirectories()
			{
				for ( uint64_t i = 0; i < tmpdirectories.size(); ++i )
					rmdir ( tmpdirectories[i].c_str() );
			}

			static void removeTempFiles()
			{
				// for ( uint64_t i = 0; i < tmpfilenames.size(); ++i )
				for ( std::set<std::string>::const_iterator ita = tmpfilenames.begin(); ita != tmpfilenames.end(); ++ita )
					libmaus2::aio::FileRemoval::removeFileNoUnregister(*ita);
			}

			static void removeSemaphores()
			{
				for ( uint64_t i = 0; i < tmpsemaphores.size(); ++i )
					sem_unlink(tmpsemaphores[i].c_str());
			}

			static void sigIntHandler(int arg)
			{
				cleanup();
				if ( siginthandler )
				{
					siginthandler(arg);
				}
				else
				{
					signal(SIGINT,SIG_DFL);
					raise(SIGINT);
				}
			}

			static void sigHupHandler(int arg)
			{
				cleanup();
				if ( sighuphandler )
				{
					sighuphandler(arg);
				}
				else
				{
					signal(SIGHUP,SIG_DFL);
					raise(SIGHUP);
				}
			}

			static void sigTermHandler(int arg)
			{
				cleanup();
				if ( sigtermhandler )
				{
					sigtermhandler(arg);
				}
				else
				{
					signal(SIGTERM,SIG_DFL);
					raise(SIGTERM);
				}
			}

			static void sigPipeHandler(int arg)
			{
				cleanup();
				if ( sigpipehandler )
				{
					sigpipehandler(arg);
				}
				else
				{
					signal(SIGPIPE,SIG_DFL);
					raise(SIGPIPE);
				}
			}

			static void setupTempFileRemovalRoutines()
			{
				siginthandler = signal(SIGINT,sigIntHandler);
				sigtermhandler = signal(SIGTERM,sigTermHandler);
				sigpipehandler = signal(SIGPIPE,sigPipeHandler);
				sighuphandler = signal(SIGHUP,sigHupHandler);
				atexit(cleanup);
			}

			static void setupUnlocked()
			{
				if ( ! setupComplete )
				{
					setupTempFileRemovalRoutines();
					setupComplete = true;
				}
			}

			static void setup()
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				setupUnlocked();
			}

			static void removeTempFile(std::string const & filename)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				setupUnlocked();
				std::set<std::string>::const_iterator it = tmpfilenames.find(filename);
				if ( it != tmpfilenames.end() )
				{
					tmpfilenames.erase(it);
				}
			}

			static void addTempFile(std::string const & filename)
			{
				libmaus2::parallel::ScopePosixSpinLock slock(lock);
				setupUnlocked();
				tmpfilenames.insert(filename);
			}
		};
	}
}
#endif
