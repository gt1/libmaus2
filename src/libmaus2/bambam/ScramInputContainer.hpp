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
#include <libmaus/lz/BufferedGzipStream.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct ScramInputContainer
		{
			static std::map<void *, libmaus::util::shared_ptr<scram_cram_io_input_t>::type > Mcontrol;	
			static std::map<void *, libmaus::aio::InputStream::shared_ptr_type> Mstream;
			static std::map<void *, libmaus::aio::InputStream::shared_ptr_type> Mcompstream;
			static libmaus::parallel::PosixMutex Mlock;

			static scram_cram_io_input_t * allocate(char const * filename, int const decompress)
			{
				libmaus::parallel::ScopePosixMutex Llock(Mlock);
				libmaus::util::shared_ptr<scram_cram_io_input_t>::type sptr;
				libmaus::aio::InputStream::shared_ptr_type sstr;
				libmaus::aio::InputStream::shared_ptr_type gstr;
				
				/* allocate data structure */
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
			
				/* store pointer to data structure in Mcontrol */
				try
				{
					Mcontrol[sptr.get()] = libmaus::util::shared_ptr<scram_cram_io_input_t>::type();
				}
				catch(std::exception const & ex)
				{
					std::cerr << ex.what() << std::endl;
					return NULL;
				}
				
				{
					std::map<void *, libmaus::util::shared_ptr<scram_cram_io_input_t>::type >::iterator it = Mcontrol.find(sptr.get());					
					assert (it != Mcontrol.end());
					it->second = sptr;
				}
				
				/* open file via factory */
				try
				{
					libmaus::aio::InputStream::shared_ptr_type tstr(libmaus::aio::InputStreamFactoryContainer::constructShared(filename));
					sstr = tstr;
				}
				catch(std::exception const & ex)
				{
					std::cerr << "Failed to open " << filename << ":\n" << ex.what() << std::endl;
					
					std::map<void *, libmaus::util::shared_ptr<scram_cram_io_input_t>::type >::iterator it = Mcontrol.find(sptr.get());
					assert ( it != Mcontrol.end() );
					Mcontrol.erase(it);
					
					return NULL;
				}
				
				try
				{
					Mstream[sptr.get()] = libmaus::aio::InputStream::shared_ptr_type();
				}
				catch(...)
				{
					std::map<void *, libmaus::util::shared_ptr<scram_cram_io_input_t>::type >::iterator it = Mcontrol.find(sptr.get());
					assert ( it != Mcontrol.end() );
					Mcontrol.erase(it);
					
					return NULL;
				}

				{
					std::map<void *, libmaus::aio::InputStream::shared_ptr_type>::iterator it = Mstream.find(sptr.get());
					assert ( it != Mstream.end() );
					it->second = sstr;
				}
				
				if ( decompress )
				{
					try
					{
						libmaus::util::shared_ptr<std::istream>::type tptr(new libmaus::lz::BufferedGzipStream(*sstr));
						libmaus::aio::InputStream::shared_ptr_type ttptr(new libmaus::aio::InputStream(tptr));
						gstr = ttptr;
					}
					catch(...)
					{
						std::map<void *, libmaus::aio::InputStream::shared_ptr_type>::iterator its = Mstream.find(sptr.get());
						assert ( its != Mstream.end() );
						Mstream.erase(its);
						
						std::map<void *, libmaus::util::shared_ptr<scram_cram_io_input_t>::type >::iterator itc = Mcontrol.find(sptr.get());
						assert ( itc != Mcontrol.end() );
						Mcontrol.erase(itc);

						return NULL;
					}
					
					try
					{
						Mcompstream[sptr.get()] = libmaus::aio::InputStream::shared_ptr_type();
					}
					catch(...)
					{
						std::map<void *, libmaus::aio::InputStream::shared_ptr_type>::iterator its = Mstream.find(sptr.get());
						assert ( its != Mstream.end() );
						Mstream.erase(its);
						
						std::map<void *, libmaus::util::shared_ptr<scram_cram_io_input_t>::type >::iterator itc = Mcontrol.find(sptr.get());
						assert ( itc != Mcontrol.end() );
						Mcontrol.erase(itc);

						return NULL;						
					}

					{
						std::map<void *, libmaus::aio::InputStream::shared_ptr_type>::iterator its = Mcompstream.find(sptr.get());
						assert ( its != Mcompstream.end() );
						its->second = gstr;						
					}
				}
				
				memset(sptr.get(),0,sizeof(scram_cram_io_input_t));
				sptr->user_data = decompress ? gstr.get() : sstr.get();
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
					{
						std::map<void *, libmaus::aio::InputStream::shared_ptr_type>::iterator its = Mcompstream.find(obj);
						if ( its != Mcompstream.end() )
							Mcompstream.erase(its);						
					}
					{
						std::map<void *, libmaus::aio::InputStream::shared_ptr_type>::iterator its = Mstream.find(obj);
						if ( its != Mstream.end() )
							Mstream.erase(its);						
					}
					{
						std::map<void *, libmaus::util::shared_ptr<scram_cram_io_input_t>::type >::iterator itc = Mcontrol.find(obj);
						assert ( itc != Mcontrol.end() );
						Mcontrol.erase(itc);					
					}
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
