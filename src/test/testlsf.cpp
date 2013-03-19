/**
    libmaus
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
**/
#include <libmaus/lsf/SimpleDispatchedLsfProcessSet.hpp>

void testBunch(::libmaus::util::ArgInfo const & arginfo)
{
	try
	{
		std::cerr << "--- testing bunch setting ---" << std::endl;
	
		bool const verbose = arginfo.getValue<uint64_t>("verbose",0);
		::libmaus::lsf::SimpleDispatchedLsfProcessSet SDLPS("testlsf",verbose);
		uint64_t const numprocs = arginfo.getValue<uint64_t>("numprocs",8);
		std::vector < uint64_t > const ids = SDLPS.startProcessesAndWait(numprocs/* wait for all */,numprocs,arginfo,"testlsfdispatcher","testlsfclient",1,100);
	
		std::cerr << "active size is " << SDLPS.active.size() << std::endl;
	
		// sleep(30);	
		// sleep(1);
		
		for ( uint64_t i = 0; i < ids.size(); ++i )
		{
			uint64_t const id = ids[i];
			
			while ( ! SDLPS.isActive(id) && ! SDLPS.isFinished(id) )
			{
				sleep(1);
			}
			
			if ( SDLPS.isActive(id) )
			{
				try
				{
					::libmaus::lsf::SimpleDispatchedLsfProcessSet::proc_ptr proc = SDLPS.process(id);
					assert ( proc );
					assert ( proc->controlsocket );
					std::string const mes = proc->controlsocket->readString();
					proc->controlsocket->barrierWr();
					std::cerr << "Got message " << mes << " from process " << id << " hostname " << proc->hostname << std::endl;
				}
				catch(std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
				}
				
				SDLPS.deactivate(id);
			}
			else
			{
				std::cerr << "Process " << id << " seems to have failed before we talked to it." << std::endl;
			}
		}
		
		while ( SDLPS.numFinished() < ids.size() )
			sleep(1);
	
	}
	catch(std::exception const & ex)
	{
	
	}
}

struct TestLsfTaskQueue : public ::libmaus::lsf::LsfTaskQueue
{
	std::deque < uint64_t > Q;
	std::vector < std::string > payloads;
	uint64_t curproc; // number of jobs currently running
	
	TestLsfTaskQueue(uint64_t const taskcnt)
	: curproc(0)
	{
		for ( uint64_t i = 0; i < taskcnt; ++i )
		{
			std::ostringstream ostr;
			ostr << i;
			Q.push_back(i);
			payloads.push_back(ostr.str());
		}
	}
	
	bool empty() { return Q.empty(); }
	uint64_t next() { uint64_t const id = Q.front(); Q.pop_front(); curproc++; return id; }
	std::string payload(uint64_t const i) { return payloads[i]; }
	void putback(uint64_t const id) { Q.push_front(id); curproc--; }
	void finished(uint64_t const id, std::string reply) { 
		// std::cerr << "*** id " << id << " finished." << std::endl;
		std::cerr << "*** id " << id << " finished, reply is " << reply << std::endl;
		curproc--;
	}

	virtual std::string packettype(uint64_t const /* i */)
	{
		return "sleeppacket";
	}
        virtual std::string termpackettype()
        {
        	return "termpacket";
        }
        
        virtual uint64_t numUnfinished()
        {
        	return Q.size() + curproc;
        }
};

void testFarmerWorker(::libmaus::util::ArgInfo const & arginfo)
{
	try
	{
		std::cerr << "--- testing farmer/worker setting ---" << std::endl;
		
		bool const verbose = arginfo.getValue<uint64_t>("verbose",0);
		::libmaus::lsf::SimpleDispatchedLsfProcessSet SDLPS("testlsf",verbose);
		uint64_t const numprocs = arginfo.getValue<uint64_t>("numprocs",8);
					
		uint64_t const taskcnt = 128;
		TestLsfTaskQueue TLTQ(taskcnt);

		SDLPS.processQueue(TLTQ,arginfo,numprocs,"testlsfdispatcher","testlsfclientfw",100 /* mem */);
	}
	catch(std::exception const & ex)
	{
	
	}
}

int main(int argc, char ** argv)
{
	try
	{
		::libmaus::util::ArgInfo const arginfo(argc,argv);
		::libmaus::lsf::LSF::init(arginfo.progname);

		testFarmerWorker(arginfo);
		// testBunch(arginfo);

		#if 0
		::libmaus::network::LogReceiver::unique_ptr_type LR(new ::libmaus::network::LogReceiver("testlsf",4444,1024));
		
		std::vector < DispatchedLsfProcess::shared_ptr_type > procs = 
			DispatchedLsfProcess::start(arginfo,LR->sid,LR->hostname,LR->port,8/*id*/,"testlsfdispatcher","testlsfclient",1/* numthreads*/,100/*mem*/,0/*hosts*/,0/*cwd*/,0/*tmpspace*/,false/*valgrind*/);
			
		DispatchedLsfProcess::join(procs);

		#if 0
		DispatchedLsfProcess proc(arginfo,LR->sid,LR->hostname,LR->port,0/*id*/,"testlsfdispatcher","testlsfclient",1/* numthreads*/,100/*mem*/,0/*hosts*/,0/*cwd*/,0/*tmpspace*/,false/*valgrind*/);
		
		proc.waitKnown();

		while ( proc.isUnfinished() )
		{
			std::cerr << "Waiting for process to finish." << std::endl;
			sleep(1);
		}
		#endif	
		#endif
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
