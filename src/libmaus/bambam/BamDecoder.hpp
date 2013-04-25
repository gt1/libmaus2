/**
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
**/
#if ! defined(LIBMAUS_BAMBAM_BAMDECODER_HPP)
#define LIBMAUS_BAMBAM_BAMDECODER_HPP

#include <libmaus/bambam/BamAlignmentDecoder.hpp>
#include <libmaus/lz/BgzfInflateStream.hpp>

namespace libmaus
{
	namespace bambam
	{				
		struct BamDecoder : public libmaus::bambam::BamAlignmentDecoder
		{
			typedef BamDecoder this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			// private:
			::libmaus::util::unique_ptr< std::ifstream >::type PISTR;
			libmaus::lz::BgzfInflateStream GZ;

			public:
			::libmaus::bambam::BamHeader bamheader;
						
			public:
			BamDecoder(std::string const & filename, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  PISTR(new std::ifstream(filename.c_str(),std::ios::binary)),
			  GZ(*PISTR),
			  bamheader(GZ)
			{}
			
			BamDecoder(std::istream & in, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  PISTR(), GZ(in),
			  bamheader(GZ)
			{}

			libmaus::bambam::BamHeader const & getHeader() const
			{
				return bamheader;
			}
						
			bool readAlignmentInternal(bool const delayPutRank = false)
			{
				bool const ok = readAlignmentGz(GZ,alignment,&bamheader,validate);
				
				if ( ! ok )
					return false;
			
				if ( ! delayPutRank )
					putRank();
			
				return true;
			}			
		};
		
		struct BamDecoderWrapper
		{
			libmaus::bambam::BamDecoder bamdec;
			
			BamDecoderWrapper(std::string const & filename, bool const rputrank = false)
			: bamdec(filename,rputrank) {}
			BamDecoderWrapper(std::istream & in, bool const rputrank = false)
			: bamdec(in,rputrank) {}
		};
	}
}
#endif
