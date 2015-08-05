/*
    libmaus2
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
#if ! defined(LIBMAUS2_LRU_SPARSELRUFILEBUNCH_HPP)
#define LIBMAUS2_LRU_SPARSELRUFILEBUNCH_HPP

#include <libmaus2/aio/InputOutputStreamFactoryContainer.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/lru/SparseLRU.hpp>
#include <libmaus2/util/GetFileSize.hpp>

namespace libmaus2
{
	namespace lru
	{
		struct SparseLRUFileBunch
		{
			std::string const tmpfilenamebase;
			SparseLRU SLRU;
			std::map<uint64_t, libmaus2::aio::InputOutputStream::shared_ptr_type> COSmap;
			
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
				std::map<uint64_t, libmaus2::aio::InputOutputStream::shared_ptr_type>::iterator it = COSmap.find(fileid);
				
				if ( it != COSmap.end() )
				{
					SLRU.erase(fileid);
					COSmap.erase(it);
				}
				
				std::string const fn = getFileName(fileid);
				
				::remove ( fn.c_str() );
			}
			
			libmaus2::aio::InputOutputStream & operator[](uint64_t const fileid)
			{
				std::map<uint64_t, libmaus2::aio::InputOutputStream::shared_ptr_type>::iterator it = COSmap.find(fileid);
				
				if ( it != COSmap.end() )
					return *(it->second);
					
				int64_t const kickid = SLRU.get(fileid);
				
				// close file
				if ( kickid >= 0 )
				{
					std::map<uint64_t, libmaus2::aio::InputOutputStream::shared_ptr_type>::iterator ito = COSmap.find(kickid);
					assert ( ito != COSmap.end() );
					
					ito->second->flush();
					ito->second.reset();
					COSmap.erase(ito);
				}
				
				libmaus2::aio::InputOutputStream::shared_ptr_type ptr(libmaus2::aio::InputOutputStreamFactoryContainer::constructShared(getFileName(fileid),
					std::ios_base::in | std::ios_base::out | std::ios_base::binary
				));

				COSmap[fileid] = ptr;

				ptr->seekg(0,std::ios::end);

				return *ptr;
			}
		};
	}
}
#endif
