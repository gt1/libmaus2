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
#include <libmaus/lz/BgzfInflateDeflateParallelInputStream.hpp>
#include <libmaus/aio/InputStream.hpp>
#include <libmaus/aio/InputStreamFactoryContainer.hpp>

namespace libmaus
{
	namespace bambam
	{	
		/**
		 * BAM file decoding class
		 **/
		template<typename _bgzf_type>			
		struct BamDecoderTemplate : public libmaus::bambam::BamAlignmentDecoder
		{
			//! decompressor type
			typedef _bgzf_type bgzf_type;
			//! decompressor pointer type
			typedef typename bgzf_type::unique_ptr_type bgzf_ptr_type;
			//! this type
			typedef BamDecoderTemplate<bgzf_type> this_type;
			//! unique pointer type
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			libmaus::aio::InputStream::unique_ptr_type AISTR;
			//! decmopressor pointer
			bgzf_ptr_type PGZ;
			//! decompressor
			bgzf_type * GZ;
			//! BAM header pointer
			::libmaus::bambam::BamHeader::unique_ptr_type Pbamheader;
			//! BAM header
			::libmaus::bambam::BamHeader const & bamheader;

			/**
			 * interval alignment input method
			 *
			 * @param delayPutRank if true, then rank aux tag will not be inserted by this call
			 * @return true iff next alignment could be successfully read, false if no more alignment were available
			 **/
			bool readAlignmentInternal(bool const delayPutRank = false)
			{
				bool const ok = readAlignmentGz(*GZ,alignment,&bamheader,validate);
				
				if ( ! ok )
					return false;
			
				if ( ! delayPutRank )
					putRank();
			
				return true;
			}
		
