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
#if ! defined(LIBMAUS_BAMBAM_BAMTOFASTQOUTPUTFILESET_HPP)
#define LIBMAUS_BAMBAM_BAMTOFASTQOUTPUTFILESET_HPP

#include <map>
#include <string>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/lz/GzipOutputStream.hpp>
#include <libmaus/util/ArgInfo.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * output file set for BAM to FastQ conversion
		 **/
		struct BamToFastqOutputFileSet
		{
			//! map of all files
			std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type > files;
			//! map of gzip compressed files
			std::map < std::string, libmaus::lz::GzipOutputStream::shared_ptr_type> gzfiles;
			//! first mates file
			std::ostream & Fout;
			//! second mates file
			std::ostream & F2out;
			//! orphan read 1 file
			std::ostream & Oout;
			//! orphan read 2 file
			std::ostream & O2out;
			//! single ended file
			std::ostream & Sout;
			
			static uint64_t getGzipBufferSize()
			{
				return 256*1024;
			}
			
			static std::vector<std::string> getFileArgs()
			{
				std::vector<std::string> V;
				char const * fileargs[] = { "F", "F2", "O", "O2", "S", 0 };
				
				for ( char const ** filearg = &fileargs[0]; *filearg; ++filearg )
					V.push_back(*filearg);
			
				return V;
			}
			
			/**
			 * open output files
			 **/
			static std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type > openFiles(libmaus::util::ArgInfo const & arginfo)
			{
				std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type > files;
				std::vector<std::string> const fileargs = getFileArgs();
				
				for ( uint64_t i = 0; i < fileargs.size(); ++i )
				{
					std::string const filearg = fileargs[i];
					std::string const fn = arginfo.getValue<std::string>(filearg,"-");
					
					if ( fn != "-" && files.find(fn) == files.end() )
						files [ fn ] = libmaus::aio::CheckedOutputStream::shared_ptr_type(new libmaus::aio::CheckedOutputStream(fn));
				}

				return files;	
			}

			/**
			 * open gzipped files
			 **/
			static std::map < std::string, libmaus::lz::GzipOutputStream::shared_ptr_type> openGzFiles(
				std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type > & files,
				libmaus::util::ArgInfo const & arginfo
			)
			{
				std::map < std::string, libmaus::lz::GzipOutputStream::shared_ptr_type> gzfiles;

				bool stdoutactive = false;
				std::vector<std::string> const fileargs = getFileArgs();				
				for ( uint64_t i = 0; i < fileargs.size(); ++i )
					if ( arginfo.getValue<std::string>(fileargs[i],"-") == "-" )
						stdoutactive = true;
			
				if ( arginfo.getValue<int>("gz",0) )
				{
					int const level = std::min(9,std::max(-1,arginfo.getValue<int>("level",Z_DEFAULT_COMPRESSION)));
					
					for ( std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type >::const_iterator ita = files.begin();
						ita != files.end(); ++ita )
						gzfiles[ita->first] = libmaus::lz::GzipOutputStream::shared_ptr_type(new libmaus::lz::GzipOutputStream(*(ita->second),getGzipBufferSize(),level));
						
					if ( stdoutactive )
						gzfiles["-"] = libmaus::lz::GzipOutputStream::shared_ptr_type(new libmaus::lz::GzipOutputStream(std::cout,getGzipBufferSize(),level));
				}
				
				return gzfiles;
			}
			
			/**
			 * get file for identifier opt
			 *
			 * @param arginfo argument key=value pairs
			 * @param opt bamtofastq option (one of "F","F2","O","O2","S")
			 * @param files file map
			 * @return output stream 
			 **/
			static std::ostream & getFile(
				libmaus::util::ArgInfo const & arginfo, 
				std::string const opt, 
				std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type > & files,
				std::map < std::string, libmaus::lz::GzipOutputStream::shared_ptr_type> & gzfiles
			)
			{
				std::string const fn = arginfo.getValue<std::string>(opt,"-");
				bool const gz = arginfo.getValue<int>("gz",0);
				
				if ( gz )
				{
					assert ( gzfiles.find(fn) != gzfiles.end() );
					return *(gzfiles.find(fn)->second);
				}
				else
				{
					if ( fn == "-" )
						return std::cout;
					else
					{
						assert ( files.find(fn) != files.end() );
						return *(files.find(fn)->second);
					}
				}
			}
			
			/**
			 * construct from ArgInfo object
			 *
			 * @param arginfo arguments object
			 **/
			BamToFastqOutputFileSet(libmaus::util::ArgInfo const & arginfo)
			: files(openFiles(arginfo)), 
			  gzfiles(openGzFiles(files,arginfo)),
			  Fout( getFile(arginfo,"F",files,gzfiles) ),
			  F2out( getFile(arginfo,"F2",files,gzfiles) ),
			  Oout( getFile(arginfo,"O",files,gzfiles) ),
			  O2out( getFile(arginfo,"O2",files,gzfiles) ),
			  Sout( getFile(arginfo,"S",files,gzfiles) )
			{
			} 
			
			/**
			 * destructor, flush and close files
			 **/
			~BamToFastqOutputFileSet()
			{
				Fout.flush(); F2out.flush();
				Oout.flush(); O2out.flush();
				Sout.flush();

				for ( std::map < std::string, libmaus::lz::GzipOutputStream::shared_ptr_type>::iterator ita = gzfiles.begin();
					ita != gzfiles.end(); ++ita )
				{
					ita->second->flush();
					ita->second.reset();
				}				
				
				for (
					std::map < std::string, libmaus::aio::CheckedOutputStream::shared_ptr_type >::iterator ita = files.begin();
					ita != files.end(); ++ita 
				)
				{
					ita->second->flush();
					ita->second->close();
					ita->second.reset();
				}
			}
		};
	}
}
#endif
