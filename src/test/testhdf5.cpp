/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#include <libmaus/fastx/StreamFastQReader.hpp>
#include <libmaus/hdf5/HDF5Handle.hpp>
#include <libmaus/lz/GzipOutputStream.hpp>
#include <libmaus/parallel/LockedCounter.hpp>
#include <libmaus/parallel/SimpleThreadPool.hpp>
#include <libmaus/parallel/SimpleThreadPoolInterfaceEnqueTermInterface.hpp>
#include <libmaus/parallel/SimpleThreadPoolWorkPackageFreeList.hpp>
#include <libmaus/parallel/SimpleThreadWorkPackageDispatcher.hpp>

#include <sys/types.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* set of hdf paths we check for fastq data */
char const * fastq_paths[] =
{
	"/Analyses/Basecall_2D_000/BaseCalled_2D/Fastq",
	"/Analyses/Basecall_2D_000/BaseCalled_template/Fastq",
	"/Analyses/Basecall_2D_000/BaseCalled_complement/Fastq",
	0
};

struct FileCallback
{
	virtual ~FileCallback() {}
	virtual void operator()(std::string const & fn) = 0;
};

void enumFiles(std::string const & filename, FileCallback & FC)
{
	struct stat sb;
	if ( !stat(filename.c_str(),&sb) )
	{
		if ( S_ISDIR(sb.st_mode) )
		{
			DIR * dir = opendir(filename.c_str());
			if ( dir )
			{
				try
				{
					std::vector<std::string> fns;
					struct dirent * de;
					
					while ( (de = readdir(dir)) )
					{
						std::string const fn(de->d_name);
						if ( fn != "." && fn != ".." )
							fns.push_back(fn);
					}
					
					closedir(dir);
					dir = 0;
					
					for ( uint64_t i = 0; i < fns.size(); ++i )
						enumFiles(filename + "/" + fns[i],FC);
				}
				catch(...)
				{
					if ( dir )
						closedir(dir);
					dir = 0;
					throw;
				}
			}
			else
			{
				int error = errno;
				std::cerr << "WARNING: failed to opendir(" << filename << "): " << strerror(error) << std::endl;
			}
		}
		else if ( S_ISREG(sb.st_mode) )
		{
			FC(filename);
		}
	}
	else
	{
		int error = errno;
		std::cerr << "WARNING: failed to stat(" << filename << "): " << strerror(error) << std::endl;
	}
}

struct Fast5ToFastQWorkPackage : public libmaus::parallel::SimpleThreadWorkPackage
{
	typedef Fast5ToFastQWorkPackage this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

	uint64_t id;
	std::string filename;
	std::ostream * out;
	libmaus::parallel::PosixMutex * outmutex;
	std::string idsuffix;
	std::map<std::string, libmaus::util::Histogram::shared_ptr_type > * rlHhistograms;
	libmaus::parallel::PosixSpinLock * rlHhistogramsLock;
	volatile uint64_t * minexpstarttime;
	volatile uint64_t * maxexpstarttime;
	libmaus::parallel::PosixSpinLock * expstarttimelock;

	Fast5ToFastQWorkPackage() : libmaus::parallel::SimpleThreadWorkPackage()
	{}
	Fast5ToFastQWorkPackage(
		uint64_t const rid,
		std::string const & rfilename,
		std::ostream * rout,
		libmaus::parallel::PosixMutex * routmutex,
		std::string const & ridsuffix,
		std::map<std::string, libmaus::util::Histogram::shared_ptr_type > * rrlHhistograms,
		libmaus::parallel::PosixSpinLock * rrlHhistogramsLock,
		volatile uint64_t * rminexpstarttime,
		volatile uint64_t * rmaxexpstarttime,
		libmaus::parallel::PosixSpinLock * rexpstarttimelock,
		uint64_t const rpriority, uint64_t const rdispatcherid, uint64_t const rpackageid = 0
	)
	: libmaus::parallel::SimpleThreadWorkPackage(rpriority,rdispatcherid,rpackageid), id(rid), filename(rfilename), out(rout), outmutex(routmutex), idsuffix(ridsuffix),
	  rlHhistograms(rrlHhistograms), rlHhistogramsLock(rrlHhistogramsLock),
	  minexpstarttime(rminexpstarttime),
	  maxexpstarttime(rmaxexpstarttime),
	  expstarttimelock(rexpstarttimelock)
	{}                                                                                                                                                                                                

	char const * getPackageName() const
	{
		return "Fast5ToFastQWorkPackage";
	}
};

struct Fast5ToFastQWorkPackageReturnInterface
{
	virtual ~Fast5ToFastQWorkPackageReturnInterface() {}
	virtual void returnFast5ToFastQWorkPackage(Fast5ToFastQWorkPackage * ptr) = 0;
};

struct Fast5ToFastQWorkPackageFinishedInterface
{
	virtual ~Fast5ToFastQWorkPackageFinishedInterface() {}
	virtual void fast5ToFastQWorkPackageFinished() = 0;

};

