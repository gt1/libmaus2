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
#if ! defined(LIBMAUS2_AIO_MEMORYFILEADAPTER_HPP)
#define LIBMAUS2_AIO_MEMORYFILEADAPTER_HPP

#include <libmaus2/aio/MemoryFile.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct MemoryFileAdapter
		{
			typedef MemoryFileAdapter this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			MemoryFile::shared_ptr_type memfile;
			uint64_t p;
			
			MemoryFileAdapter() : p(0)
			{
			
			}
			
			MemoryFileAdapter(MemoryFile::shared_ptr_type rmemfile) : memfile(rmemfile), p(0)
			{
			
			}
			
			void truncate()
			{
				p = 0;
				memfile->f = 0;
				memfile->A->resize(0);
			}
			
			ssize_t read(char * buffer, size_t len)
			{
				// std::cerr << "read(" << len << ")";
			
				assert ( p <= memfile->f );
				
				ssize_t tocopy = std::min(static_cast<uint64_t>(memfile->f-p), static_cast<uint64_t>(len));
				
				std::copy ( memfile->A->begin() + p, memfile->A->begin() + p + tocopy, buffer );
				
				p += tocopy;
				
				return tocopy;
			}
			
			ssize_t write(char const * buffer, size_t len)
			{
				// std::cerr << "write(" << len << ")";
				
				while ( p + len > memfile->A->size() )
				{
					size_t oldsize = memfile->A->size();
					size_t newsize = (oldsize*17)/16;
					
					if ( newsize <= oldsize )
						newsize = oldsize + 1024;
					
					try
					{
						memfile->A->resize(newsize);
					}
					catch(...)
					{
						errno = ENOMEM;
						return -1;
					}
				}
				
				std::copy(buffer,buffer+len,memfile->A->begin() + p);
				p += len;
				
				if ( p > memfile->f )
					memfile->f = p;
				
				return len;
			}
			
			off_t lseek(off_t offset, int whence)
			{
				// std::cerr << "seek(" << offset << "," << whence << ")";

				off_t abs = 0;
				
				switch ( whence )
				{
					case SEEK_SET:
						abs = offset;
						break;
					case SEEK_CUR:
						abs = p + offset;
						break;
					case SEEK_END:
						abs = memfile->f + offset;
						break;
					default:
						return static_cast<off_t>(-1);
				}
				
				if ( abs < 0 )
					return static_cast<off_t>(-1);
				if ( static_cast<uint64_t>(abs) > memfile->f )
					return static_cast<off_t>(-1);
				
				p = static_cast<uint64_t>(abs);
				
				return p;
			}
			
			off_t getFileSize()
			{
				return memfile->f;
			}
		};
	}
}
#endif
