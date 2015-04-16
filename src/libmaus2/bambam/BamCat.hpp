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
			typedef libmaus::bambam::BamAlignmentDecoderWrapper wrapper_type;
			typedef wrapper_type::unique_ptr_type wrapper_pointer_type;
			typedef libmaus::autoarray::AutoArray<wrapper_pointer_type> wrapper_pointer_array_type;
			typedef wrapper_pointer_array_type::unique_ptr_type wrapper_pointer_array_pointer_type;

			bool const streaming;
			std::vector<libmaus::bambam::BamAlignmentDecoderInfo> infos;
			
			wrapper_pointer_array_pointer_type Pwrappers;			
			
			libmaus::bambam::BamCatHeader::unique_ptr_type const Pheader;
			libmaus::bambam::BamCatHeader const & header;
			
			std::vector<std::string>::const_iterator filenameit;
			int64_t fileid;
			wrapper_pointer_type wrapper;
			libmaus::bambam::BamAlignmentDecoder * decoder;
			libmaus::bambam::BamAlignment * algnptr;
			
			static wrapper_pointer_array_pointer_type constructWrappers(std::vector<libmaus::bambam::BamAlignmentDecoderInfo> const & infos, bool const streaming)
			{
				wrapper_pointer_array_pointer_type Pwrappers;
				
				if ( streaming )
				{
					wrapper_pointer_array_pointer_type Twrappers(new wrapper_pointer_array_type(infos.size()));
					Pwrappers = UNIQUE_PTR_MOVE(Twrappers);
					
					for ( uint64_t i = 0; i < infos.size(); ++i )
					{
						wrapper_pointer_type tptr ( libmaus::bambam::BamAlignmentDecoderFactory::construct(infos[i]) );
						(*Pwrappers)[i] = UNIQUE_PTR_MOVE(tptr);
					}
				}
				
				return UNIQUE_PTR_MOVE(Pwrappers);
			}
			
			static libmaus::bambam::BamCatHeader::unique_ptr_type constructHeader(
				std::vector<libmaus::bambam::BamAlignmentDecoderInfo> const & infos, 
				wrapper_pointer_array_pointer_type & Pwrappers
			)
			{
				if ( Pwrappers.get() )
				{
					libmaus::autoarray::AutoArray<libmaus::bambam::BamAlignmentDecoder *> decs(Pwrappers->size());
					for ( uint64_t i = 0; i < Pwrappers->size(); ++i )
						decs[i] = &((*Pwrappers)[i]->getDecoder());

					libmaus::bambam::BamCatHeader::unique_ptr_type tptr(new libmaus::bambam::BamCatHeader(decs));
					return UNIQUE_PTR_MOVE(tptr);
				}
				else
				{
					libmaus::bambam::BamCatHeader::unique_ptr_type tptr(new libmaus::bambam::BamCatHeader(infos));
					return UNIQUE_PTR_MOVE(tptr);
				}
			}

			static libmaus::bambam::BamCatHeader::unique_ptr_type constructHeader(libmaus::util::ArgInfo const & arginfo, std::vector<std::string> const & filenames)
			{
				libmaus::bambam::BamCatHeader::unique_ptr_type tptr(new libmaus::bambam::BamCatHeader(arginfo,filenames));
				return UNIQUE_PTR_MOVE(tptr);				
			}

			static libmaus::bambam::BamCatHeader::unique_ptr_type constructHeader(std::vector<std::string> const & filenames)
			{
				libmaus::bambam::BamCatHeader::unique_ptr_type tptr(new libmaus::bambam::BamCatHeader(filenames));
				return UNIQUE_PTR_MOVE(tptr);				
			}
			
			BamCat(
				libmaus::util::ArgInfo const & arginfo,
				std::vector<std::string> const & rfilenames, 
				bool const putrank = false,
				bool const rstreaming = false
			) 
			: BamAlignmentDecoder(putrank), 
			  streaming(rstreaming),
			  infos(libmaus::bambam::BamAlignmentDecoderInfo::filenameToInfo(arginfo,rfilenames)), 
			  Pwrappers(constructWrappers(infos,streaming)),
			  Pheader(streaming ? constructHeader(infos,Pwrappers) : constructHeader(arginfo,rfilenames)),
			  header(*Pheader),
			  fileid(-1), 
			  wrapper(), 
			  decoder(0)
			{
			}

			BamCat(
				std::vector<std::string> const & rfilenames, 
				bool const putrank = false,
				bool const rstreaming = false
			) 
			: BamAlignmentDecoder(putrank), 
			  streaming(rstreaming),
			  infos(libmaus::bambam::BamAlignmentDecoderInfo::filenameToInfo(rfilenames)), 
			  Pwrappers(constructWrappers(infos,streaming)),
			  Pheader(streaming ? constructHeader(infos,Pwrappers) : constructHeader(rfilenames)),
			  header(*Pheader),
			  fileid(-1), 
			  wrapper(), 
			  decoder(0)
			{
			}

			BamCat(
				std::vector<libmaus::bambam::BamAlignmentDecoderInfo> const & rinfos, 
				bool const putrank = false,
				bool const rstreaming = false
			) 
			: BamAlignmentDecoder(putrank), 
			  streaming(rstreaming),
			  infos(rinfos), 
			  Pwrappers(constructWrappers(infos,streaming)),
			  Pheader(constructHeader(infos,Pwrappers)),
			  header(*Pheader),
			  fileid(-1), 
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
							if ( Pwrappers.get() )
							{
								decoder = &((*Pwrappers)[fileid]->getDecoder());
							}
							else
							{
								libmaus::bambam::BamAlignmentDecoderWrapper::unique_ptr_type tptr ( libmaus::bambam::BamAlignmentDecoderFactory::construct(infos[fileid]) );
								wrapper = UNIQUE_PTR_MOVE(tptr);
								decoder = &(wrapper->getDecoder());
							}
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