// work package dispatcher for Fast5 to FastQ conversion
struct Fast5ToFastQWorkPackageDispatcher : public libmaus::parallel::SimpleThreadWorkPackageDispatcher
{
	Fast5ToFastQWorkPackageFinishedInterface & finishedInterface;
	Fast5ToFastQWorkPackageReturnInterface & returnInterface;
	
	Fast5ToFastQWorkPackageDispatcher(Fast5ToFastQWorkPackageFinishedInterface & rfinishedInterface, Fast5ToFastQWorkPackageReturnInterface & rreturnInterface)
	: finishedInterface(rfinishedInterface), returnInterface(rreturnInterface)
	{
	
	}

	void dispatch(libmaus::parallel::SimpleThreadWorkPackage * P, libmaus::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
	{
		Fast5ToFastQWorkPackage * F = dynamic_cast<Fast5ToFastQWorkPackage *>(P);
		assert ( F );
		
		try
		{
			// load data into memory
			libmaus::autoarray::AutoArray<char> B = libmaus::autoarray::AutoArray<char>::readFile(F->filename);
			// open HDF5
			libmaus::hdf5::HDF5Handle::shared_ptr_type fhandle = libmaus::hdf5::HDF5Handle::createMemoryHandle(B.begin(),B.size());
			bool printed = false;
			
			// check for known FastQ paths
			for ( char const ** fqp = &fastq_paths[0]; *fqp; ++fqp )
				if ( fhandle->hasPath(*fqp) )
				{
					// get FastQ string
					std::string const fq = fhandle->getPath(*fqp)->datasetDecodeString();

					// get meta data
					std::string const channel = fhandle->getPath("/Analyses/Basecall_2D_000/Configuration/general")->decodeAttributeString("channel");
					std::string const timestamp = fhandle->getPath("/Analyses/Basecall_2D_000")->decodeAttributeString("time_stamp");
					std::string const expstarttime = fhandle->getPath("/UniqueGlobalKey/tracking_id")->decodeAttributeString("exp_start_time");
					std::string const eventsstarttime = fhandle->getPath("/Analyses/Basecall_2D_000/BaseCalled_template/Events")->decodeAttributeString("start_time");
					
					double dexpstarttime;
					std::istringstream sexpstarttime(expstarttime);
					sexpstarttime >> dexpstarttime;

					double deventsstarttime;
					std::istringstream seventsstarttime(eventsstarttime);
					seventsstarttime >> deventsstarttime;
					
					uint64_t uexpstarttime;
					std::istringstream suexpstarttime(expstarttime);
					suexpstarttime >> uexpstarttime;
					
					{
						libmaus::parallel::ScopePosixSpinLock lock(*(F->expstarttimelock));
						*(F->minexpstarttime) = std::min(static_cast<uint64_t>(*(F->minexpstarttime)),uexpstarttime);
						*(F->maxexpstarttime) = std::max(static_cast<uint64_t>(*(F->maxexpstarttime)),uexpstarttime);
					}
										
					// set up FastQ parser
					std::istringstream istr(fq);
					libmaus::fastx::StreamFastQReaderWrapper reader(istr);
					libmaus::fastx::StreamFastQReaderWrapper::pattern_type pattern;
					
					// read entries and modify name
					while ( reader.getNextPatternUnlocked(pattern) )
					{
						{
							libmaus::parallel::ScopePosixSpinLock lock(*(F->rlHhistogramsLock));
							std::map<std::string, libmaus::util::Histogram::shared_ptr_type> & M = 
								*(F->rlHhistograms);
							if ( M.find(*fqp) == M.end() )
								M[*fqp] = libmaus::util::Histogram::shared_ptr_type(
									new libmaus::util::Histogram
								);
							
							libmaus::util::Histogram & H = *(M.find(*fqp)->second);
							H(pattern.spattern.size());
						}
						
						if ( F->idsuffix.size() )
						{
							pattern.sid += "_";
							pattern.sid += F->idsuffix;
						}
						pattern.sid += "_";
						pattern.sid += libmaus::util::NumberSerialisation::formatNumber(F->id,6);
						pattern.sid += " seqlen=";
						pattern.sid += libmaus::util::NumberSerialisation::formatNumber(pattern.spattern.size(),0);
						pattern.sid += " path=";
						pattern.sid += *fqp;
						pattern.sid += " channel=";
						pattern.sid += channel;
						pattern.sid += " exp_start_time=";
						pattern.sid += expstarttime;
						pattern.sid += " events_start_time=";
						pattern.sid += eventsstarttime;
						pattern.sid += " timestamp=";
						pattern.sid += timestamp;
						
						// create output
						std::ostringstream blockout;
						blockout << pattern;
						std::string const outblock = blockout.str();

						// lock output stream and write data
						if ( ! printed )
						{
							libmaus::parallel::ScopePosixMutex smutex(*(F->outmutex));
							F->out->write(
								outblock.c_str(),
								outblock.size()
							);
							
							printed = true;
						}
					}					
				}
		}
		catch(std::exception const & ex)
		{
			std::cerr << ex.what() << std::endl;			
		}

		finishedInterface.fast5ToFastQWorkPackageFinished();
		returnInterface.returnFast5ToFastQWorkPackage(F);
	}
};

struct PrintFileCallback : public FileCallback, public Fast5ToFastQWorkPackageFinishedInterface, public Fast5ToFastQWorkPackageReturnInterface
{
	std::ostream & out;
	libmaus::parallel::PosixMutex outmutex;
	
