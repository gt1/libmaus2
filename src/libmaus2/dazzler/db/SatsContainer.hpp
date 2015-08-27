/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_DB_SATSCONTAINER_HPP)
#define LIBMAUS2_DAZZLER_DB_SATSCONTAINER_HPP

#include <libmaus2/dazzler/db/PartTrackContainer.hpp>
		
namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct SatsContainer : public PartTrackContainer
			{
				SatsContainer(libmaus2::dazzler::db::DatabaseFile const & rDB)
				: PartTrackContainer(rDB,"sats")
				{
				}
				
				bool haveSatsForBlock(uint64_t const blockid) const
				{
					return haveBlock(blockid);
				}
				
				libmaus2::dazzler::db::Track const & getSatsForBlock(uint64_t const blockid) const
				{
					if ( haveSatsForBlock(blockid) )
						return *(Atracks[blockid-1]);
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "SatsContainer::getSatsForBlock: sats track not present for block " << blockid << std::endl;
						lme.finish();
						throw lme;		
					}
				}
				
				bool haveSatsForRead(uint64_t const readid) const
				{
					uint64_t const blockid = DB.getBlockForIdTrimmed(readid);
					assert ( blockid >= 1 );
					assert ( blockid <= DB.numblocks );
					return haveSatsForBlock(blockid);
				}
				
				std::vector < std::pair<int32_t,int32_t> > getSatsForRead(uint64_t const id) const
				{
					
					if ( haveSatsForRead(id) )
					{
						uint64_t const blockid = DB.getBlockForIdTrimmed(id);
						libmaus2::dazzler::db::Track const & sats = getSatsForBlock(blockid);
						std::pair<uint64_t,uint64_t> const blockintv = DB.getTrimmedBlockInterval(blockid);	
						
						libmaus2::dazzler::db::TrackAnnoInterface const & anno = sats.getAnno();
						uint64_t const anno_low  = anno[id  -blockintv.first];
						uint64_t const anno_high = anno[id+1-blockintv.first];
					
						assert ( sats.Adata );
						assert ( anno_high <= sats.Adata->size() );

						char const * Du = reinterpret_cast<char const *>(sats.Adata->begin() + anno_low);
						std::string const s(Du,Du+(anno_high-anno_low));
						std::istringstream istr(s);
						
						std::vector < std::pair<int32_t,int32_t> > Vsats;
						uint64_t offset = 0;
						while ( istr.peek() != std::istream::traits_type::eof() )
						{
							int32_t const ilow = libmaus2::dazzler::db::InputBase::getLittleEndianInteger4(istr,offset);
							int32_t const ihigh = libmaus2::dazzler::db::InputBase::getLittleEndianInteger4(istr,offset);
							Vsats.push_back(std::pair<int32_t,int32_t>(ilow,ihigh));
						}
						
						return Vsats;
					}
					else
					{
						uint64_t const blockid = DB.getBlockForIdTrimmed(id);
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "SatsContainer::getSatsForRead: cannot find sats for read " << id << " (sats track not present for block " << blockid << ")" << std::endl;
						lme.finish();
						throw lme;
					}
				}				
			};
		}
	}
}
#endif
