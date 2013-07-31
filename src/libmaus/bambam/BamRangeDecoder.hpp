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
#if ! defined(LIBMAUS_BAMBAM_BAMRANGEDECODER_HPP)
#define LIBMAUS_BAMBAM_BAMRANGEDECODER_HPP

#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/bambam/BamIndex.hpp>
#include <libmaus/bambam/BamRangeParser.hpp>
#include <libmaus/util/OutputFileNameTools.hpp>

namespace libmaus
{
	namespace bambam
	{
		//! BAM decoder supporting ranges
		struct BamRangeDecoder
		{
			private:
			/**
			 * derive .bai file name from .bam file name. The function
			 * checks if bamname+".bai" or ${bamname%.bam}.bai exists
			 * and returns this name. If none of the two exist then an
			 * exception is thrown.
			 *
			 * @param bamname name of bam file
			 * @return file name of index for bamname
			 **/
			static std::string deriveBamIndexName(std::string const & bamname)
			{
				std::string bainame;
				
				if ( libmaus::util::GetFileSize::fileExists(bamname+".bai") )
				{
					return bamname+".bai";
				}
				else if ( libmaus::util::GetFileSize::fileExists(
					libmaus::util::OutputFileNameTools::clipOff(bamname,".bam")+".bai") 
				)
				{
					return libmaus::util::OutputFileNameTools::clipOff(bamname,".bam")+".bai";
				}
				else
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "Unable to find index for file " << bamname << std::endl;
					se.finish();
					throw se;
				}
			}
			
			/**
			 * load BAM file header of file named filename
			 *
			 * @param filename BAM file name
			 * @return header object from BAM file
			 **/
			static libmaus::bambam::BamHeader::unique_ptr_type loadHeader(std::string const & filename)
			{
				libmaus::aio::CheckedInputStream CIS(filename);
				libmaus::lz::BgzfInflateStream BIS(CIS);
				libmaus::bambam::BamHeader::unique_ptr_type Pheader(new libmaus::bambam::BamHeader(BIS));
				return UNIQUE_PTR_MOVE(Pheader);
			}

			/**
			 * load BAM file index from filename
			 *
			 * @param filename name of .bai index file
			 * @return index
			 **/
			static libmaus::bambam::BamIndex::unique_ptr_type loadIndex(std::string const & filename)
			{
				libmaus::aio::CheckedInputStream CIS(filename);
				libmaus::bambam::BamIndex::unique_ptr_type Pindex(new libmaus::bambam::BamIndex(CIS));
				return UNIQUE_PTR_MOVE(Pindex);
			}
			
			//! name of bam file
			std::string const filename;
			//! name of bai file
			std::string const indexname;
			
			//! pointer to bam header
			libmaus::bambam::BamHeader::unique_ptr_type const Pheader;
			//! bam header reference
			libmaus::bambam::BamHeader const & header;
			
			//! pointer to bam index (bai)
			libmaus::bambam::BamIndex::unique_ptr_type const Pindex;
			//! bam index reference
			libmaus::bambam::BamIndex const & index;
			
			//! range array
			libmaus::autoarray::AutoArray<libmaus::bambam::BamRange::unique_ptr_type> const ranges;
			//! next element to be processed in ranges
			uint64_t rangeidx;
			//! pointer to range which is currently processed
			libmaus::bambam::BamRange const * rangecur;

			//! bam decoder wrapper			
			libmaus::bambam::BamDecoderWrapper::unique_ptr_type wrapper;
			//! chunks for current range
			std::vector< std::pair<uint64_t,uint64_t> > chunks;
			//! next element to be processed in chunks
			uint64_t chunkidx;
			
			//! decoder for current chunk
			libmaus::bambam::BamAlignmentDecoder * decoder;
			//! alignment object in decoder
			libmaus::bambam::BamAlignment * algn;
			
			public:
			/**
			 * constructor
			 *
			 * @param rfilename name of BAM file
			 * @param rranges range descriptor string
			 **/
			BamRangeDecoder(std::string const & rfilename, std::string const & rranges)
			: filename(rfilename), indexname(deriveBamIndexName(filename)),
			  Pheader(UNIQUE_PTR_MOVE(loadHeader(filename))),
			  header(*Pheader),
			  Pindex(UNIQUE_PTR_MOVE(loadIndex(indexname))),
			  index(*Pindex),
			  ranges(libmaus::bambam::BamRangeParser::parse(rranges,header)),
			  rangeidx(0),
			  rangecur(0),
			  wrapper(),
			  chunks(),
			  chunkidx(0),
			  decoder(0),
			  algn(0)
			{
			
			}
			
			/**
			 * read next alignment. if the function yields true, then
			 * the alignment can be retrieved getAlignment. Any call
			 * to readAlignment invalidates a previous return values
			 * of the getAlignment method.
			 *
			 * @return true iff an alignment could be obtained
			 **/
			bool readAlignment()
			{
				while ( true )
				{
					if ( ! wrapper )
					{
						// next chunk
						if ( chunkidx < chunks.size() )
						{
							wrapper = UNIQUE_PTR_MOVE(
								libmaus::bambam::BamDecoderWrapper::unique_ptr_type(
									new
										libmaus::bambam::BamDecoderWrapper(
											filename,
											header,
											chunks[chunkidx].first,
											chunks[chunkidx].second
										)
								)
							);
							
							decoder = &(wrapper->getDecoder());
							algn = &(decoder->getAlignment());
							
							chunkidx++;
						}
						// next range
						else if ( rangeidx < ranges.size() )
						{
							rangecur = ranges[rangeidx++].get();
							chunks = rangecur->getChunks(index);
							chunkidx = 0;							
						}
						// no more ranges
						else
						{
							decoder = 0;
							algn = 0;
							return false;
						}
					}
					else
					{
						if ( decoder->readAlignment() )
						{
							if ( (*rangecur)(*algn) == libmaus::bambam::BamRange::interval_rel_pos_matching )
								return true;
						}
						else
						{
							wrapper.reset();
						}
					}
				}
			}
			
			/**
			 * get next alignment. Calling this function is only valid
			 * if the most recent call to readAlignment returned true
			 *
			 * @return alignment
			 **/
			libmaus::bambam::BamAlignment & getAlignment()
			{
				return *algn;
			}

			/**
			 * get next alignment. Calling this function is only valid
			 * if the most recent call to readAlignment returned true
			 *
			 * @return alignment
			 **/
			libmaus::bambam::BamAlignment const & getAlignment() const
			{
				return *algn;
			}
			
			/**
			 * @return BAM file header object
			 **/
			libmaus::bambam::BamHeader const & getHeader() const
			{
				return header;
			}
		};
	}
}
#endif
