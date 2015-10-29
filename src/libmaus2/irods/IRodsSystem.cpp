/*
    libmaus2
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
#include <libmaus2/irods/IRodsSystem.hpp>
#include <csignal>

libmaus2::irods::IRodsSystem::shared_ptr_type libmaus2::irods::IRodsSystem::defaultIrodsSystem;
libmaus2::parallel::PosixMutex libmaus2::irods::IRodsSystem::defaultIrodsSystemLock;

libmaus2::irods::IRodsSystem::shared_ptr_type libmaus2::irods::IRodsSystem::getDefaultIRodsSystem()
{
	libmaus2::parallel::ScopePosixMutex llock(defaultIrodsSystemLock);
	if ( !defaultIrodsSystem )
	{
		IRodsSystem::shared_ptr_type tptr(new IRodsSystem);
		defaultIrodsSystem = tptr;
	}
	
	return defaultIrodsSystem;
}


libmaus2::irods::IRodsFileBase::unique_ptr_type libmaus2::irods::IRodsSystem::openFile(
	#if defined(LIBMAUS2_HAVE_IRODS)
	IRodsSystem::shared_ptr_type irodsSystem, 
	std::string const & filename
	#else
	IRodsSystem::shared_ptr_type, 
	std::string const & filename
	#endif
)
{
	#if defined(LIBMAUS2_HAVE_IRODS)
	
	std::string const processed_filename = irodsSystem->setComm(filename); 
	
	IRodsFileBase::unique_ptr_type tptr(new IRodsFileBase);
	IRodsFileBase & file = *tptr;

	dataObjInp_t pathParseHandle;
	long status = -1;
	
	// erase dataObjInp
	memset (&pathParseHandle, 0, sizeof (pathParseHandle));
	
	// copy filename
	libmaus2::autoarray::AutoArray<char> filenameCopy(processed_filename.size()+1,false);
	std::copy(processed_filename.begin(),processed_filename.end(),filenameCopy.begin());
	filenameCopy[processed_filename.size()] = 0;
	
	// parse path
	if ( (status = parseRodsPathStr(filenameCopy.begin(),&(irodsSystem->irodsEnvironment),pathParseHandle.objPath)) < 0 )
	{
		char * subname = 0;
		char * name = rodsErrorName(status,&subname);
		
		libmaus2::exception::LibMausException lme;
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
	
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "IRodsSystem::openFile: failed to open file " << filename << " with error " << descriptor << " (" << name << ")" << std::endl;
		lme.finish();
		throw lme;		
	}
	
	file.fd           = descriptor;
	file.comm         = irodsSystem->getComm();
	file.fdvalid      = true;

	return UNIQUE_PTR_MOVE(tptr);
	#else
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "IRodsSystem::openFile: failed to open file " << filename << ": irods support not present" << std::endl;
	lme.finish();
	throw lme;		
	#endif
}


#if defined(LIBMAUS2_HAVE_IRODS)

std::string libmaus2::irods::IRodsSystem::setComm(std::string const & filename)
{
	std::string host;
	std::string zone;
	std::string user;
	int port;
	
	std::string file = parseIRodsURI(filename, host, zone, user, port);
	std::stringstream port_to_string;
	port_to_string << port;	
	
	std::string search = host + ":" + port_to_string.str();
	
	if ( comms.find(search) != comms.end() ) 
	{
	    	comm = comms[search];
	} 
	else
	{
	    	// open connection to iRODS server

	    	int status = -1;
	    	rErrMsg_t errorMessage;
	    	comm = rcConnect(
		        const_cast<char *>(host.c_str()), 
		    	port, 
		    	const_cast<char *>(user.c_str()),
		    	const_cast<char *>(zone.c_str()),
		    	0, &errorMessage
	    	);
	    
	    	// check if we were successful
		if ( ! comm )
		{
			char * subname = 0;
			char * name = rodsErrorName(errorMessage.status, &subname);

			libmaus2::exception::LibMausException lme;
			lme.getStream() << "IRodsSystem: failed to connect to iRODS via rcConnect(): "
				<< name << " (" << subname << ")" << "(" << errorMessage.status << ")" << " " << errorMessage.msg << "\n";
			lme.finish();
			throw lme;
		}

		// perform login
		if ( (status = clientLogin(comm)) < 0 )
		{
		    	// clean up the lingering connection
		    	rcDisconnect(comm);
		
			char * subname = 0;
			char * name = rodsErrorName(status, &subname);

			libmaus2::exception::LibMausException lme;
			lme.getStream() << "IRodsSystem: call clientLogin failed: " << status << " (" << name << ")\n";
			lme.finish();
			throw lme;		
		}

		// store the newly opened comm handle
		comms[search] = comm;
	}
	
	return file;
}

/*
    Provisional scheme for an iRODS uri looks like this.
    
    irods://[irodsUserName%23irodsZone@][irodsHost][:irodsPort]/collection_path/data_object
*/

