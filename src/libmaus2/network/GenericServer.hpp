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


#if ! defined(FASTSERVER_HPP)
#define FASTSERVER_HPP

#include <libmaus2/network/Socket.hpp>
#include <libmaus2/aio/SynchronousFastReaderBase.hpp>
#include <libmaus2/aio/GenericInput.hpp>
#include <libmaus2/aio/ReorderConcatGenericInput.hpp>
#include <iostream>
#include <set>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <csignal>

namespace libmaus2
{
        namespace network
        {
        	template<typename _type>
                struct GenericServer
                {
                	typedef _type type;
                        typedef GenericServer<type> this_type;
                        typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                        typedef ::libmaus2::network::ServerSocket server_socket_type;
                        typedef server_socket_type::unique_ptr_type server_socket_ptr_type;

                        std::set<std::string> filenames;
                        pid_t pid;
                        unsigned short port;
                        server_socket_ptr_type seso;

                        static void sigchildhandler(int)
                        {
                        	int status;
                        	pid_t pid = waitpid(-1,&status,WNOHANG);
                        	// std::cerr << "sigchld for id " << pid << std::endl;
                        }

                        GenericServer(
                                std::set<std::string> const & rfilenames,
                                std::string const & shostname,
                                unsigned short rport = 4444,
                                unsigned int const backlog = 128)
                        : filenames(rfilenames), port(rport),
                          seso(UNIQUE_PTR_MOVE(server_socket_type::allocateServerSocket(port,backlog,shostname.c_str(),8*1024)))
                        {
                        }

                        ~GenericServer()
                        {
                                shutdown();
                        }

                        void exec()
                        {
                                pid = fork();

                                if ( pid < 0 )
                                {
                                        ::libmaus2::exception::LibMausException ex;
                                        ex.getStream() << "failed to fork: " << strerror(errno);
                                        ex.finish();
                                        throw ex;
                                }

                                if ( ! pid )
                                {
                                	signal(SIGCHLD,sigchildhandler);

                                        while ( true )
                                        {

                                                try
                                                {
	                                                ::libmaus2::network::SocketBase::unique_ptr_type recsock = seso->accept();

                                                        pid_t childpid = fork();

                                                        if ( childpid == 0 )
                                                        {
                                                        	try
                                                        	{
									uint64_t tag;
									std::string const filename = recsock->readString(tag);

									if ( filenames.find(filename) != filenames.end() )
									{
										uint64_t offset, n;
										recsock->readMessage<uint64_t>(tag,&offset,n);
										uint64_t limit;
										recsock->readMessage<uint64_t>(tag,&limit,n);

										uint64_t const blocksize = 16*1024*sizeof(type);
										uint64_t const byteoffset = offset*sizeof(type);
										uint64_t todo = ::libmaus2::util::GetFileSize::getFileSize(filename) - byteoffset;

										if ( limit != std::numeric_limits<uint64_t>::max() )
											todo = std::min(todo,limit*sizeof(type));

										if ( todo % sizeof(type) != 0 )
										{
											::libmaus2::exception::LibMausException se;
											se.getStream() << "Data size not multiple of type size.";
											se.finish();
											throw se;
										}

										uint64_t todowords = todo/sizeof(type);
										recsock->writeMessage<uint64_t>(0,&todowords,1);

										::libmaus2::aio::SynchronousFastReaderBase SFRB(
											filename,0,blocksize,byteoffset
										);

										::libmaus2::autoarray::AutoArray<char> B(blocksize,false);

										while ( todo )
										{
											uint64_t const toproc = std::min(blocksize,todo);

											for ( uint64_t k = 0; k < toproc; ++k )
												B[k] = SFRB.getNextCharacter();
											recsock->write(B.get(),toproc);

											todo -= toproc;
										}
									}
								}
								catch(std::exception const & ex)
								{
									std::cerr << ex.what() << std::endl;
								}

                                                                _exit(0);
                                                        }
                                                }
                                                catch(std::exception const & ex)
                                                {
                                                        std::cerr << "Error in GenericServer: " << ex.what() << std::endl;
                                                }
                                        }

                                        _exit(0);
                                }
                        }

                        void shutdown()
                        {
                                kill(pid,SIGTERM);
                        }
                };