			public:
			/**
			 * constructor by file name
			 *
			 * @param filename name of input BAM file
			 * @param rputrank if true, then a rank auxiliary tag will be attached to each alignment
			 **/
			BamDecoderTemplate(std::string const & filename, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  AISTR(libmaus::aio::InputStreamFactoryContainer::constructUnique(filename)),
			  PGZ(new bgzf_type(*AISTR)), GZ(PGZ.get()),
			  Pbamheader(new BamHeader(*GZ)),
			  bamheader(*Pbamheader)
			{}
			
			/**
			 * constructor by compressed input stream
			 *
			 * @param in input stream delivering BAM
			 * @param rputrank if true, then a rank auxiliary tag will be attached to each alignment
			 **/
			BamDecoderTemplate(std::istream & in, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  AISTR(new libmaus::aio::InputStream(in)), PGZ(new bgzf_type(*AISTR)), GZ(PGZ.get()),
			  Pbamheader(new BamHeader(*GZ)),
			  bamheader(*Pbamheader)
			{}

			/**
			 * constructor by compressed input stream
			 *
			 * @param in input stream delivering BAM
			 * @param rputrank if true, then a rank auxiliary tag will be attached to each alignment
			 **/
			BamDecoderTemplate(libmaus::aio::InputStream::unique_ptr_type & rAISTR, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  AISTR(UNIQUE_PTR_MOVE(rAISTR)), PGZ(new bgzf_type(*AISTR)), GZ(PGZ.get()),
			  Pbamheader(new BamHeader(*GZ)),
			  bamheader(*Pbamheader)
			{}

			/**
			 * constructor by decompressor object
			 *
			 * @param rGZ decompressor object
			 * @param rputrank if true, then a rank auxiliary tag will be attached to each alignment
			 **/
			BamDecoderTemplate(bgzf_type & rGZ, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  AISTR(), PGZ(), GZ(&rGZ),
			  Pbamheader(new BamHeader(*GZ)),
			  bamheader(*Pbamheader)
			{}

			/**
			 * constructor by decompressor object and BAM header
			 *
			 * @param rGZ decompressor object
			 * @param rheader BAM header object
			 * @param rputrank if true, then a rank auxiliary tag will be attached to each alignment
			 **/
			BamDecoderTemplate(bgzf_type & rGZ, libmaus::bambam::BamHeader const & rheader, bool const rputrank = false)
			: 
			  libmaus::bambam::BamAlignmentDecoder(rputrank),
			  AISTR(), PGZ(), GZ(&rGZ),
			  Pbamheader(),
			  bamheader(rheader)
			{}
			
			/**
			 * constructor from header. A compressed stream needs to be set before
			 * alignments can be extracted.
			 *
			 * @param rheader BAM header object
			 **/
			BamDecoderTemplate(libmaus::bambam::BamHeader const & rheader)
			:
			  libmaus::bambam::BamAlignmentDecoder(false),
			  AISTR(), PGZ(), GZ(0), Pbamheader(), bamheader(rheader)
			{
			
			}

			/**
			 * @return BAM header
			 **/
			libmaus::bambam::BamHeader const & getHeader() const
			{
				return bamheader;
			}
						
			/**
			 * @return decompressor object
			 **/
			bgzf_type & getStream()
			{
				return *GZ;
			}			
			
			/**
			 * @return true if there is a decompressor object
			 **/
			bool hasStream()
			{
				return GZ != 0;
			}
			
			/**
			 * set decompressor object
			 *
			 * @param rGZ decompressor object
			 **/
			void setStream(bgzf_type * rGZ)
			{
				GZ = rGZ;
			}			
		};

		//! BAM file decoding class based on serial bgzf input
		typedef BamDecoderTemplate<libmaus::lz::BgzfInflateStream> BamDecoder;
		//! BAM file decoding class based on parallel bgzf input
		typedef BamDecoderTemplate<libmaus::lz::BgzfInflateParallelStream> BamParallelDecoder;
		//! BAM file decoding class based on parallel bgzf input
		typedef BamDecoderTemplate<libmaus::lz::BgzfInflateDeflateParallelInputStream> BamParallelRewriteDecoder;
		
		/**
		 * wrapper class for serial BAM decoder
		 **/
		struct BamDecoderWrapper : public BamAlignmentDecoderWrapper
		{
			//! this type
			typedef BamDecoderWrapper this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			protected:
			//!
			libmaus::aio::InputStream::unique_ptr_type AISTR;
			//! compressed input stream
			std::istream & istr;
			//! decompressor
			libmaus::lz::BgzfInflateStream::unique_ptr_type bgzf;
			//! BAM file decoder
			libmaus::bambam::BamDecoder bamdec;
			
			public:
			/**
			 * constructor from file name
			 *
			 * @param filename input file name
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamDecoderWrapper(std::string const & filename, bool const rputrank = false)
			: AISTR(libmaus::aio::InputStreamFactoryContainer::constructUnique(filename)),
			  istr(*AISTR),
			  bgzf(new libmaus::lz::BgzfInflateStream(istr)),
			  bamdec(*bgzf,rputrank) {}

			/**
			 * constructor from file name and virtual offsets
			 *
			 * @param filename input file name
			 * @param startoffset virtual start offset
			 * @param endoffset virtual end offset
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamDecoderWrapper(
				std::string const & filename,
				libmaus::bambam::BamHeader const & header,
				libmaus::lz::BgzfVirtualOffset const & startoffset,
				libmaus::lz::BgzfVirtualOffset const & endoffset,
				bool const rputrank = false)
			: AISTR(libmaus::aio::InputStreamFactoryContainer::constructUnique(filename)),
			  istr(*AISTR),
			  bgzf(new libmaus::lz::BgzfInflateStream(istr,startoffset,endoffset)),
			  bamdec(*bgzf,header,rputrank) {}

			/**
			 * constructor from input stream
			 *
			 * @param ristr input stream
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamDecoderWrapper(libmaus::aio::InputStream::unique_ptr_type & rAISTR, bool const rputrank = false)
			: AISTR(UNIQUE_PTR_MOVE(rAISTR)), istr(*AISTR), bgzf(new libmaus::lz::BgzfInflateStream(istr)), bamdec(*bgzf,rputrank) {}
			/**
			 * constructor from input stream
			 *
			 * @param ristr input stream
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamDecoderWrapper(std::istream & ristr, bool const rputrank = false)
			: AISTR(new libmaus::aio::InputStream(ristr)), istr(*AISTR), bgzf(new libmaus::lz::BgzfInflateStream(istr)), bamdec(*bgzf,rputrank) {}

			/**
			 * constructor from input stream, header and interval (input stream needs to be seekable)
			 *
			 * @param ristr input stream
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamDecoderWrapper(
				std::istream & ristr, 
				libmaus::bambam::BamHeader const & header,
				libmaus::lz::BgzfVirtualOffset const & startoffset,
				libmaus::lz::BgzfVirtualOffset const & endoffset,
				bool const rputrank = false
			)
			: AISTR(new libmaus::aio::InputStream(ristr)), istr(*AISTR), bgzf(new libmaus::lz::BgzfInflateStream(istr,startoffset,endoffset)), bamdec(*bgzf,header,rputrank) {}
			/**
			 * constructor from input stream, header and interval (input stream needs to be seekable)
			 *
			 * @param ristr input stream
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamDecoderWrapper(
				libmaus::aio::InputStream::unique_ptr_type & rAISTR, 
				libmaus::bambam::BamHeader const & header,
				libmaus::lz::BgzfVirtualOffset const & startoffset,
				libmaus::lz::BgzfVirtualOffset const & endoffset,
				bool const rputrank = false
			)
			: AISTR(UNIQUE_PTR_MOVE(rAISTR)), istr(*AISTR), bgzf(new libmaus::lz::BgzfInflateStream(istr,startoffset,endoffset)), bamdec(*bgzf,header,rputrank) {}

			/**
			 * constructor from input stream and output stream;
			 * the original input stream will be copied to the output stream
			 * while processing the BAM file
			 *
			 * @param ristr input stream
			 * @param copyout output stream
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamDecoderWrapper(std::istream & ristr, std::ostream & copyout, bool const rputrank = false)
			: AISTR(new libmaus::aio::InputStream(ristr)), istr(*AISTR), bgzf(new libmaus::lz::BgzfInflateStream(istr,copyout)), bamdec(*bgzf,rputrank) {}
			/**
			 * constructor from input stream and output stream;
			 * the original input stream will be copied to the output stream
			 * while processing the BAM file
			 *
			 * @param ristr input stream
			 * @param copyout output stream
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamDecoderWrapper(libmaus::aio::InputStream::unique_ptr_type & rAISTR, std::ostream & copyout, bool const rputrank = false)
			: AISTR(UNIQUE_PTR_MOVE(rAISTR)), istr(*AISTR), bgzf(new libmaus::lz::BgzfInflateStream(istr,copyout)), bamdec(*bgzf,rputrank) {}
			
			/**
			 * @return wrapped decoder
			 **/
			libmaus::bambam::BamAlignmentDecoder & getDecoder()
			{
				return bamdec;
			}
		};

		/**
		 * wrapper class for parallel BAM decoder
		 **/
		struct BamParallelDecoderWrapper : public BamAlignmentDecoderWrapper
		{
			protected:
			//!
			libmaus::aio::InputStream::unique_ptr_type AISTR;
			//! input stream reference
			std::istream & istr;
			//! decompressor object
			libmaus::lz::BgzfInflateParallelStream bgzf;
			//! BAM decoder
			libmaus::bambam::BamParallelDecoder bamdec;
			
			public:
			/**
			 * constructor from file name
			 *
			 * @param filename input file name
			 * @param numthreads number of decoding threads
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamParallelDecoderWrapper(
				std::string const & filename,
				uint64_t const numthreads,
				bool const rputrank = false
			)
			: AISTR(libmaus::aio::InputStreamFactoryContainer::constructUnique(filename)),
			  istr(*AISTR),
			  bgzf(istr,numthreads,4*numthreads),
			  bamdec(bgzf,rputrank) 
			{}

			/**
			 * constructor from stream
			 *
			 * @param filename input file name
			 * @param numthreads number of decoding threads
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamParallelDecoderWrapper(
				libmaus::aio::InputStream::unique_ptr_type & rAISTR,
				uint64_t const numthreads,
				bool const rputrank = false
			)
			: AISTR(UNIQUE_PTR_MOVE(rAISTR)),
			  istr(*AISTR),
			  bgzf(istr,numthreads,4*numthreads),
			  bamdec(bgzf,rputrank) 
			{}
			
			/**
			 * constructor from input file name and output stream;
			 * the original input stream will be copied to the output stream
			 * while processing the BAM file
			 *
			 * @param filename input file name
			 * @param copyostr output stream
			 * @param numthreads number of decoding threads
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamParallelDecoderWrapper(
				std::string const & filename,
				std::ostream & copyostr,
				uint64_t const numthreads,
				bool const rputrank = false
			)
			: AISTR(libmaus::aio::InputStreamFactoryContainer::constructUnique(filename)),
			  istr(*AISTR),
			  bgzf(istr,copyostr,numthreads,4*numthreads),
			  bamdec(bgzf,rputrank) 
			{}

			/**
			 * constructor from input file name and output stream;
			 * the original input stream will be copied to the output stream
			 * while processing the BAM file
			 *
			 * @param filename input file name
			 * @param copyostr output stream
			 * @param numthreads number of decoding threads
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamParallelDecoderWrapper(
				libmaus::aio::InputStream::unique_ptr_type & rAISTR,
				std::ostream & copyostr,
				uint64_t const numthreads,
				bool const rputrank = false
			)
			: AISTR(UNIQUE_PTR_MOVE(rAISTR)),
			  istr(*AISTR),
			  bgzf(istr,copyostr,numthreads,4*numthreads),
			  bamdec(bgzf,rputrank) 
			{}
			
			/**
			 * constructor by input stream
			 *
			 * @param in input stream
			 * @param numthreads number of decoding threads
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamParallelDecoderWrapper(
				std::istream & in, 
				uint64_t const numthreads,
				bool const rputrank = false
			)
			: AISTR(new libmaus::aio::InputStream(in)), istr(*AISTR), bgzf(istr,numthreads,4*numthreads), bamdec(bgzf,rputrank) {}
			
			/**
			 * constructor from input stream and output stream;
			 * the original input stream will be copied to the output stream
			 * while processing the BAM file
			 *
			 * @param in input stream
			 * @param copyostr output stream
			 * @param numthreads number of decoding threads
			 * @param rputrank add rank aux field to each alignment at time of reading
			 **/
			BamParallelDecoderWrapper(
				std::istream & in, 
				std::ostream & copyostr,
				uint64_t const numthreads,
				bool const rputrank = false
			)
			: AISTR(new libmaus::aio::InputStream(in)), istr(*AISTR), bgzf(istr,copyostr,numthreads,4*numthreads), bamdec(bgzf,rputrank) {}

			/**
			 * @return wrapped decoder
			 **/
			libmaus::bambam::BamAlignmentDecoder & getDecoder()
			{
				return bamdec;
			}
		};

		/**
		 * resetable wrapper class for serial BAM decoder
		 **/
		struct BamDecoderResetableWrapper : public BamAlignmentDecoderWrapper
		{
			//! this type
			typedef BamDecoderResetableWrapper this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			protected:
			//!
			libmaus::aio::InputStream::unique_ptr_type AISTR;
			//! decompressor
			libmaus::lz::BgzfInflateStream::unique_ptr_type bgzf;
			//! BAM file decoder
			libmaus::bambam::BamDecoder bamdec;
			
			public:
			/**
			 * constructor from file name and header. The wrapped decoder is not ready for
			 * reading alignments until resetStream() is called.
			 *
			 * @param filename input file name
			 * @param header BAM header object
			 **/
			BamDecoderResetableWrapper(std::string const & filename, libmaus::bambam::BamHeader const & header)
			: AISTR(libmaus::aio::InputStreamFactoryContainer::constructUnique(filename)), bgzf(), bamdec(header)
			{
			
			}
			
			void resetStream(
				libmaus::lz::BgzfVirtualOffset const & startoffset,
				libmaus::lz::BgzfVirtualOffset const & endoffset
			)
			{
				AISTR->clear();
				
				libmaus::lz::BgzfInflateStream::unique_ptr_type tbgzf(
                                                new libmaus::lz::BgzfInflateStream(
                                                        *AISTR,startoffset,endoffset
                                                )
                                        );
				bgzf = UNIQUE_PTR_MOVE(tbgzf);

				bamdec.setStream(bgzf.get());
			}

			/**
			 * @return wrapped decoder
			 **/
			libmaus::bambam::BamAlignmentDecoder & getDecoder()
			{
				return bamdec;
			}
		};
	}
}
#endif
