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
#include <libmaus/irods/IRodsSystem.hpp>
#include <csignal>

libmaus::irods::IRodsSystem::shared_ptr_type libmaus::irods::IRodsSystem::defaultIrodsSystem;
libmaus::parallel::PosixMutex libmaus::irods::IRodsSystem::defaultIrodsSystemLock;

libmaus::irods::IRodsSystem::shared_ptr_type libmaus::irods::IRodsSystem::getDefaultIRodsSystem()
{
	libmaus::parallel::ScopePosixMutex llock(defaultIrodsSystemLock);
	if ( !defaultIrodsSystem )
	{
		IRodsSystem::shared_ptr_type tptr(new IRodsSystem);
		defaultIrodsSystem = tptr;
	}
	
	return defaultIrodsSystem;
}

libmaus::irods::IRodsFileBase::unique_ptr_type libmaus::irods::IRodsSystem::openFile(
	#if defined(LIBMAUS_HAVE_IRODS)
	IRodsSystem::shared_ptr_type irodsSystem, 
	std::string const & filename
	#else
	IRodsSystem::shared_ptr_type, 
	std::string const & filename
	#endif
)
{
	#if defined(LIBMAUS_HAVE_IRODS)
	IRodsFileBase::unique_ptr_type tptr(new IRodsFileBase);
	IRodsFileBase & file = *tptr;

	dataObjInp_t pathParseHandle;
	long status = -1;
	
	// erase dataObjInp
	memset (&pathParseHandle, 0, sizeof (pathParseHandle));
	
	// copy filename
	libmaus::autoarray::AutoArray<char> filenameCopy(filename.size()+1,false);
	std::copy(filename.begin(),filename.end(),filenameCopy.begin());
	filenameCopy[filename.size()] = 0;
	
	// parse path
	if ( (status = parseRodsPathStr(filenameCopy.begin(),&(irodsSystem->irodsEnvironment),pathParseHandle.objPath)) < 0 )
	{
		char * subname = 0;
		char * name = rodsErrorName(status,&subname);
		
		libmaus::exception::LibMausException lme;
		lme.getStream() << "IRodsSystem::openFile: failed to parse filename " << filename << " via parseRodsPathStr, error code " << status << " (" << name << ")" << std::endl;
		lme.finish();
		throw lme;		
	}
	
	// read only
	pathParseHandle.openFlags = O_RDONLY;

	long const descriptor = rcDataObjOpen(irodsSystem->getComm(),&pathParseHandle);

	if ( descriptor < 0 )
	{
		char * subname = 0;
		char * name = rodsErrorName(descriptor,&subname);
	
		libmaus::exception::LibMausException lme;
		lme.getStream() << "IRodsSystem::openFile: failed to open file " << filename << " with error " << descriptor << " (" << name << ")" << std::endl;
		lme.finish();
		throw lme;		
	}
	
	file.fd           = descriptor;
	file.commProvider = irodsSystem;
	file.fdvalid      = true;

	return UNIQUE_PTR_MOVE(tptr);
	#else
	libmaus::exception::LibMausException lme;
	lme.getStream() << "IRodsSystem::openFile: failed to open file " << filename << ": irods support not present" << std::endl;
	lme.finish();
	throw lme;		
	#endif
}	

libmaus::irods::IRodsSystem::IRodsSystem() 
  #if defined(LIBMAUS_HAVE_IRODS)
: 
  comm(0), 
  prevpipesighandler(SIG_DFL)
  #endif
{
	#if defined(LIBMAUS_HAVE_IRODS)
	int status = -1;
	
	// read environment
	if ( (status = getRodsEnv(&irodsEnvironment)) < 0 )
	{
		libmaus::exception::LibMausException lme;
		lme.getStream() << "IRodsSystem: failed to get iRODS environment via getRodsEnv(): " << status << std::endl;
		lme.finish();
		throw lme;
	}
	
	// open connection to iRODS server
	rErrMsg_t errorMessage;
	comm = rcConnect(
		irodsEnvironment.rodsHost, 
		irodsEnvironment.rodsPort, 
		irodsEnvironment.rodsUserName,
		irodsEnvironment.rodsZone,0,
		&errorMessage
	);
	
	// check if we were successful
	if ( ! comm )
	{
		char * subname = 0;
		char * name = rodsErrorName(errorMessage.status, &subname);
		
		libmaus::exception::LibMausException lme;
		lme.getStream() << "IRodsSystem: failed to connect to iRODS via rcConnect(): "
			<< name << " (" << subname << ")" << "(" << errorMessage.status << ")" << " " << errorMessage.msg << "\n";
		lme.finish();
		throw lme;
	}
	
	// reset SIG PIPE handler
	prevpipesighandler = signal(SIGPIPE, SIG_DFL);

	// perform login
	if ( (status = clientLogin(comm)) < 0 )
	{
		char * subname = 0;
		char * name = rodsErrorName(status, &subname);
		
		libmaus::exception::LibMausException lme;
		lme.getStream() << "IRodsSystem: call clientLogin failed: " << status << " (" << name << ")\n";
		lme.finish();
		throw lme;		
	}
	#else
	libmaus::exception::LibMausException lme;
	lme.getStream() << "IRodsSystem: iRODS support is not present." << std::endl;
	lme.finish();
	throw lme;						
	#endif
}

libmaus::irods::IRodsSystem::~IRodsSystem()
{
	#if defined(LIBMAUS_HAVE_IRODS)
	if ( prevpipesighandler != SIG_DFL )
		signal(SIGPIPE, prevpipesighandler);
	
	// close connection
	rcDisconnect(comm);
	#endif
}