		template < typename input_type >
		struct GenericClient
		{
			typedef input_type value_type;

			uint64_t const bufsize;
			::libmaus2::autoarray::AutoArray<input_type> buffer;
			input_type const * const pa;
			input_type const * pc;
			input_type const * pe;

			typedef ::libmaus2::network::ClientSocket socket_type;
			typedef socket_type::unique_ptr_type socket_ptr_type;
			socket_ptr_type socket;

			uint64_t totalwords;
			uint64_t totalwordsread;

			typedef GenericClient<input_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			static ::libmaus2::autoarray::AutoArray<input_type> readArray(
				std::string const & servername,
				unsigned short const serverport,
				std::string const & inputfilename
			)
			{
				GenericClient<input_type> in(servername,serverport,inputfilename,64*1024);
				uint64_t const n = in.totalwords;
				::libmaus2::autoarray::AutoArray<input_type> A(n,false);

				for ( uint64_t i = 0; i < n; ++i )
				{
					input_type v;
					bool ok = in.getNext(v);
					assert ( ok );
					A[i] = v;
				}

				return A;
			}

			GenericClient(
				std::string const & servername,
				unsigned short const serverport,
				std::string const & filename,
				uint64_t const rbufsize,
				uint64_t const roffset = 0,
				uint64_t const rtotalwords = std::numeric_limits<uint64_t>::max()
			)
			: bufsize(rbufsize), buffer(bufsize,false),
			  pa(buffer.get()), pc(pa), pe(pa),
			  totalwords (0),
			  totalwordsread(0)
			{
				socket = socket_ptr_type(new socket_type(serverport,servername.c_str()));
				socket->writeString(0,filename);
				socket->writeMessage<uint64_t>(0,&roffset,1);
				socket->writeMessage<uint64_t>(0,&rtotalwords,1);
				uint64_t tag, n;
				socket->readMessage<uint64_t>(tag,&totalwords,n);
			}
			~GenericClient()
			{
			}
			bool getNext(input_type & word)
			{
				if ( pc == pe )
				{
					assert ( totalwordsread <= totalwords );
					uint64_t const remwords = totalwords-totalwordsread;
					uint64_t const toreadwords = std::min(remwords,bufsize);

					uint64_t const bytesread = socket->read ( reinterpret_cast<char *>(buffer.get()), toreadwords * sizeof(input_type));
					assert ( bytesread % sizeof(input_type) == 0 );
					uint64_t const wordsread = bytesread / sizeof(input_type);

					if ( wordsread == 0 )
						return false;

					pc = pa;
					pe = pa + wordsread;
				}

				word = *(pc++);
				totalwordsread++;

				return true;
			}
		};

        	template<typename _type>
                struct GenericConcatServer
                {
                	typedef _type type;
                        typedef GenericConcatServer<type> this_type;
                        typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                        typedef ::libmaus2::network::ServerSocket server_socket_type;
                        typedef server_socket_type::unique_ptr_type server_socket_ptr_type;

                        std::vector<std::string> filenames;
                        ::libmaus2::autoarray::AutoArray< std::pair <uint64_t,uint64_t> > const & P;
                        pid_t pid;
                        unsigned short port;
                        server_socket_ptr_type seso;

                        static void sigchildhandler(int)
                        {
                        	int status;
                        	/* pid_t pid = */ waitpid(-1,&status,WNOHANG);
                        	// std::cerr << "sigchld for id " << pid << std::endl;
                        }

                        GenericConcatServer(
                                std::vector<std::string> const & rfilenames,
                                ::libmaus2::autoarray::AutoArray< std::pair <uint64_t,uint64_t> > const & rP,
                                std::string const & shostname,
                                unsigned short rport = 4444,
                                unsigned int const backlog = 128)
                        : filenames(rfilenames), P(rP), port(rport),
                          seso(UNIQUE_PTR_MOVE(server_socket_type::allocateServerSocket(port,backlog,shostname.c_str(),8*1024)))
                        {
                        }

                        ~GenericConcatServer()
                        {
                                shutdown();
                        }

