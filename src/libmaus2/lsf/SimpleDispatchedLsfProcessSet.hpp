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
#include <libmaus2/lsf/DispatchedLsfProcess.hpp>
#include <libmaus2/lsf/LSFTaskQueue.hpp>
#include <libmaus2/network/LogReceiver.hpp>
#include <libmaus2/parallel/PosixMutex.hpp>
#include <iostream>

namespace libmaus2
{
	namespace lsf
	{
		struct SimpleDispatchedLsfProcessSet : public ::libmaus2::parallel::PosixThread
		{
			typedef SimpleDispatchedLsfProcessSet this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef ::libmaus2::lsf::DispatchedLsfProcess::shared_ptr_type proc_ptr;
			typedef std::map < uint64_t, proc_ptr > map_type;
			typedef map_type::const_iterator const_iterator;
			typedef map_type::iterator iterator;

			::libmaus2::parallel::PosixMutex quitlock;
			::libmaus2::parallel::PosixMutex qlock;
			std::map < uint64_t, proc_ptr > unknown;
			std::map < uint64_t, proc_ptr > pending;
			std::map < uint64_t, proc_ptr > running;
			std::map < uint64_t, proc_ptr > finished;

			std::map < uint64_t, proc_ptr > active;
			std::map < uint64_t, proc_ptr > all;

			std::set < uint64_t > used;
			std::set < uint64_t > unused;

			::libmaus2::network::LogReceiver LR;
			uint64_t nextid;
			bool const verbose;

			SimpleDispatchedLsfProcessSet(std::string const name, bool const rverbose = false)
			: LR(name,4444,1024), nextid(0), verbose(rverbose)
			{
				quitlock.lock();
				start();
			}
			~SimpleDispatchedLsfProcessSet()
			{
				quitlock.unlock();
				join();
			}

			virtual void * run()
			{
				try
				{
					// run until parent tells us to stop
					while ( ! quitlock.trylock() )
					{
						{
							::libmaus2::parallel::ScopePosixMutex PSL(qlock);
							std::map<int64_t,int64_t> IS;

							try
							{
								IS = ::libmaus2::lsf::LSFProcess::getIntStates();
							}
							catch(std::exception const & ex)
							{
								std::cerr << "Failed to get LSF process states." << std::endl;
								std::cerr << ex.what();

								// wait for 5 s
								struct timespec waittime = { 5,0 };
								nanosleep(&waittime,0);

								// try again
								continue;
							}

							/* move processes from the unknown to the pending queue */
							std::vector<uint64_t> moveV;
							for ( const_iterator ita = unknown.begin(); ita != unknown.end(); ++ita )
							{
								::libmaus2::lsf::LSFStateBase::state const state = ita->second->getState(IS);

								// keep unknown processes, move rest
								switch ( state )
								{
									case ::libmaus2::lsf::LSFStateBase::state_unknown:
										break;
									default:
										moveV.push_back(ita->first);
										break;
								}
							}
							while ( moveV.size() )
							{
								uint64_t const id = moveV.back();
								pending [ id ] = unknown.find(id)->second;
								unknown.erase(unknown.find(id));
								if ( verbose )
									std::cerr << "moved id " << id << " from unknown to pending queue." << std::endl;
								moveV.pop_back();
							}

							/* move pending processes to the running queue */
							for ( const_iterator ita = pending.begin(); ita != pending.end(); ++ita )
							{
								::libmaus2::lsf::LSFStateBase::state const state = ita->second->getState(IS);

								// std::cerr << "Pending process " << ita->first << " state " << state << std::endl;

								// keep pending processes, move rest
								switch ( state )
								{
									case ::libmaus2::lsf::LSFStateBase::state_pending:
									case ::libmaus2::lsf::LSFStateBase::state_psusp:
										break;
									default:
										moveV.push_back(ita->first);
										break;
								}
							}
							while ( moveV.size() )
							{
								uint64_t const id = moveV.back();
								running [ id ] = pending.find(id)->second;
								pending.erase(pending.find(id));
								if ( verbose )
									std::cerr << "moved id " << id << " from pending to running queue." << std::endl;
								moveV.pop_back();
							}

							/* move running processes to the finished queue */
							for ( const_iterator ita = running.begin(); ita != running.end(); ++ita )
							{
								::libmaus2::lsf::LSFStateBase::state const state = ita->second->getState(IS);

								// keep running processes, move rest
								switch ( state )
								{
									case ::libmaus2::lsf::LSFStateBase::state_ssusp:
									case ::libmaus2::lsf::LSFStateBase::state_ususp:
									case ::libmaus2::lsf::LSFStateBase::state_run:
										break;
									default:
										// std::cerr << "proc " << ita->first << " has state " << state << std::endl;
										moveV.push_back(ita->first);
										break;
								}
							}
							while ( moveV.size() )
							{
								uint64_t const id = moveV.back();
								finished [ id ] = running.find(id)->second;
								running.erase(running.find(id));
								if ( verbose )
									std::cerr << "moved id " << id << " from running to finished queue." << std::endl;
								moveV.pop_back();
							}

							/**
							 * receive new control file descriptors
							 **/
							while ( LR.controlDescriptorPending() )
							{
								// std::cerr << "Descriptor is pending." << std::endl;

								try
								{
									::libmaus2::network::LogReceiver::ControlDescriptor cd = LR.getControlDescriptor();
									all[cd.id]->controlsocket = cd.controlsock;
									all[cd.id]->hostname      = cd.hostname;
									active[cd.id]             = all.find(cd.id)->second;
									unused.insert(cd.id);
									if ( verbose )
										std::cerr << "Received descriptor for id " << cd.id << std::endl;

									// wait for 1/10 s
									struct timespec waittime = { 0, 100000000 };
									nanosleep(&waittime,0);
								}
								catch(std::exception const & ex)
								{
									std::cerr << ex.what() << std::endl;
								}
							}
						}

						// std::cerr << "Running." << std::endl;
						// wait for 5 s
						struct timespec waittime = { 5, 0 };
						nanosleep(&waittime,0);
					}

					// std::cerr << "Quit." << std::endl;

					quitlock.unlock();
				}
				catch(std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
				}
				return 0;
			}

