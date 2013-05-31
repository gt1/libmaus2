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
#if ! defined(LIBMAUS_BAMBAM_BAMDECODER_HPP)
#define LIBMAUS_BAMBAM_BAMDECODER_HPP

#include <libmaus/bambam/BamAlignmentDecoder.hpp>
#include <libmaus/lz/BgzfInflateStream.hpp>
#include <libmaus/lz/BgzfInflateParallelStream.hpp>

namespace libmaus
{
	namespace bambam
	{	
		template<typename _bgzf_type>			
		struct BamDecoderTemplate : public libmaus::bambam::BamAlignmentDecoder
		{
			typedef _bgzf_type bgzf_type;
			typedef typename bgzf_type::unique_ptr_type bgzf_ptr_type;
			typedef BamDecoderTemplate<bgzf_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			::libmaus::util::unique_ptr< std::ifstream >::type PISTR;
			bgzf_ptr_type PGZ;
			bgzf_type & GZ;

			public:
			::libmaus::bambam::BamHeader bamheader;
						
			public:
			BamDecoderTemplate(std::string const & filename, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  PISTR(new std::ifstream(filename.c_str(),std::ios::binary)),
			  PGZ(new bgzf_type(*PISTR)), GZ(*PGZ),
			  bamheader(GZ)
			{}
			
			BamDecoderTemplate(std::istream & in, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  PISTR(), PGZ(new bgzf_type(in)), GZ(*PGZ),
			  bamheader(GZ)
			{}

			BamDecoderTemplate(bgzf_type & rGZ, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  PISTR(), PGZ(), GZ(rGZ),
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

		typedef BamDecoderTemplate<libmaus::lz::BgzfInflateStream> BamDecoder;
		typedef BamDecoderTemplate<libmaus::lz::BgzfInflateParallelStream> BamParallelDecoder;
		
		struct BamDecoderWrapper
		{
			libmaus::aio::CheckedInputStream::unique_ptr_type Pistr;
			std::istream & istr;
			libmaus::lz::BgzfInflateStream bgzf;
			
			libmaus::bambam::BamDecoder bamdec;
			
			BamDecoderWrapper(std::string const & filename, bool const rputrank = false)
			: Pistr(new libmaus::aio::CheckedInputStream(filename)), 
			  istr(*Pistr),
			  bgzf(istr),
			  bamdec(bgzf,rputrank) {}
			BamDecoderWrapper(std::istream & ristr, bool const rputrank = false)
			: Pistr(), istr(ristr), bgzf(istr), bamdec(bgzf,rputrank) {}
			BamDecoderWrapper(std::istream & ristr, std::ostream & copyout, bool const rputrank = false)
			: Pistr(), istr(ristr), bgzf(istr,copyout), bamdec(bgzf,rputrank) {}
		};

		struct BamParallelDecoderWrapper
		{
			libmaus::aio::CheckedInputStream::unique_ptr_type Pistr;
			std::istream & istr;
			libmaus::lz::BgzfInflateParallelStream bgzf;
			libmaus::bambam::BamParallelDecoder bamdec;
			
			BamParallelDecoderWrapper(
				std::string const & filename,
				uint64_t const numthreads,
				bool const rputrank = false
			)
			: Pistr(new libmaus::aio::CheckedInputStream(filename)), 
			  istr(*Pistr),
			  bgzf(istr,numthreads,4*numthreads),
			  bamdec(bgzf,rputrank) 
			{}
			BamParallelDecoderWrapper(
				std::string const & filename,
				std::ostream & copyostr,
				uint64_t const numthreads,
				bool const rputrank = false
			)
			: Pistr(new libmaus::aio::CheckedInputStream(filename)), 
			  istr(*Pistr),
			  bgzf(istr,copyostr,numthreads,4*numthreads),
			  bamdec(bgzf,rputrank) 
			{}
			BamParallelDecoderWrapper(
				std::istream & in, 
				uint64_t const numthreads,
				bool const rputrank = false
			)
			: Pistr(), istr(in), bgzf(istr,numthreads,4*numthreads), bamdec(bgzf,rputrank) {}
			BamParallelDecoderWrapper(
				std::istream & in, 
				std::ostream & copyostr,
				uint64_t const numthreads,
				bool const rputrank = false
			)
			: Pistr(), istr(in), bgzf(istr,copyostr,numthreads,4*numthreads), bamdec(bgzf,rputrank) {}
		};
	}
}
#endif