std::string libmaus2::irods::IRodsSystem::parseIRodsURI(std::string const & uri, 
    	std::string & host, std::string & zone, std::string & user, int & port)
{
	// copy default connection info into local version    
    	host = std::string(irodsEnvironment.rodsHost);
	zone = std::string(irodsEnvironment.rodsZone);
	user = std::string(irodsEnvironment.rodsUserName);
        port = irodsEnvironment.rodsPort;
	
	// these should always be true if we have an auth section
	std::size_t auth      = uri.find("//");
	std::size_t tag_start = auth + std::string("//").length();
	std::size_t port_end  = uri.find("/", tag_start);

	if ( auth == std::string::npos || port_end == std::string::npos )
	{
	    	// nothing we can do with this so return untouched
	    	return uri;
	}

	// look for the user name
	std::size_t tag_end = uri.find("%23");

	if ( tag_end != std::string::npos ) 
	{
    	    	user  = uri.substr(tag_start, tag_end - tag_start);
	    	tag_start = tag_end + std::string("%23").length();
	}

	// look for zone
	tag_end = uri.find("@");

	if ( tag_end != std::string::npos ) 
	{
    	    	zone  = uri.substr(tag_start, tag_end - tag_start);
	    	tag_start = tag_end + 1;
	}

	// now the host and port
	tag_end = uri.find(":", auth);
	std::size_t host_tag = tag_end + 1;

	if ( tag_end != std::string::npos ) 
	{
    	    	host = uri.substr(tag_start, tag_end - tag_start);
    	    	port = stoi(uri.substr(host_tag, port_end - host_tag));
	} 
	else 
	{
    	    	host = uri.substr(tag_start, port_end - tag_start);
	}

    	// return location of the data object
    	return uri.substr(port_end);
}
#endif



libmaus2::irods::IRodsSystem::IRodsSystem() 
  #if defined(LIBMAUS2_HAVE_IRODS)
: 
  comm(0), 
  prevpipesighandler(SIG_DFL)
  #endif
{
	#if defined(LIBMAUS2_HAVE_IRODS)
	int status = -1;
	
	// read environment
	if ( (status = getRodsEnv(&irodsEnvironment)) < 0 )
	{
		libmaus2::exception::LibMausException lme;
		lme.getStream() << "IRodsSystem: failed to get iRODS environment via getRodsEnv(): " << status << std::endl;
		lme.finish();
		throw lme;
	}
	
	// reset SIG PIPE handler
	prevpipesighandler = signal(SIGPIPE, SIG_DFL);

	#else
	libmaus2::exception::LibMausException lme;
	lme.getStream() << "IRodsSystem: iRODS support is not present." << std::endl;
	lme.finish();
	throw lme;						
	#endif
}


libmaus2::irods::IRodsSystem::~IRodsSystem()
{
	#if defined(LIBMAUS2_HAVE_IRODS)
	if ( prevpipesighandler != SIG_DFL )
		signal(SIGPIPE, prevpipesighandler);
	
	// close connection
	for ( std::map<std::string, rcComm_t *>::iterator itr = comms.begin(); itr != comms.end(); ++itr ) 
	{
	    	rcDisconnect(itr->second);
	}
	#endif
}
