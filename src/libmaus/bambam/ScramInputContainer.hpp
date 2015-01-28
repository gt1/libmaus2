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
#if ! defined(LIBMAUS_BAMBAM_SCRAMINPUTCONTAINER_HPP)
#define LIBMAUS_BAMBAM_SCRAMINPUTCONTAINER_HPP

#include <libmaus/aio/InputStream.hpp>
#include <libmaus/aio/InputStreamFactoryContainer.hpp>
#include <libmaus/bambam/Scram.h>

namespace libmaus
{
	namespace bambam
	{
		struct ScramInputContainer
		{
			static std::map<void *, libmaus::util::shared_ptr<scram_cram_io_input_t>::type > Mcontrol;	
			static std::map<void *, libmaus::aio::InputStream::shared_ptr_type> Mstream;
			static libmaus::parallel::PosixMutex Mlock;

			static scram_cram_io_input_t * allocate(char const * filename)
			{
				libmaus::parallel::ScopePosixMutex Llock(Mlock);
				libmaus::util::shared_ptr<scram_cram_io_input_t>::type sptr;
				libmaus::aio::InputStream::shared_ptr_type sstr;
				
				try
				{
					libmaus::util::shared_ptr<scram_cram_io_input_t>::type tptr(new scram_cram_io_input_t);
					sptr = tptr;
				}
				catch(std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
					return NULL;
				}
			
				try
				{
					Mcontrol[sptr.get()] = libmaus::util::shared_ptr<scram_cram_io_input_t>::type();
				}
				catch(...)
				{
					return NULL;
				}
				
				Mcontrol[sptr.get()] = sptr;
				
				try
				{
					libmaus::aio::InputStream::unique_ptr_type tstr(libmaus::aio::InputStreamFactoryContainer::construct(filename));
					libmaus::aio::InputStream::shared_ptr_type tsstr(tstr.release());
					sstr = tsstr;
				}
				catch(std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
					Mcontrol.erase(Mcontrol.find(sptr.get()));
					return NULL;
				}
				
				try
				{
					Mstream[sptr.get()] = libmaus::aio::InputStream::shared_ptr_type();
				}
				catch(...)
				{
					Mcontrol.erase(Mcontrol.find(sptr.get()));
					return NULL;
				}
				
				Mstream[sptr.get()] = sstr;
				
				memset(sptr.get(),0,sizeof(scram_cram_io_input_t));
				sptr->user_data = sstr.get();
				sptr->fread_callback = call_fread;
				sptr->fseek_callback = call_fseek;
				sptr->ftell_callback = call_ftell;
				
				return sptr.get();
			}

			static scram_cram_io_input_t * deallocate(scram_cram_io_input_t * obj)
			{
				libmaus::parallel::ScopePosixMutex Llock(Mlock);
				if ( obj )
				{
					if ( Mstream.find(obj) != Mstream.end() )
						Mstream.erase(Mstream.find(obj));
					if ( Mcontrol.find(obj) != Mcontrol.end() )
						Mcontrol.erase(Mcontrol.find(obj));
				}
				return NULL;
			}

			static size_t call_fread(void * ptr, size_t size, size_t nmemb, void *stream)
			{
				std::istream * pistr = (std::istream *)stream;
				char * out = reinterpret_cast<char *>(ptr);
				uint64_t bytestoread = size*nmemb;
				uint64_t r = 0, rr = 0;
				
				do
				{
					pistr->read(out,bytestoread);
					rr = pistr->gcount();
					r += rr;
					bytestoread -= rr;
					out += rr;
				} while ( bytestoread && rr );
				
				return (r / size);
			}

			static int call_fseek(void * fd, off_t offset, int whence)
			{
				std::istream * pistr = reinterpret_cast<std::istream *>(fd);
				pistr->clear();
			
				switch ( whence )
				{
					case SEEK_SET:
						pistr->seekg(offset,std::ios::beg);
						break;
					case SEEK_CUR:
						pistr->seekg(offset,std::ios::cur);
						break;
					case SEEK_END:
						pistr->seekg(offset,std::ios::end);
						break;
					default:
						return -1;
				}
				
				if ( pistr->fail() )
				{
					pistr->clear();
					return -1;
				}
				else
				{
					return 0;
				}
			}

			static off_t call_ftell(void * fd)
			{
				std::istream * pistr = reinterpret_cast<std::istream *>(fd);
				return pistr->tellg();
			}	
		};
	}
}
#endif
