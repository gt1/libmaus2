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
#include <libmaus/aio/LineSplittingPosixFdOutputStream.hpp>
#include <libmaus/aio/PosixFdOutputStream.hpp>
#include <libmaus/lz/LineSplittingGzipOutputStream.hpp>
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
			//! split files after this many lines (0 for never)
			uint64_t const split;
		
			typedef std::map < std::string, libmaus::aio::PosixFdOutputStream::shared_ptr_type > files_map_type;
			//! map of all files
			files_map_type files;
			
			typedef std::map < std::string, libmaus::lz::GzipOutputStream::shared_ptr_type> gz_files_map_type;
			//! map of gzip compressed files
			gz_files_map_type gzfiles;
			
			typedef std::map < std::string, libmaus::aio::LineSplittingPosixFdOutputStream::shared_ptr_type > split_files_map_type;
			//! map of all split files
			split_files_map_type splitfiles;

			typedef std::map < std::string, libmaus::lz::LineSplittingGzipOutputStream::shared_ptr_type > split_gz_files_map_type;
			//! map of all split gzip files
			split_gz_files_map_type splitgzfiles;

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
			static files_map_type openFiles(libmaus::util::ArgInfo const & arginfo)
			{
				files_map_type files;
				std::vector<std::string> const fileargs = getFileArgs();
				
				for ( uint64_t i = 0; i < fileargs.size(); ++i )
				{
					std::string const filearg = fileargs[i];
					std::string const fn = arginfo.getValue<std::string>(filearg,"-");
					
					if ( fn != "-" && files.find(fn) == files.end() )
						files [ fn ] = libmaus::aio::PosixFdOutputStream::shared_ptr_type(new libmaus::aio::PosixFdOutputStream(fn));
				}

				return files;	
			}

			/**
			 * open gzipped files
			 **/
			static gz_files_map_type openGzFiles(
				files_map_type & files,
				libmaus::util::ArgInfo const & arginfo,
				uint64_t const split
			)
			{
				gz_files_map_type gzfiles;

				bool stdoutactive = false;
				std::vector<std::string> const fileargs = getFileArgs();				
				for ( uint64_t i = 0; i < fileargs.size(); ++i )
					if ( arginfo.getValue<std::string>(fileargs[i],"-") == "-" )
						stdoutactive = true;
			
				if ( arginfo.getValue<int>("gz",0) )
				{
					int const level = std::min(9,std::max(-1,arginfo.getValue<int>("level",Z_DEFAULT_COMPRESSION)));
					
					if ( ! split )
						for ( files_map_type::const_iterator ita = files.begin();
							ita != files.end(); ++ita )
							gzfiles[ita->first] = libmaus::lz::GzipOutputStream::shared_ptr_type(new libmaus::lz::GzipOutputStream(*(ita->second),getGzipBufferSize(),level));
						
					if ( stdoutactive )
						gzfiles["-"] = libmaus::lz::GzipOutputStream::shared_ptr_type(new libmaus::lz::GzipOutputStream(std::cout,getGzipBufferSize(),level));
				}
				
				return gzfiles;
			}

			/**
			 * open split files
			 **/
			static split_files_map_type openSplitFiles(libmaus::util::ArgInfo const & arginfo, uint64_t const split)
			{
				assert ( split != 0 );
			
				split_files_map_type splitfiles;
				std::vector<std::string> const fileargs = getFileArgs();
				bool const fasta = arginfo.getValue<unsigned int>("fasta",0);
				uint64_t const mult = fasta ? 2 : 4;
				
				if ( arginfo.getValue<int>("gz",0) == 0 )
					for ( uint64_t i = 0; i < fileargs.size(); ++i )
					{
						std::string const filearg = fileargs[i];
						std::string const fn = arginfo.getValue<std::string>(filearg,"-");
						
						if ( fn != "-" && splitfiles.find(fn) == splitfiles.end() )
							splitfiles [ fn ] = libmaus::aio::LineSplittingPosixFdOutputStream::shared_ptr_type(
								new libmaus::aio::LineSplittingPosixFdOutputStream(fn,mult*split));
					}

				return splitfiles;	
			}

			/**
			 * open split gz files
			 **/
			static split_gz_files_map_type openSplitGzFiles(libmaus::util::ArgInfo const & arginfo, uint64_t const split)
			{
				assert ( split != 0 );
			
				split_gz_files_map_type splitgzfiles;
				std::vector<std::string> const fileargs = getFileArgs();
				
				if ( arginfo.getValue<int>("gz",0) == 1 )
				{
					int const level = std::min(9,std::max(-1,arginfo.getValue<int>("level",Z_DEFAULT_COMPRESSION)));
					bool const fasta = arginfo.getValue<unsigned int>("fasta",0);
					uint64_t const mult = fasta ? 2 : 4;
					
					for ( uint64_t i = 0; i < fileargs.size(); ++i )
					{
						std::string const filearg = fileargs[i];
						std::string const fn = arginfo.getValue<std::string>(filearg,"-");
						
						if ( fn != "-" && splitgzfiles.find(fn) == splitgzfiles.end() )
							splitgzfiles [ fn ] = libmaus::lz::LineSplittingGzipOutputStream::shared_ptr_type(
								new libmaus::lz::LineSplittingGzipOutputStream(fn,mult*split,64*1024,level));
					}
				}

				return splitgzfiles;	
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
				files_map_type & files,
				gz_files_map_type & gzfiles,
				split_files_map_type & splitfiles,
				split_gz_files_map_type & splitgzfiles
			)
			{
				std::string const fn = arginfo.getValue<std::string>(opt,"-");
				
				if ( splitgzfiles.find(fn) != splitgzfiles.end() )
				{
					#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
					std::cerr << "returning split gz file for " << opt << std::endl;
					#endif
					return *(splitgzfiles.find(fn)->second);
				}
				else if ( splitfiles.find(fn) != splitfiles.end() )
				{
					#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
					std::cerr << "returning split file for " << opt << std::endl;
					#endif
					return *(splitfiles.find(fn)->second);
				}
				else if ( arginfo.getValue<int>("gz",0) )
				{
					#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
					std::cerr << "returning gz file for " << opt << std::endl;
					#endif
					assert ( gzfiles.find(fn) != gzfiles.end() );
					return *(gzfiles.find(fn)->second);
				}
				else
				{
					if ( fn == "-" )
					{
						#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
						std::cerr << "returning std::cout for " << opt << std::endl;
						#endif
						return std::cout;
					}
					else
					{
						#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
						std::cerr << "returning file for " << opt << std::endl;
						#endif
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
			: split(arginfo.getValueUnsignedNumeric("split",0)),
			  files(split ? files_map_type() : openFiles(arginfo)), 
			  gzfiles(openGzFiles(files,arginfo,split)),
			  splitfiles(split ? openSplitFiles(arginfo,split) : split_files_map_type()),
			  splitgzfiles(split ? openSplitGzFiles(arginfo,split) : split_gz_files_map_type()),
			  Fout( getFile(arginfo,"F",files,gzfiles,splitfiles,splitgzfiles) ),
			  F2out( getFile(arginfo,"F2",files,gzfiles,splitfiles,splitgzfiles) ),
			  Oout( getFile(arginfo,"O",files,gzfiles,splitfiles,splitgzfiles) ),
			  O2out( getFile(arginfo,"O2",files,gzfiles,splitfiles,splitgzfiles) ),
			  Sout( getFile(arginfo,"S",files,gzfiles,splitfiles,splitgzfiles) )
			{
			} 
			
			/**
			 * destructor, flush and close files
			 **/
			~BamToFastqOutputFileSet()
			{
				#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
				std::cerr << "flushing Fout" << std::endl;
				#endif
				Fout.flush(); 
				#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
				std::cerr << "flushing F2out" << std::endl;
				#endif
				F2out.flush();
				#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
				std::cerr << "flushing Oout" << std::endl;
				#endif
				Oout.flush(); 
				#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
				std::cerr << "flushing O2out" << std::endl;
				#endif
				O2out.flush();
				#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
				std::cerr << "flushing Sout" << std::endl;
				#endif
				Sout.flush();
				#if defined(BAMTOFASTQOUTPUTFILESETDEBUG)
				std::cerr << "all flushed" << std::endl;
				#endif

				for ( split_gz_files_map_type::iterator ita = splitgzfiles.begin(); ita != splitgzfiles.end(); ++ita )
					ita->second.reset();					

				for ( split_files_map_type::iterator ita = splitfiles.begin(); ita != splitfiles.end(); ++ita )
					ita->second.reset();

				for ( gz_files_map_type::iterator ita = gzfiles.begin();
					ita != gzfiles.end(); ++ita )
				{
					ita->second->flush();
					ita->second.reset();
				}				
				
				for (
					files_map_type::iterator ita = files.begin();
					ita != files.end(); ++ita 
				)
				{
					ita->second->flush();
					ita->second.reset();
				}
			}
		};
	}
}
#endif