			uint64_t startProcessUnlocked(
				::libmaus2::parallel::PosixMutex * tlock,
				::libmaus2::util::ArgInfo const & arginfo,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			{
				if ( tlock )
					tlock->lock();

				uint64_t id = nextid++;

				if ( tlock )
					tlock->unlock();

				proc_ptr ptr(new ::libmaus2::lsf::DispatchedLsfProcess(
					arginfo,
					LR.sid,
					LR.hostname,
					LR.port,
					id,
					dispatcherexe,
					payloadexe,
					numthreads,
					mem,
					hosts,
					cwd,
					tmpspace,
					valgrind
				));

				if ( tlock )
					tlock->lock();

				unknown[id] = ptr;
				all[id] = ptr;

				if ( tlock )
					tlock->unlock();

				return id;
			}

			uint64_t startProcess(
				::libmaus2::util::ArgInfo const & arginfo,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			{
				return startProcessUnlocked(&qlock,arginfo,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind);
			}

			void startProcesses(
				std::vector<uint64_t> & ids,
				uint64_t const n,
				::libmaus2::util::ArgInfo const & arginfo,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			{
				::libmaus2::parallel::ScopePosixMutex PSL(qlock);
				for ( uint64_t i = 0; i < n; ++i )
					ids.push_back(startProcessUnlocked(0,arginfo,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind));
			}

			void startProcessesSingleLock(
				std::vector<uint64_t> & ids,
				uint64_t const n,
				::libmaus2::util::ArgInfo const & arginfo,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			{
				for ( uint64_t i = 0; i < n; ++i )
					ids.push_back(startProcess(arginfo,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind));
			}

			std::vector<uint64_t> startProcesses(
				uint64_t const n,
				::libmaus2::util::ArgInfo const & arginfo,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			{
				std::vector<uint64_t> ids;
				startProcesses(ids,n,arginfo,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind);
				return ids;
			}

			std::vector<uint64_t> startProcessesSingleLock(
				uint64_t const n,
				::libmaus2::util::ArgInfo const & arginfo,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			{
				std::vector<uint64_t> ids;
				startProcessesSingleLock(ids,n,arginfo,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind);
				return ids;
			}

			bool isRunning(uint64_t const id)
			{
				::libmaus2::parallel::ScopePosixMutex PSL(qlock);

				bool const run = running.find(id) != running.end();

				return run;
			}
			bool isFinished(uint64_t const id)
			{
				::libmaus2::parallel::ScopePosixMutex PSL(qlock);

				bool const run = finished.find(id) != finished.end();

				return run;
			}
			bool isActive(uint64_t const id)
			{
				::libmaus2::parallel::ScopePosixMutex PSL(qlock);

				bool const run = active.find(id) != active.end();

				return run;
			}

			uint64_t numFinished()
			{
				::libmaus2::parallel::ScopePosixMutex PSL(qlock);

				uint64_t const numfin = finished.size();

				return numfin;
			}
			uint64_t numActive()
			{
				::libmaus2::parallel::ScopePosixMutex PSL(qlock);

				uint64_t const numact = active.size();

				return numact;
			}

			proc_ptr process(uint64_t const id)
			{
				::libmaus2::parallel::ScopePosixMutex PSL(qlock);

				proc_ptr proc = all.find(id)->second;

				return proc;
			}

			void deactivate(uint64_t const id)
			{
				::libmaus2::parallel::ScopePosixMutex PSL(qlock);

				if ( active.find(id) != active.end() )
					active.erase(active.find(id));
				if ( used.find(id) != used.end() )
					used.erase(used.find(id));
				if ( unused.find(id) != unused.end() )
					unused.erase(unused.find(id));
			}

			void waitOneNoRestart()
			{
				while ( true )
				{
					{
						::libmaus2::parallel::ScopePosixMutex PSL(qlock);

						if ( active.size() )
							break;
					}

					// wait
					struct timespec waittime = { 1, 0 };
					nanosleep(&waittime,0);
				}
			}

			void waitK(
				::libmaus2::parallel::PosixMutex * lquitlock,
				std::vector<uint64_t> & ids,
				uint64_t const k,
				uint64_t const /* n */,
				::libmaus2::util::ArgInfo const & arginfo,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			{
				while ( (!lquitlock) || (! (lquitlock->tryLockUnlock())) )
				{
					int sleeptime = -1;
					int restart = 0;

					{
						::libmaus2::parallel::ScopePosixMutex PSL(qlock);

						if ( active.size() >= k )
						{
							break;
						}
						else if ( unknown.size() + pending.size() + running.size() < k )
						{
							std::cerr << "Process(es) seem(s) to have died, starting one more." << std::endl;
							restart = 1;
							sleeptime = 5;
						}
						else
						{
							sleeptime = 1;
						}
					}

					if ( restart > 0 )
						startProcesses(ids,restart,arginfo,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind);

					// wait
					struct timespec waittime = { sleeptime, 0 };
					nanosleep(&waittime,0);
				}
			}

			std::vector<uint64_t> startProcessesAndWait(
				uint64_t const k,
				uint64_t const n,
				::libmaus2::util::ArgInfo const & arginfo,
				std::string const & dispatcherexe,
				std::string const & payloadexe,
				uint64_t const numthreads,
				uint64_t const mem,
				std::vector<std::string> const * hosts = 0,
				char const * cwd = 0,
				uint64_t const tmpspace = 0,
				bool const valgrind = false
			)
			{
				assert ( k <= n );

				std::vector<uint64_t> ids;
				startProcesses(ids,n,arginfo,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind);
				waitK(0/* quit lock */,ids,k,n,arginfo,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind);

				return ids;
			}

			bool haveUnused()
			{
				::libmaus2::parallel::ScopePosixMutex PSL(qlock);
				bool const r = unused.size() > 0;
				return r;
			}

			uint64_t getUnused()
			{
				if ( ! haveUnused() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "SimpleDispatchedLsfProcessSet::getUnused() called, but there are no unused processes." << std::endl;
					se.finish();
					throw se;
				}

				::libmaus2::parallel::ScopePosixMutex PSL(qlock);
				uint64_t const id = *(unused.begin());
				unused.erase(unused.find(id));
				used.insert(id);

				return id;
			}

			void returnUsed(uint64_t const id)
			{
				::libmaus2::parallel::ScopePosixMutex PSL(qlock);
				used.erase(used.find(id));
				unused.insert(id);
			}

			uint64_t selectActive()
			{
				while ( true )
				{
					{
						::libmaus2::parallel::ScopePosixMutex PSL(qlock);

						if ( ! active.size() )
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "selectActive() called, but there are no active processes." << std::endl;
							se.finish();
							throw se;
						}

						std::vector<int> fds;

						std::map<int,uint64_t> fdToIdMap;
						for ( std::map < uint64_t, proc_ptr >::const_iterator ita = active.begin();
							ita != active.end(); ++ita )
						{
							int const fd = ita->second->controlsocket->getFD();
							fdToIdMap [ fd  ] = ita->first;
							fds.push_back(fd);
						}

						int pfd = -1;
						int const r = ::libmaus2::network::SocketBase::multiPoll(fds,pfd);

						if ( r < 0 )
						{
							switch ( errno )
							{
								case EAGAIN:
								case EINTR:
									break;
								default:
								{
									int const error = errno;
									::libmaus2::exception::LibMausException se;
									se.getStream() << "poll() failed in selectActive():" << strerror(error) << std::endl;
									se.finish();
									throw se;
								}
							}
						}
						else if ( r == 0 )
						{
							// std::cerr << "select() returned 0 in selectActive() !?!" << std::endl;
							// sleep(1);
							struct timespec waittime = { 0, 100000000 };
							nanosleep(&waittime,0);
						}
						else
						{
							uint64_t const id = fdToIdMap.find(pfd)->second;

							return id;
						}
					}

					// wait for 1/10 s while not holding lock
					struct timespec waittime = { 0, 100000000 };
					nanosleep(&waittime,0);
				}
			}

			struct AsynchronousStarter : public ::libmaus2::parallel::PosixThread
			{
				typedef AsynchronousStarter this_type;
				typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				SimpleDispatchedLsfProcessSet * obj;

				uint64_t const n;
				::libmaus2::util::ArgInfo const & arginfo;
				std::string const & dispatcherexe;
				std::string const & payloadexe;
				uint64_t const numthreads;
				uint64_t const mem;
				std::vector<std::string> const * hosts;
				char const * cwd;
				uint64_t const tmpspace;
				bool const valgrind;

				::libmaus2::parallel::PosixMutex quitlock;

				AsynchronousStarter(
					SimpleDispatchedLsfProcessSet * robj,
					uint64_t const rn,
					::libmaus2::util::ArgInfo const & rarginfo,
					std::string const & rdispatcherexe,
					std::string const & rpayloadexe,
					uint64_t const rnumthreads,
					uint64_t const rmem,
					std::vector<std::string> const * rhosts = 0,
					char const * rcwd = 0,
					uint64_t const rtmpspace = 0,
					bool const rvalgrind = false
				)
				: obj(robj), n(rn), arginfo(rarginfo), dispatcherexe(rdispatcherexe),
				  payloadexe(rpayloadexe), numthreads(rnumthreads), mem(rmem),
				  hosts(rhosts), cwd(rcwd), tmpspace(rtmpspace), valgrind(rvalgrind)
				{
					quitlock.lock();
					start();
				}
				~AsynchronousStarter()
				{
					quitlock.unlock();
				}

				virtual void * run()
				{
					try
					{
						std::vector<uint64_t> ids;

						for ( uint64_t i = 0; i < n && (!(quitlock.tryLockUnlock())); ++i )
							ids.push_back(
								obj->startProcess(arginfo,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind)
							);

						obj->waitK(&quitlock,ids,1,n,arginfo,dispatcherexe,payloadexe,numthreads,mem,hosts,cwd,tmpspace,valgrind);

						std::cerr << "asynchronous starter about to quit." << std::endl;
					}
					catch(std::exception const & ex)
					{
						std::cerr << ex.what() << std::endl;
					}

					return 0;
				}
			};

			void processQueue(
				LsfTaskQueue & LTQ,
				::libmaus2::util::ArgInfo const & arginfo,
				uint64_t const numprocs,
				std::string const & lsfdispatcher,
				std::string const & lsfclient,
				uint64_t const memmb,
				uint64_t const threadsperproc = 1
				)
			{
				std::cerr << "starting " << numprocs << " processes "
					<< " dispatcher " << lsfdispatcher
					<< " client " << lsfclient << std::endl;

				#if 0
				/* std::vector < uint64_t > ids = */ startProcessesAndWait(1,
					numprocs,arginfo,lsfdispatcher,lsfclient,threadsperproc,memmb);
				#else
				AsynchronousStarter::unique_ptr_type AS(new AsynchronousStarter(this, numprocs,arginfo,lsfdispatcher,lsfclient,threadsperproc,memmb));
				waitOneNoRestart();
				#endif

				std::map<uint64_t,uint64_t> activepackets;

				while ( (!LTQ.empty()) || (activepackets.size()) || numActive() )
				{
					// are all processes gone?
					if ( ! numActive() )
					{
						while (
							! numActive()
						)
						{
							if ( ! (unknown.size() + pending.size() + running.size()) )
							{
								std::cerr << "All processes are dead but there is still work, starting new processes." << std::endl;
								std::cerr << "LTQ.empty()=" << LTQ.empty() << std::endl;
								std::cerr << "activepackets.size()=" << activepackets.size() << std::endl;

								/* ids = */ startProcessesAndWait(1,numprocs,arginfo,lsfdispatcher,lsfclient,threadsperproc,memmb);
							}
							else
							{
								struct timespec waittime = { 1, 0 };
								nanosleep(&waittime,0);
							}
						}
					}

					int64_t activeid = -1;

					try
					{
						activeid = selectActive();
						::libmaus2::lsf::SimpleDispatchedLsfProcessSet::proc_ptr proc = process(activeid);
						assert ( proc );
						assert ( proc->controlsocket );

						uint64_t const req = proc->controlsocket->readSingle<uint64_t>();

						if ( req == 0 )
						{
							// there is still work to do, send package
							if ( !(LTQ.empty()) && (LTQ.numUnfinished() >= numActive()) )
							{
								LsfTaskQueueElement const qe = LTQ.nextPacket();
								uint64_t const packet = qe.id;
								std::string const packettype = qe.packettype;
								std::string const spacket = qe.payload;
								activepackets[activeid] = packet;

								// send packet type
								proc->controlsocket->writeString(packettype);
								// send packet id
								proc->controlsocket->writeSingle<uint64_t>(packet);
								// send packet payload data
								proc->controlsocket->writeString(spacket);

								if ( LTQ.isRealPackage(packet) )
								{
									std::cerr << "sent packet " << qe << " to process " << activeid << std::endl;
								}
								// std::cerr << "sent packet to id " << activeid << std::endl;
							}
							// no more work or too many active processes, deactivate process
							else
							{
								proc->controlsocket->writeString(LTQ.termpackettype());
								proc->controlsocket->barrierWr();

								// std::cerr << "queue is empty, deactivating " << activeid << std::endl;
								deactivate(activeid);
							}
						}
						else if ( req == 1 )
						{
							// std::cerr << "process " << activeid << " finished a packet, ";
							if ( activepackets.find(activeid) != activepackets.end() )
							{
								uint64_t const packid = activepackets.find(activeid)->second;
								uint64_t const remid = proc->controlsocket->readSingle<uint64_t>();
								std::string const reply = proc->controlsocket->readString();
								// std::cerr << "packet id " << packid << " reply length " << reply.size() << std::endl;
								assert ( remid == packid );
								activepackets.erase(activepackets.find(activeid));
								LTQ.finished(packid,reply);

								#if 0
								if ( LTQ.empty() )
									std::cerr << "queue is now empty." << std::endl;
								#endif
							}
							else
							{
								std::cerr << "no active package for process ???";
							}
						}
						else
						{
							std::cerr << "unknown request type " << req << " from process " << activeid << ", expect failure." << std::endl;
						}
					}
					catch(std::exception const & ex)
					{
						std::cerr << ex.what() << std::endl;

						if ( activeid >= 0 )
						{
							if ( activepackets.find(activeid) != activepackets.end() )
							{
								uint64_t const packet = activepackets.find(activeid)->second;
								activepackets.erase(activepackets.find(activeid));
								LTQ.putback(packet);
								std::cerr << "Requeuing failed packet " << packet << std::endl;
							}

							std::cerr << "Deactivating process " << activeid << " after broken communication attempt." << std::endl;
							deactivate(activeid);
						}
					}
				}

				std::cerr << "Left loop." << std::endl;

				AS.reset();
			}

		};
	}
}