	// number of files detected so far
	libmaus::parallel::LockedCounter c_in;
	// number of files finished
	libmaus::parallel::LockedCounter c_out;
	// id suffix for name line
	std::string idsuffix;
	// free list for packages
	libmaus::parallel::SimpleThreadPoolWorkPackageFreeList<Fast5ToFastQWorkPackage> packageFreeList;
	// thread pool
	libmaus::parallel::SimpleThreadPool STP;
	// package dispatcher
	Fast5ToFastQWorkPackageDispatcher fast5tofastqdispatcher;
	// dispatcher id
	uint64_t const fast5tofastqdispatcherid;
	// read length histograms
	std::map<std::string, libmaus::util::Histogram::shared_ptr_type > rlHhistograms;
	libmaus::parallel::PosixSpinLock rlHhistogramsLock;

	volatile uint64_t minexpstarttime;
	volatile uint64_t maxexpstarttime;
	libmaus::parallel::PosixSpinLock expstarttimelock;


	// work package finished callback
	void fast5ToFastQWorkPackageFinished()
	{
		c_out.increment();
	}

	// return work package callback
	void returnFast5ToFastQWorkPackage(Fast5ToFastQWorkPackage * ptr)
	{
		packageFreeList.returnPackage(ptr);	
	}
	
	// check whether we are done
	bool finished()
	{
		std::cerr << c_in << "\t" << c_out << std::endl;
		return static_cast<uint64_t>(c_in) == static_cast<uint64_t>(c_out);
	}
	
	// terminate thread pool
	void terminate()
	{
		STP.terminate();
	}
	
	// constructor
	PrintFileCallback(
		std::ostream & rout, 
		std::string const & ridsuffix,
		uint64_t const rnumthreads
		) 
	: out(rout), c_in(0), c_out(0), idsuffix(ridsuffix), STP(rnumthreads), fast5tofastqdispatcher(*this,*this), fast5tofastqdispatcherid(0),
	  minexpstarttime(std::numeric_limits<uint64_t>::max()), maxexpstarttime(0)
	{
		STP.registerDispatcher(fast5tofastqdispatcherid,&fast5tofastqdispatcher);
	}

	// aux function
	static bool endsOn(std::string const & s, std::string const & suf)
	{
		return
			s.size() >= suf.size() && 
			s.substr(s.size()-suf.size()) == suf;
	}

	// callback for newly detected input file
	void operator()(std::string const & fn)
	{
		if ( endsOn(fn,".fast5" ) )
		{
			Fast5ToFastQWorkPackage * package = packageFreeList.getPackage();
			*package = Fast5ToFastQWorkPackage(c_in.increment()-1,fn,&out,&outmutex,idsuffix,
				&rlHhistograms,&rlHhistogramsLock,
				&minexpstarttime, &maxexpstarttime, &expstarttimelock,
				0 /* prio */,fast5tofastqdispatcherid
			);
			STP.enque(package);
		}
	}

	void printHistograms()
	{
		for (
			std::map<std::string, libmaus::util::Histogram::shared_ptr_type >::iterator ita = rlHhistograms.begin();
			ita != rlHhistograms.end();
			++ita )
		{
			std::string const & name = ita->first;
			libmaus::util::Histogram & hist = *(ita->second);
			std::map<uint64_t,uint64_t> M = hist.get();
			uint64_t s = 0;
			
			for ( std::map<uint64_t,uint64_t>::const_iterator Mita = M.begin(); Mita != M.end(); ++Mita )
			{
				s += Mita->second;
				std::cerr << "[H]\t" << name << "\t" << Mita->first << "\t" << Mita->second << "\t" << s << std::endl;
			}
		}
	}
};

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		
		std::string const idsuffix = arginfo.getUnparsedValue("idsuffix","");
	
		std::ostream * Pout = &std::cout;
		libmaus::lz::GzipOutputStream::unique_ptr_type Pgz;
		if ( arginfo.getValue<int>("gz",0) )
		{
			int const level = arginfo.getValue<int>("level",Z_DEFAULT_COMPRESSION);
			libmaus::lz::GzipOutputStream::unique_ptr_type Tgz(new libmaus::lz::GzipOutputStream(*Pout,64*1024,level));
			Pgz = UNIQUE_PTR_MOVE(Tgz);
			Pout = Pgz.get();
		}
	
		int const threads= arginfo.getValue<int>("threads",1);
		PrintFileCallback PFC(*Pout,idsuffix,threads);
		for ( size_t i = 0; i < arginfo.restargs.size(); ++i )
			enumFiles(arginfo.restargs[i],PFC);
			
		while ( ! PFC.finished() )
			sleep(1);
			
		PFC.terminate();
		
		PFC.printHistograms();
		std::cerr << "minexpstarttime=" << PFC.minexpstarttime << std::endl;
		std::cerr << "maxexpstarttime=" << PFC.maxexpstarttime << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