                        void exec()
                        {
                                pid = fork();

                                if ( pid < 0 )
                                {
                                        ::libmaus2::exception::LibMausException ex;
                                        ex.getStream() << "failed to fork: " << strerror(errno);
                                        ex.finish();
                                        throw ex;
                                }

                                if ( ! pid )
                                {
                                	signal(SIGCHLD,sigchildhandler);

                                        while ( true )
                                        {

                                                try
                                                {
	                                                ::libmaus2::network::SocketBase::unique_ptr_type recsock = seso->accept();

                                                        pid_t childpid = fork();

                                                        if ( childpid == 0 )
                                                        {
                                                        	try
                                                        	{
                                                        	        uint64_t const fileid = recsock->readSingle<uint64_t>();

									if ( fileid < P.size() )
									{
									        uint64_t const blocksize = 64*1024;
										uint64_t todo = P[fileid].second-P[fileid].first;

										recsock->writeSingle(todo);

										// std::cerr << "Opening file...";

										typename ::libmaus2::aio::ReorderConcatGenericInput<type>::unique_ptr_type file =
										        ::libmaus2::aio::ReorderConcatGenericInput<type>::openConcatFile(
										                filenames,64*1024,todo,P[fileid].first);

										// std::cerr << "Opened file, todo=" << todo << std::endl;

										::libmaus2::autoarray::AutoArray<type> B(blocksize,false);

										while ( todo )
										{
											uint64_t const toproc = std::min(blocksize,todo);

											// std::cerr << "getting" << toproc << "...";

											file->fillBlock(B.get(), toproc);
											/*
											for ( uint64_t k = 0; k < toproc; ++k )
												file->getNext(B[k]);
                                                                                        */

                                                                                        // std::cerr << "gotok";

											recsock->write(reinterpret_cast<char const *>(B.get()),toproc*sizeof(type));

											// std::cerr << "writeok";

											todo -= toproc;
										}
									}
									else
									{
                                                                                std::cerr << "Client asked for nonexisting file." << std::endl;
										recsock->writeSingle(0);
									}
								}
								catch(std::exception const & ex)
								{
									std::cerr << ex.what() << std::endl;
								}

                                                                _exit(0);
                                                        }
                                                }
                                                catch(std::exception const & ex)
                                                {
                                                        std::cerr << "Error in GenericConcatServer: " << ex.what() << std::endl;
                                                }
                                        }

                                        _exit(0);
                                }
                        }

                        void shutdown()
                        {
                                kill(pid,SIGTERM);
                        }
                };

		template < typename input_type >
		struct GenericConcatClient
		{
			typedef input_type value_type;

			uint64_t const bufsize;
			::libmaus2::autoarray::AutoArray<input_type> buffer;
			input_type const * const pa;
			input_type const * pc;
			input_type const * pe;

			typedef ::libmaus2::network::ClientSocket socket_type;
			typedef socket_type::unique_ptr_type socket_ptr_type;
			socket_ptr_type socket;

			uint64_t totalwords;
			uint64_t totalwordsread;

			typedef GenericConcatClient<input_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			static ::libmaus2::autoarray::AutoArray<input_type> readArray(
				std::string const & servername,
				unsigned short const serverport,
				std::string const & inputfilename
			)
			{
				GenericConcatClient<input_type> in(servername,serverport,inputfilename,64*1024);
				uint64_t const n = in.totalwords;
				::libmaus2::autoarray::AutoArray<input_type> A(n,false);

				for ( uint64_t i = 0; i < n; ++i )
				{
					input_type v;
					bool ok = in.getNext(v);
					assert ( ok );
					A[i] = v;
				}

				return A;
			}

			GenericConcatClient(
				std::string const & servername,
				unsigned short const serverport,
				uint64_t const fileid,
				uint64_t const rbufsize
			)
			: bufsize(rbufsize), buffer(bufsize,false),
			  pa(buffer.get()), pc(pa), pe(pa),
			  totalwords (0),
			  totalwordsread(0)
			{
				socket = UNIQUE_PTR_MOVE(socket_ptr_type(new socket_type(serverport,servername.c_str())));
				socket->writeSingle<uint64_t>(fileid);
				totalwords = socket->readSingle<uint64_t>();
			}
			~GenericConcatClient()
			{
			}
			bool getNext(input_type & word)
			{
				if ( pc == pe )
				{
				        // std::cerr << "*";

					assert ( totalwordsread <= totalwords );
					uint64_t const remwords = totalwords-totalwordsread;
					uint64_t const toreadwords = std::min(remwords,bufsize);

					uint64_t const bytesread = socket->read ( reinterpret_cast<char *>(buffer.get()), toreadwords * sizeof(input_type));
					assert ( bytesread % sizeof(input_type) == 0 );
					uint64_t const wordsread = bytesread / sizeof(input_type);

					// std::cerr << "#";

					if ( wordsread == 0 )
						return false;

					pc = pa;
					pe = pa + wordsread;

					// std::cerr << "/";
				}

				word = *(pc++);
				totalwordsread++;

				return true;
			}
		};

