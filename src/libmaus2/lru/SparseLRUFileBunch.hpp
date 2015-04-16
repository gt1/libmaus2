/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_LRU_SPARSELRUFILEBUNCH_HPP)
#define LIBMAUS_LRU_SPARSELRUFILEBUNCH_HPP

#include <libmaus/aio/CheckedInputOutputStream.hpp>
#include <libmaus/lru/SparseLRU.hpp>
#include <libmaus/util/GetFileSize.hpp>

namespace libmaus
{
	namespace lru
	{
		struct SparseLRUFileBunch
		{
			std::string const tmpfilenamebase;
			SparseLRU SLRU;
			std::map<uint64_t, libmaus::aio::CheckedInputOutputStream::shared_ptr_type> COSmap;
			
			std::string getFileName(uint64_t const id) const
			{
				std::ostringstream ostr;
				ostr << tmpfilenamebase << "_" << id;
				return ostr.str();
			}
						
			SparseLRUFileBunch(std::string const & rtmpfilenamebase, uint64_t const rmaxsimult)
			: tmpfilenamebase(rtmpfilenamebase), SLRU(rmaxsimult)
			{
			
			}
			
			void remove(uint64_t const fileid)
			{
				std::map<uint64_t, libmaus::aio::CheckedInputOutputStream::shared_ptr_type>::iterator it = COSmap.find(fileid);
				
				if ( it != COSmap.end() )
				{
					SLRU.erase(fileid);
					COSmap.erase(it);
				}
				
				std::string const fn = getFileName(fileid);
				
				::remove ( fn.c_str() );
			}
			
			libmaus::aio::CheckedInputOutputStream & operator[](uint64_t const fileid)
			{
				std::map<uint64_t, libmaus::aio::CheckedInputOutputStream::shared_ptr_type>::iterator it = COSmap.find(fileid);
				
				if ( it != COSmap.end() )
					return *(it->second);
					
				int64_t const kickid = SLRU.get(fileid);
				
				// close file
				if ( kickid >= 0 )
				{
					std::map<uint64_t, libmaus::aio::CheckedInputOutputStream::shared_ptr_type>::iterator ito = COSmap.find(kickid);
					assert ( ito != COSmap.end() );
					
					ito->second->flush();
					ito->second.reset();
					COSmap.erase(ito);
				}
				
				std::string const fn = getFileName(fileid);
				
				if ( libmaus::util::GetFileSize::fileExists(fn) )
				{
					libmaus::aio::CheckedInputOutputStream::shared_ptr_type ptr(new libmaus::aio::CheckedInputOutputStream(fn,
						std::ios_base::in | std::ios_base::out | std::ios_base::binary
					));
					
					COSmap[fileid] = ptr;
					
					ptr->seekg(0,std::ios::end);
				
					return *ptr;
				}
				else
				{
					libmaus::aio::CheckedInputOutputStream::shared_ptr_type ptr(new libmaus::aio::CheckedInputOutputStream(fn,
						std::ios_base::in | std::ios_base::out | std::ios_base::binary | std::ios_base::trunc
					));
				
					COSmap[fileid] = ptr;
				
					return *ptr;				
				}
			}
		};
	}
}
#endif
