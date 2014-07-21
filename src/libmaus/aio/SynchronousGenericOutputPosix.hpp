/*
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
*/
#if ! defined(LIBMAUS_AIO_SYNCHRONOUSGENERICOUTPUTPOSIX_HPP)
#define LIBMAUS_AIO_SYNCHRONOUSGENERICOUTPUTPOSIX_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/aio/PutOutputIterator.hpp>
#include <string>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
                    
namespace libmaus
{
	namespace aio
	{
		/**
		 * synchronous block oriented output based on posix system calls
		 **/
		template<typename _data_type>
		struct SynchronousGenericOutputPosix
		{
			//! data type
			typedef _data_type data_type;
			//! this type
			typedef SynchronousGenericOutputPosix<data_type> this_type;
			//! unique pointer type
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! iterator type
			typedef PutOutputIterator<data_type,this_type> iterator_type;
                
			private:
			//! file name
			std::string const filename;
			//! directory name
			std::string const dirname;
			//! run sync for meta information
			bool const metasync;
                
			//! output buffer
                        ::libmaus::autoarray::AutoArray<data_type> B;
                        //! output buffer begin pointer
                        data_type * const pa;
                        //! output buffer current pointer
                        data_type * pc;
                        //! output buffer end pointer
                        data_type * const pe;
                        //! file descriptor
                        int fd;
                        
                        //! total bytes written
                        uint64_t totalwrittenbytes;
                        //! total words written
                        uint64_t totalwrittenwords;

			//! deactivated assignment operator (private and unimplemented)
                        this_type & operator=(this_type const &);
                        SynchronousGenericOutputPosix(this_type const &);

                        /**
                         * write the buffer contents to the output file
                         **/
                        void writeBuffer()
                        {
                                char const * ca = reinterpret_cast<char const *>(pa);
                                char const * cc = reinterpret_cast<char const *>(pc);
                                
                                while ( ca != cc )
                                {
	                                ssize_t const written = write ( fd, ca, cc-ca );
	                                
	                                if ( written < 0 )
	                                {
	                                	switch ( errno )
	                                	{
	                                		case EINTR:
	                                			std::cerr << "Restarting write() call after interuption by signal." << std::endl;
	                                			break;
							default:
							{
								::libmaus::exception::LibMausException se;
								se.getStream() << "Failed to write in SynchronousGenericOutputPosix::writeBuffer(): " << strerror(errno)
									<< " fd=" << fd << " filename=" << filename << " dirname=" << dirname << std::endl;
								se.finish();
								throw se;							
							}
	                                	}
	                                }
	                                else
	                                {
	                                	ca += written;
	                                }
                                }

                                totalwrittenbytes += (pc-pa)*sizeof(data_type);
                                totalwrittenwords += (pc-pa);
                                pc = pa;
                                
                                #if 0
                                ssize_t written = write ( fd, ca, cc-ca );
                                
                                if ( written != cc-ca )
                                {
                                        ::libmaus::exception::LibMausException se;
                                        se.getStream() << "Failed to write in SynchronousGenericOutputPosix::writeBuffer(): " << strerror(errno)
                                        	<< " fd=" << fd << " filename=" << filename << " dirname=" << dirname << std::endl;
                                        se.finish();
                                        throw se;                              
                                }
                                
                                assert ( (cc-ca) % sizeof(data_type) == 0 );
                                totalwrittenbytes += (cc-ca);
                                totalwrittenwords += ((cc-ca)/sizeof(data_type));
                                
                                pc = pa;
                                #endif
                        }

                        
                        public:
                        /**
                         * get sum of file lengths for a list of files
                         *
                         * @param filenames list of files
                         * @return sum of the lengths of the files in list filenames
                         **/
                        static uint64_t getFileSize(std::vector<std::string> const & filenames)
                        {
                        	uint64_t s = 0;
                        	for ( uint64_t i = 0; i < filenames.size(); ++i )
                        		s += getFileSize(filenames[i]);
				return s;
                        }
                        /**
                         * get file size for a single file filename
                         *
                         * @param filename name of file
                         * @return length of file filename
                         **/
                        static uint64_t getFileSize(std::string const & filename)
                        {
                        	struct stat sb;
                        	
                        	while ( ::stat(filename.c_str(),&sb) != 0 )
                        	{
                        		switch ( errno )
                        		{
                        			case EINTR:
                        				std::cerr << "Restarting stat() in SynchronousGenericOutputPosix::getFileSize() after interuption by signal." << std::endl;
                        				break;
						default:
						{
							::libmaus::exception::LibMausException se;
							se.getStream() << "Failed to stat() file " << filename << " in SynchronousGenericOutputPosix::getFileSize(): " << strerror(errno) << std::endl;
							se.finish();
							throw se;
						}
                        		}
                        	}
                        	
                        	return sb.st_size;
                        }
                        
