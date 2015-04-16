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
#if ! defined(LIBMAUS2_UTIL_FILETEMPFILECONTAINER_HPP)
#define LIBMAUS2_UTIL_FILETEMPFILECONTAINER_HPP

#include <libmaus2/util/TempFileContainer.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <libmaus2/aio/CheckedInputStream.hpp>
#include <libmaus2/aio/CheckedOutputStream.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <map>

namespace libmaus2
{
	namespace util
	{
		/**
		 * file based temporary storage container class
		 **/
		struct FileTempFileContainer : public libmaus2::util::TempFileContainer
		{
			//! type of output stream of temporary storage
			typedef libmaus2::aio::CheckedOutputStream output_stream_type;
			//! pointer to output stream
			typedef libmaus2::util::shared_ptr<output_stream_type>::type output_stream_ptr_type;
			//! type of input stream for temporary storage
			typedef libmaus2::aio::CheckedInputStream input_stream_type;
			//! pointer to input stream
			typedef libmaus2::util::shared_ptr<input_stream_type>::type input_stream_ptr_type;
			
			//! temporary file name generator object
			libmaus2::util::TempFileNameGenerator & tmpgen;
			//! output streams
			std::map < uint64_t, output_stream_ptr_type > outstreams;
			//! output file names
			std::map < uint64_t, std::string > filenames;
			//! input streams
			std::map < uint64_t, input_stream_ptr_type > instreams;
			//! concurrency lock
			libmaus2::parallel::OMPLock lock;
			
			/**
			 * constructor
			 *
			 * @param rtmpgen generator object for temporary file names
			 **/
			FileTempFileContainer(
				libmaus2::util::TempFileNameGenerator & rtmpgen
			) : tmpgen(rtmpgen)
			{
				libmaus2::util::TempFileRemovalContainer::setup();
			}
			
			/**
			 * open temp file for id and return reference;
			 * destroys previous files and existing references to id
			 *
			 * @param id temp file id
			 * @return reference to output stream for id
			 **/
			std::ostream & openOutputTempFile(uint64_t id)
			{
				libmaus2::parallel::ScopeLock slock(lock);
				filenames[id] = tmpgen.getFileName();
				libmaus2::util::TempFileRemovalContainer::addTempFile(filenames[id]);
				outstreams[id] = output_stream_ptr_type(new output_stream_type(filenames[id]));
				return *(outstreams[id]);
			}
			/**
			 * get tempfile for id; openOutputTempFile(id) must have been called before
			 *
			 * @param id temp file id
			 * @return reference to output stream for id
			 **/
			std::ostream & getOutputTempFile(uint64_t id)
			{
				libmaus2::parallel::ScopeLock slock(lock);
				return *(outstreams[id]);
			}
			/**
			 * close output file for id; this makes id unavailable for getOutputTempFile
			 *
			 * @param id file to be closedd
			 **/
			void closeOutputTempFile(uint64_t id)
			{
				libmaus2::parallel::ScopeLock slock(lock);
				if ( outstreams.find(id) != outstreams.end() )
				{
					outstreams.find(id)->second->flush();
					outstreams.find(id)->second->close();
					outstreams.erase(outstreams.find(id));
				}
			}
			/**
			 * open file for id
			 *
			 * @param id temp file id
			 * @return input stream for reading the data in temp file for id
			 **/
			std::istream & openInputTempFile(uint64_t id)
			{
				libmaus2::parallel::ScopeLock slock(lock);
				assert ( filenames.find(id) != filenames.end() );
				
				instreams[id] = input_stream_ptr_type(new input_stream_type(filenames.find(id)->second));
					
				return *(instreams[id]);
			}
			/**
			 * close temporary file for id and delete it
			 *
			 * @param id file to be closed and deleted
			 **/
			void closeInputTempFile(uint64_t id)
			{
				libmaus2::parallel::ScopeLock slock(lock);
				if ( instreams.find(id) != instreams.end() )
				{
					instreams.erase(instreams.find(id));
					remove ( filenames.find(id)->second.c_str() );
				}
			}
		};
	}
}
#endif