		template < typename input_type >
		struct AsynchronousGenericConcatClient : public ::libmaus2::parallel::PosixThread
		{
			typedef input_type value_type;

			typedef ::libmaus2::network::ClientSocket socket_type;
			typedef socket_type::unique_ptr_type socket_ptr_type;
			socket_ptr_type socket;

			uint64_t const bufsize;

			uint64_t totalwords;
			uint64_t totalwordsread;

			::libmaus2::autoarray::AutoArray<input_type> inbuffer;

			::libmaus2::autoarray::AutoArray<input_type> outbuffer;
			input_type const * pc;
			input_type const * pe;

			::libmaus2::parallel::SynchronousQueue<uint64_t> emptyqueue;
			::libmaus2::parallel::SynchronousQueue<uint64_t> fullqueue;

			virtual void * run()
			{
			        bool running = true;

			        while ( running )
			        {
			                emptyqueue.deque();

                                        assert ( totalwordsread <= totalwords );
                                        uint64_t const remwords = totalwords-totalwordsread;
                                        uint64_t const toreadwords = std::min(remwords,bufsize);

                                        inbuffer = ::libmaus2::autoarray::AutoArray<input_type>(toreadwords);

                                        uint64_t const bytesread = socket->read ( reinterpret_cast<char *>(inbuffer.get()), toreadwords * sizeof(input_type));
                                        assert ( bytesread % sizeof(input_type) == 0 );
                                        uint64_t const wordsread = bytesread / sizeof(input_type);

                                        fullqueue.enque(wordsread);

                                        running = (wordsread!=0);
                                }

                                return 0;
			}

			typedef AsynchronousGenericConcatClient<input_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			static ::libmaus2::autoarray::AutoArray<input_type> readArray(
				std::string const & servername,
				unsigned short const serverport,
				std::string const & inputfilename
			)
			{
				AsynchronousGenericConcatClient<input_type> in(servername,serverport,inputfilename,64*1024);
				uint64_t const n = in.totalwords;
				::libmaus2::autoarray::AutoArray<input_type> A(n,false);

				for ( uint64_t i = 0; i < n; ++i )
				{
					input_type v;
					bool ok = in.getNext(v);
					assert ( ok );
					A[i] = v;
				}

				return A;
			}

			AsynchronousGenericConcatClient(
				std::string const & servername,
				unsigned short const serverport,
				uint64_t const fileid,
				uint64_t const rbufsize,
				uint64_t const affinity
			)
			: bufsize(rbufsize),
			  totalwords (0),
			  totalwordsread(0),
			  pc(0),
			  pe(0)
			{
				socket = UNIQUE_PTR_MOVE(socket_ptr_type(new socket_type(serverport,servername.c_str())));
				socket->writeSingle<uint64_t>(fileid);
				totalwords = socket->readSingle<uint64_t>();
				emptyqueue.enque(0);
				start(affinity);
			}
			~AsynchronousGenericConcatClient()
			{
				join();
			}

			bool fillBuffer()
			{
			        uint64_t const wordsread = fullqueue.deque();

                                if ( wordsread == 0 )
                                        return false;

                                outbuffer = inbuffer;
                                pc = outbuffer.begin();
                                pe = pc + wordsread;

                                emptyqueue.enque(0);

                                return true;
			}

			bool getNext(input_type & word)
			{
				if ( pc == pe )
				{
				        if ( ! fillBuffer() )
				                return false;
				}

				word = *(pc++);
				totalwordsread++;

				return true;
			}
		};

        }
}
#endif
