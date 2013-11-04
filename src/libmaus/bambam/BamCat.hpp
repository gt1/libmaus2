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
#if ! defined(LIBMAUS_BAMBAM_BAMCAT_HPP)
#define LIBMAUS_BAMBAM_BAMCAT_HPP

#include <libmaus/bambam/BamCatHeader.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * base class for decoding BAM files
		 **/
		struct BamCat : public BamAlignmentDecoder
		{
			std::vector<libmaus::bambam::BamAlignmentDecoderInfo> infos;
			libmaus::bambam::BamCatHeader const header;
			std::vector<std::string>::const_iterator filenameit;
			int64_t fileid;
			libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type wrapper;
			libmaus::bambam::BamAlignmentDecoder * decoder;
			libmaus::bambam::BamAlignment * algnptr;
			
			BamCat(std::vector<std::string> const & rfilenames, bool const putrank = false) 
			: BamAlignmentDecoder(putrank), infos(libmaus::bambam::BamAlignmentDecoderInfo::filenameToInfo(rfilenames)), header(infos), fileid(-1), 
			  wrapper(), 
			  decoder(0)
			{
			}

			BamCat(std::vector<libmaus::bambam::BamAlignmentDecoderInfo> const & rinfos, bool const putrank = false) 
			: BamAlignmentDecoder(putrank), infos(rinfos), header(infos), fileid(-1), 
			  wrapper(), 
			  decoder(0)
			{
			}
		
			/**
			 * input method
			 *
			 * @return bool if alignment input was successfull and a new alignment was stored
			 **/
			virtual bool readAlignmentInternal(bool const delayPutRank = false)
			{
				while ( true )
				{
					if ( expect_false(! decoder) )
					{
						if ( static_cast<uint64_t>(++fileid) == infos.size() )
						{
							return false;
						}
						else
						{
							libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr ( libmaus::bambam::BamAlignmentDecoderFactory::construct(infos[fileid]) );
							wrapper = UNIQUE_PTR_MOVE(tptr);
							decoder = &(wrapper->getDecoder());
							algnptr = &(decoder->getAlignment());
						}
					}
					
					bool const ok = decoder->readAlignment();
					
					if ( expect_true(ok) )
					{
						alignment.swap(*algnptr);

						if ( ! delayPutRank )
							putRank();
							
						header.updateAlignment(fileid,alignment);
						return true;
					}
					else
					{
						wrapper.reset();
						decoder = 0;
						algnptr = 0;
					}
				}
			}
			
			virtual libmaus::bambam::BamHeader const & getHeader() const
			{
				return *(header.bamheader);
			}
		};
	}
}
#endif