                        /**
                         * write array A to file outputfilename
                         *
                         * @param A array to be written
                         * @param outputfilename name of output file
                         **/
			static void writeArray(::libmaus::autoarray::AutoArray<data_type> const & A, 
				std::string const & outputfilename)
			{
				this_type out(outputfilename,64*1024);
				
				for ( uint64_t i = 0; i < A.getN(); ++i )
					out.put(A[i]);
				
				out.flush();
			}

			/**
			 * get offset for appending to file filename
			 * 
			 * @param filename name of output file
			 * @return offset for appending to file
			 **/
			static uint64_t appendOffset(std::string const & filename)
			{
			        if ( ::libmaus::util::GetFileSize::fileExists(filename) )
                                        return getFileSize(filename);
                                else
			                return 0;
			}

			/**
			 * check whether appending and truncating is the same for file filename
			 *
			 * @param filename output file name
			 * @return true iff file does not exist
			 **/
			static uint64_t appendTruncate(std::string const & filename)
			{
			        if ( ::libmaus::util::GetFileSize::fileExists(filename) )
			                return false;
                                else
                                        return true;
			}
			
			/**
			 * instantiate object for a new file
			 *
			 * @param filename name of output file
			 * @param bufsize size of output buffer
			 * @param metasync true if flushing should include the syncing of meta data
			 * @return unique pointer object containg a pointer to the new object
			 **/
			static unique_ptr_type instantiateNewFile(std::string const & filename, uint64_t const bufsize, bool const metasync = true)
			{
				return UNIQUE_PTR_MOVE(unique_ptr_type(new this_type(filename,bufsize,true,0,metasync)));
			}

			public:
			/**
			 * constructor
			 *
			 * @param rfilename output file name
			 * @param bufsize size of output buffer in elements
			 * @param truncate if true file will be truncated
			 * @param offset initial write offset
			 * @param rmetasync if true flushing will also sync meta data
			 **/
                        SynchronousGenericOutputPosix(
                                std::string const & rfilename, 
                                uint64_t const bufsize, 
                                bool const truncate,
                                uint64_t const offset,
                                bool const rmetasync = true
                        )
                        : filename(rfilename), dirname(::libmaus::util::ArgInfo::getDirName(filename)), metasync(rmetasync),
                          B(bufsize), pa(B.get()), pc(pa), pe(pa+B.getN()), 
                          fd ( -1 ),
                          totalwrittenbytes(0), totalwrittenwords(0)
                        {
                                while ( (fd = ::open(filename.c_str(), truncate ? (O_WRONLY|O_TRUNC|O_CREAT) : O_WRONLY , 0755 )) < 0 )
                                {
                                	switch ( errno )
                                	{
                                		case EINTR:
                                		{
                                			std::cerr << "Restarting open() system call interrupted by signal." << std::endl;
                                			break;
                                		}
                                		default:
                                		{
		                                        ::libmaus::exception::LibMausException se;
        		                                se.getStream() << "Failed to open file "<< filename <<" in SynchronousGenericOutputPosix: " <<
                		                                strerror(errno);
                        		                se.finish();
                                		        throw se;
						}
					}
                                }
                                if ( lseek ( fd, offset, SEEK_SET) == static_cast<off_t>(-1) )
                                {
                                        close(fd);
                                        ::libmaus::exception::LibMausException se;
                                        se.getStream() << "Failed to seek " << filename << " in SynchronousGenericOutputPosix: " <<
                                                strerror(errno);
                                        se.finish();
                                        throw se;
                                }
                                
                                #if 0
                                std::cerr << "File " << filename << " opened for output in "
                                        << ::libmaus::util::Demangle::demangle<this_type>() << std::endl;
                                #endif
                        }
                        
                        public:
                        /**
                         * destructor, close file
                         **/
                        ~SynchronousGenericOutputPosix()
                        {
                                flush();
                        
                                while ( close(fd) < 0 )
                                {
                                	switch ( errno )
                                	{
                                		case EINTR:
                                		{
                                			std::cerr << "Restarting close() after interuption by signal." << std::endl;
                                			break;
                                		}
                                		default:
                                		{
		                                        ::libmaus::exception::LibMausException se;
        		                                se.getStream() << "Failed to close in ~SynchronousGenericOutputPosix:: " << strerror(errno);
                		                        se.finish();
                        		                throw se;                                                                                                      
						}
					}
                                }
                                
                                #if 0
                                std::cerr << "File " << filename << " closed succesfully." << std::endl;
                                #endif
                        }
                        
