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
#if !defined(LIBMAUS_UTIL_MEMTEMPFILECONTAINER_HPP)
#define LIBMAUS_UTIL_MEMTEMPFILECONTAINER_HPP

#include <libmaus/util/TempFileContainer.hpp>
#include <libmaus/util/shared_ptr.hpp>
#include <libmaus/parallel/OMPLock.hpp>
#include <sstream>
#include <map>

namespace libmaus
{
	namespace util
	{
		struct MemTempFileContainer : public libmaus::util::TempFileContainer
		{
			typedef std::ostringstream output_stream_type;
			typedef libmaus::util::shared_ptr<output_stream_type>::type output_stream_ptr_type;
			typedef std::istringstream input_stream_type;
			typedef libmaus::util::shared_ptr<input_stream_type>::type input_stream_ptr_type;
			
			std::map < uint64_t, output_stream_ptr_type > outstreams;
			std::map < uint64_t, std::string > data;
			std::map < uint64_t, input_stream_ptr_type > instreams;
			libmaus::parallel::OMPLock lock;
			
			std::ostream & openOutputTempFile(uint64_t id)
			{
				libmaus::parallel::ScopeLock slock(lock);
				outstreams[id] = output_stream_ptr_type(new output_stream_type);
				return *(outstreams[id]);
			}
			std::ostream & getOutputTempFile(uint64_t id)
			{
				libmaus::parallel::ScopeLock slock(lock);
				return *(outstreams[id]);
			}
			void closeOutputTempFile(uint64_t id)
			{
				libmaus::parallel::ScopeLock slock(lock);
				if ( outstreams.find(id) != outstreams.end() )
				{
					data[id] = outstreams.find(id)->second->str();
					outstreams.erase(outstreams.find(id));
				}
			}

			std::istream & openInputTempFile(uint64_t id)
			{
				libmaus::parallel::ScopeLock slock(lock);
				if ( data.find(id) != data.end() )
					instreams[id] = input_stream_ptr_type(new input_stream_type(data.find(id)->second));
				else
					instreams[id] = input_stream_ptr_type(new input_stream_type());
					
				return *(instreams[id]);
			}
			void closeInputTempFile(uint64_t id)
			{
				libmaus::parallel::ScopeLock slock(lock);
				if ( instreams.find(id) != instreams.end() )
					instreams.erase(instreams.find(id));
			}
		};
	}
}
#endif
