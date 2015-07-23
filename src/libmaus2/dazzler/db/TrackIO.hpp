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
#if ! defined(LIBMAUS2_DAZZLER_DB_TRACKIO_HPP)
#define LIBMAUS2_DAZZLER_DB_TRACKIO_HPP

#include <libmaus2/dazzler/db/InputBase.hpp>
#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		struct TrackAnnoInterface
		{
			typedef TrackAnnoInterface this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			virtual ~TrackAnnoInterface() {}
			virtual uint64_t operator[](uint64_t const i) const = 0;
		};
	
		template<typename _element_type>
		struct TrackAnno : public TrackAnnoInterface
		{
			typedef _element_type element_type;
			typedef TrackAnno<element_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			libmaus2::autoarray::AutoArray<element_type> A;
			
			TrackAnno()
			{
			
			}
			
			TrackAnno(uint64_t const n) : A(n) {}
			
			uint64_t operator[](uint64_t const i) const
			{
				return A[i];
			}
		};
		
		struct TrackIO : public libmaus2::dazzler::db::InputBase
		{
			static void readTrack(libmaus2::dazzler::db::DatabaseFile const & db, std::string const & trackname)
			{
				bool ispart = false;
				std::string annoname;

				// check whether annotation/track is specific to this block
				if ( 
					db.part &&
					libmaus2::aio::InputStreamFactoryContainer::tryOpen(db.path + "/" + db.root + "." + libmaus2::util::NumberSerialisation::formatNumber(db.part,0) + "." + trackname + ".anno" )
				)
				{
					ispart = true;
					annoname = db.path + "/" + db.root + "." + libmaus2::util::NumberSerialisation::formatNumber(db.part,0) + "." + trackname + ".anno";
				}
				// no, try whole database
				else
				{
					annoname = db.path + "/" + db.root + "." + trackname + ".anno";
				}
				
				if ( ! libmaus2::aio::InputStreamFactoryContainer::tryOpen(annoname) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "Unable to open " << annoname << std::endl;
					lme.finish();
					throw lme;
				}
				
				std::string dataname;
				if ( ispart )
					dataname = db.path + "/" + db.root + "." + libmaus2::util::NumberSerialisation::formatNumber(db.part,0) + "." + trackname + ".data";
				else
					dataname = db.path + "/" + db.root + "." + trackname + ".data";
				
				libmaus2::aio::InputStream::unique_ptr_type Panno(libmaus2::aio::InputStreamFactoryContainer::constructUnique(annoname));
				std::istream & anno = *Panno;
			
				uint64_t offset = 0;
				int32_t tracklen = getLittleEndianInteger4(anno,offset);
				int32_t size = getLittleEndianInteger4(anno,offset);
				
				libmaus2::aio::InputStream::unique_ptr_type Pdata;
				if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(dataname) )
				{
					libmaus2::aio::InputStream::unique_ptr_type Tdata(libmaus2::aio::InputStreamFactoryContainer::constructUnique(dataname));
					Pdata = UNIQUE_PTR_MOVE(Tdata);
				}
			
				TrackAnnoInterface::unique_ptr_type PDanno;
				libmaus2::autoarray::AutoArray<unsigned char> Adata;
				
				// number of reads in database loaded
				uint64_t const nreads = db.indexbase.nreads;
				
				if ( Pdata )
				{
					if ( size == 4 )
					{
						if ( db.part && ! ispart )
						{
							// anno.seekg();
						}
					
						TrackAnno<uint32_t>::unique_ptr_type Tanno(new TrackAnno<uint32_t>(nreads+1));

						uint64_t offset = 0;
						for ( uint64_t i = 0; i < nreads+1; ++i )
							Tanno->A[i] = getLittleEndianInteger4(*Panno,offset);
				
						TrackAnnoInterface::unique_ptr_type Tint(Tanno.release());
						PDanno = UNIQUE_PTR_MOVE(Tint);
					}
					else if ( size == 8 )
					{
						TrackAnno<uint64_t>::unique_ptr_type Tanno(new TrackAnno<uint64_t>(nreads+1));

						uint64_t offset = 0;
						for ( uint64_t i = 0; i < nreads+1; ++i )
							Tanno->A[i] = getLittleEndianInteger4(*Panno,offset);
				
						TrackAnnoInterface::unique_ptr_type Tint(Tanno.release());
						PDanno = UNIQUE_PTR_MOVE(Tint);
					}
					else
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "TrackIO::readTrack: unknown size " << size << std::endl;
						lme.finish();
						throw lme;
					}
				}
				// incomplete
			}
		};	
	}
}
#endif