                        /**
                         * flush meta information by syncing the meta information on the directory 
                         * contaning our output file
                         **/
                        void dirflush()
                        {                        
                                int dirfd = -1;
                                
                                try
                                {
                                        while ( (dirfd = open(dirname.c_str(),0)) < 0 )
                                        {
                                        	switch ( errno )
                                        	{
                                        		case EINTR:
                                        		{
                                        			std::cerr << "Restarting open() call for directory interrupted by signal." << std::endl;
                                        			break;
							}
							default:
							{
		                                                ::libmaus::exception::LibMausException se;
        		                                        se.getStream() << "Failed to open directory " << dirname << " in SynchronousGenericOutputPosix::flush(): " << strerror(errno);
                		                                se.finish();
                        		                        throw se;                                                                                                              
							}
						}
                                        }
                                        
                                        if ( fsync(dirfd) < 0 )
                                        {
                                        	std::cerr << "Failed to fsync directory " << dirname << " in SynchronousGenericOutputPosix::flush(): " << strerror(errno) << std::endl;
                                        }

                                        while ( close(dirfd) < 0 )
                                        {
                                        	switch ( errno )
                                        	{
                                        		case EINTR:
                                        		{
                                        			std::cerr << "Restarting close() call for directory interrupted by signal." << std::endl;
                                        			break;                                        		
                                        		}
                                        		default:
                                        		{
		                                                ::libmaus::exception::LibMausException se;
        		                                        se.getStream() << "Failed to close directory " << dirname << " in SynchronousGenericOutputPosix::flush(): " << strerror(errno);
                		                                se.finish();
                        		                        throw se;                                                              
							}
						}
                                        }
                                        
                                        dirfd = -1;
                                }
                                catch(...)
                                {
                                        while ( close(dirfd) < 0 )
                                        {
                                        	switch ( errno )
                                        	{
                                        		case EINTR:
                                        		{
                                        			std::cerr << "Restarting close() call for directory interrupted by signal." << std::endl;
                                        			break;                                        		
                                        		}
                                        		default:
                                        		{
		                                                ::libmaus::exception::LibMausException se;
        		                                        se.getStream() << "Failed to close directory " << dirname << " in SynchronousGenericOutputPosix::flush(): " << strerror(errno);
                		                                se.finish();
                        		                        throw se;                                                              
							}
						}
                                        }

                                        throw;
                                }
                        }

                        /**
                         * flush output file
                         **/
                        void flush()
                        {
                                writeBuffer();
                                
                                while ( fsync(fd) != 0 )
                                {
                                	switch ( errno )
                                	{
                                		case EINTR:
                                		{
                                			std::cerr << "Restarting fsync after interuption by signal." << std::endl;
                                			break;
						}
                                		default:
                                		{
		                                        ::libmaus::exception::LibMausException se;
        		                                se.getStream() << "Failed to fsync in SynchronousGenericOutputPosix::flush(): " << strerror(errno);
                		                        se.finish();
                        		                throw se;                                                              
						}
					}
                                }
                                
                                dirflush();
                                
                                if ( metasync )
                                {
                                        off_t const cursize = lseek(fd,0,SEEK_CUR);
                                        
                                        if ( cursize == static_cast<off_t>(-1) )
                                        {
                                                ::libmaus::exception::LibMausException se;
                                                se.getStream() << "Failed to lseek in SynchronousGenericOutputPosix::flush(): " << strerror(errno);
                                                se.finish();
                                                throw se;                                                              
                                        }

                                        uint64_t okcnt = 0;
                                        uint64_t totalcnt = 0;
                                        uint64_t const desokcnt = 16;
                                        uint64_t const maxtotalcnt = 1024;

                                        off_t filesize = -1;
                                        
                                        for ( ; okcnt < desokcnt && totalcnt < maxtotalcnt; ++totalcnt )
                                        {
                                                dirflush();
                                                
                                                filesize = getFileSize(filename);
                                                                       
                                                if ( filesize == cursize )
                                                {
                                                	okcnt++;
                                                }
                                                else
                                                {
                                                	okcnt = 0;
                                                        std::cerr << "Waiting for meta data update of file " 
                                                                << filename << " in SynchronousGenericOutputPosix::flush(), " 
                                                                << " expected " << cursize 
                                                                << " but systems reports " << filesize 
                                                                << " okcnt " << okcnt
                                                                << " totalcnt " << totalcnt
                                                                << std::endl;
                                                        sleep(1);
                                                }
                                        }

					if ( okcnt < desokcnt )
					{
                                                ::libmaus::exception::LibMausException se;
                                                se.getStream() << "Failed to flush file " << filename 
                                                	<< " in SynchronousGenericOutputPosix::flush()" 
                                                	<< " file size reported is "
                                                	<< filesize
                                                	<< " but we expected " << cursize
                                                	<< std::endl;
                                                se.finish();
                                                throw se;                                                              
                                        }
                                }
                        }

                        /**
                         * put one element c in the output buffer
                         *
                         * @param c element to be put in the buffer
                         **/
                        void put(data_type const c)
                        {
                                *(pc++) = c;
                                if ( pc == pe )
                                        writeBuffer();
                        }
                        
                        /**
                         * @return number of words written
                         **/
                        uint64_t getWrittenWords() const
                        {
                                return (pc-pa)+totalwrittenwords;
                        }
                        
                        /**
                         * @return number of bytes written
                         **/
                        uint64_t getWrittenBytes() const
                        {
                                return (pc-pa)*sizeof(data_type)+totalwrittenbytes;
                        }
                };
                
	}
}
#endif
