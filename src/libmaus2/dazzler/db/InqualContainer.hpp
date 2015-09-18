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
#if ! defined(LIBMAUS2_DAZZLER_DB_INQUALCONTAINER_HPP)
#define LIBMAUS2_DAZZLER_DB_INQUALCONTAINER_HPP

#include <libmaus2/dazzler/db/PartTrackContainer.hpp>
		
namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct InqualContainer : public PartTrackContainer
			{
				InqualContainer(libmaus2::dazzler::db::DatabaseFile const & rDB, int64_t const blockid = -1)
				: PartTrackContainer(rDB,"inqual",blockid)
				{
				}
				
				bool haveQualityForBlock(uint64_t const blockid) const
				{
					return haveBlock(blockid);
				}
				
				libmaus2::dazzler::db::Track const & getQualityForBlock(uint64_t const blockid) const
				{
					if ( haveQualityForBlock(blockid) )
						return *(Atracks[blockid-1]);
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "InqualContainer::getQualityForBlock: inqual track not present for block " << blockid << std::endl;
						lme.finish();
						throw lme;		
					}
				}
				
				bool haveQualityForRead(uint64_t const readid) const
				{
					uint64_t const blockid = DB.getBlockForIdTrimmed(readid);
					assert ( blockid >= 1 );
					assert ( blockid <= DB.numblocks );
					return haveQualityForBlock(blockid);
				}
				
				std::pair<unsigned char const *, unsigned char const *> getQualityForRead(uint64_t const id) const
				{
					if ( haveQualityForRead(id) )
					{
						uint64_t const blockid = DB.getBlockForIdTrimmed(id);
						libmaus2::dazzler::db::Track const & inqual = getQualityForBlock(blockid);
						std::pair<uint64_t,uint64_t> const blockintv = DB.getTrimmedBlockInterval(blockid);	
						
						libmaus2::dazzler::db::TrackAnnoInterface const & anno = inqual.getAnno();
						uint64_t const anno_low  = anno[id  -blockintv.first];
						uint64_t const anno_high = anno[id+1-blockintv.first];

						assert ( inqual.Adata );
						// assert ( static_cast<int64_t>(anno_high-anno_low) == (DB.getRead(id).rlen + tspace - 1) / tspace );
						assert ( anno_high <= inqual.Adata->size() );

						unsigned char const * Du = inqual.Adata->begin() + anno_low;
						uint64_t const Dn = anno_high - anno_low;
					
						return std::pair<unsigned char const *, unsigned char const *>(Du,Du+Dn);
					
					}
					else
					{
						uint64_t const blockid = DB.getBlockForIdTrimmed(id);
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "InqualContainer::getQuality: cannot find quality for read " << id << " (inqual track not present for block " << blockid << ")" << std::endl;
						lme.finish();
						throw lme;
					}
				}
				
				unsigned char getQualityForBase(uint64_t const id, uint64_t const base, int64_t const tspace) const
				{
					std::pair<unsigned char const *, unsigned char const *> const P = getQualityForRead(id);
					return P.first[base/tspace];
				}
				
				double getErrorForBase(uint64_t const id, uint64_t const base, int64_t const tspace) const
				{
					return phredToError(getQualityForBase(id,base,tspace));
				}

				static double phredToError(unsigned int const v)
				{
					return std::pow(10,static_cast<double>(v)/(-10.0));
				}

				std::vector<double> getErrorVector(uint64_t const id) const
				{
					std::pair<unsigned char const *, unsigned char const *> const P = getQualityForRead(id);
					std::vector<double> D(P.second-P.first);
					for ( ::std::ptrdiff_t i = 0; i < P.second-P.first; ++i )
						D [ i ] = phredToError(P.first[i]);
					return D;
				}
				
				std::vector< std::pair<uint64_t,uint64_t> > getGoodIntervals(uint64_t const id) const
				{
					std::vector<double> const D = getErrorVector(id);
					std::vector< std::pair<uint64_t,uint64_t> > IV;
					
					uint64_t low = 0;
					while ( low < D.size() )
					{
						while ( low < D.size() && D[low] == 1 )
							++low;
						uint64_t high = low;
						while ( high < D.size() && D[high] != 1 )
							++high;
							
						if ( high-low )
						{
							assert ( D[low] < 1 );
							IV.push_back(std::pair<uint64_t,uint64_t>(low,high));
						}
							
						low = high;
					}
					
					return IV;
				}				
			};
		}
	}
}
#endif
