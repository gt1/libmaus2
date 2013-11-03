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
#if ! defined(LIBMAUS_BAMBAM_BAMENTRYCONTAINER_HPP)
#define LIBMAUS_BAMBAM_BAMENTRYCONTAINER_HPP

#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/lz/SnappyCompress.hpp>
#include <libmaus/util/GetObject.hpp>
#include <libmaus/util/CountGetObject.hpp>
#include <libmaus/bambam/BamWriter.hpp>
#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/bambam/BamAlignmentPosComparator.hpp>
#include <libmaus/bambam/BamAlignmentNameComparator.hpp>
#include <libmaus/bambam/BamAlignmentHeapComparator.hpp>
#include <queue>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * container class for BAM alignments; used for sorting BAM
		 **/
		template<typename _comparator_type>
		struct BamEntryContainer
		{
			//! comparator type
			typedef _comparator_type comparator_type;
			//! this type
			typedef BamEntryContainer<comparator_type> this_type;

			private:
			//! buffer data type
			typedef uint64_t data_type;
			//! size of snappy blocks in bytes
			static unsigned int const snappyoutputbufsize = 256*1024;
			//! sort buffer
			::libmaus::autoarray::AutoArray<data_type> B;
			
			//! pointer/offset insert pointer descending from back of B
			uint64_t * pp;
			//! buffer start pointer
			uint8_t * const pa;
			//! buffer current pointer
			uint8_t * pc;

			//! temporary file name
			std::string const tmpfileoutname;
			//! temporary output stream
			::libmaus::aio::CheckedOutputStream::unique_ptr_type Ptmpfileout;
			
			//! uint64_t pair
			typedef std::pair<uint64_t,uint64_t> upair;
			//! file position intervals of temp file blocks
			std::vector<upair> tmpoffsetintervals;
			//! number of alignments in each block in temp file
			std::vector<uint64_t> tmpoutcnts;
			
			//! comparator for alignments
			comparator_type const BAPC;

			/**
			 * @return temporary file output stream
			 **/
			::libmaus::aio::CheckedOutputStream & getTmpFileOut()
			{
				if ( ! Ptmpfileout )
				{
					::libmaus::aio::CheckedOutputStream::unique_ptr_type t(new ::libmaus::aio::CheckedOutputStream(tmpfileoutname));
					Ptmpfileout = UNIQUE_PTR_MOVE(t);
				}
				return *Ptmpfileout;
			}

			/**
			 * @return amount of free space in buffer in bytes
			 **/
			uint64_t freeSpace() const
			{
				return reinterpret_cast<uint8_t const *>(pp)-pc;
			}

			/**
			 * create final sorted output
			 *
			 * @param writer BAM writer object
			 * @param verbose if true then progress information will be printed on std::cerr
			 **/
			template<typename writer_type>
			void createOutput(writer_type & writer, int const verbose = 0)
			{
				if ( verbose )
					std::cerr << "[V] producing sorted output" << std::endl;

				// if we already wrote data to disk
				if ( tmpoutcnts.size() )
				{
					// flush buffer
					flush();
					// flush file
					if ( Ptmpfileout )
					{
						Ptmpfileout->flush();
						Ptmpfileout->close();
					}
					// deallocate file
					Ptmpfileout.reset();
					// deallocate buffer
					B.release();
		
					// number of input blocks to be merged			
					uint64_t const nummerge = tmpoutcnts.size();
					::libmaus::autoarray::AutoArray< ::libmaus::bambam::BamAlignment > algns(nummerge);
					::libmaus::bambam::BamAlignmentHeapComparator<comparator_type> heapcmp(BAPC,algns.begin());
					::libmaus::autoarray::AutoArray<uint64_t> fileoffsets(nummerge+1,false);
					for ( uint64_t i = 0; i < nummerge; ++i )
						fileoffsets [ i ] = tmpoffsetintervals[i].first;
					fileoffsets[nummerge] = tmpoffsetintervals[nummerge-1].second;
					
					::libmaus::lz::SnappyInputStreamArrayFile SISAF(tmpfileoutname,fileoffsets.begin(),fileoffsets.end());
					::std::priority_queue< uint64_t, std::vector<uint64_t>, ::libmaus::bambam::BamAlignmentHeapComparator<comparator_type> > Q(heapcmp);
					for ( uint64_t i = 0; i < nummerge; ++i )
						if ( tmpoutcnts[i]-- )
						{
							::libmaus::bambam::BamDecoder::readAlignmentGz(SISAF[i],algns[i]);
							Q.push(i);
						}
						
					uint64_t outcnt = 0;
						
					while ( Q.size() )
					{
						uint64_t const t = Q.top(); Q.pop();				
						// algns[t].serialise(writer.bgzfos);
						writer.writeAlgn(algns[t]);
						
						if ( verbose && (++outcnt % (1024*1024) == 0) )
							std::cerr << "[V] " << outcnt/(1024*1024) << "M" << std::endl;

						if ( tmpoutcnts[t]-- )
						{
							::libmaus::bambam::BamDecoder::readAlignmentGz(SISAF[t],algns[t],0 /* bamheader */, false /* do not validate */);
							Q.push(t);
						}
					}
					
					if ( verbose )
						std::cerr << "[V] wrote " << outcnt << " alignments" << std::endl;			
				}
				// all alignments are still in memory
				else
				{
					// sort entries
					comparator_type BAPC(pa);
					std::reverse(pp,B.end());
					std::stable_sort(pp,B.end(),BAPC);		

					uint64_t const numel = B.end()-pp;
					// write entries
					uint64_t outcnt = 0;
					for ( uint64_t i = 0; i < numel; ++i )
					{
						uint64_t const off = pp[i];
						char const * data = reinterpret_cast<char const *>(pa + off);
						writer.writeData(data);
						
						#if 0
						::libmaus::util::CountGetObject<char const *> G(data);
						uint64_t const blocksize = ::libmaus::bambam::DecoderBase::getLEInteger(G,4);				
						writer.bgzfos.write(data,sizeof(uint32_t) + blocksize);
						#endif

						if ( verbose && (++outcnt % (1024*1024) == 0) )
							std::cerr << "[V]" << outcnt/(1024*1024) << std::endl;
					}

					if ( verbose )
						std::cerr << "[V] wrote " << outcnt << " alignments" << std::endl;			

					pc = pa;
					pp = B.end();
				}
			}


			public:
			/**
			 * constructor
			 *
			 * @param bufsize size of buffer in bytes
			 * @param rtmpfileoutname temp file name
			 **/
			BamEntryContainer(uint64_t const bufsize, std::string const & rtmpfileoutname)
			: B( bufsize/sizeof(data_type), false ), pp(B.end()),
			  pa(reinterpret_cast<uint8_t *>(B.begin())),
			  pc(pa),
			  tmpfileoutname(rtmpfileoutname),
			  BAPC(pa)
			{
				assert ( B.size() ) ;
			}
			
			/**
			 * flush buffer
			 **/
			void flush()
			{
				uint64_t const numel = B.end()-pp;
				
				if ( numel )
				{
					// std::cerr << "flushing for " << numel << std::endl;
					comparator_type BAPC(pa);
					std::reverse(pp,B.end());
					std::stable_sort(pp,B.end(),BAPC);
					
					::libmaus::aio::CheckedOutputStream & tempfile = getTmpFileOut();
					uint64_t const prepos = tempfile.tellp();
					::libmaus::lz::SnappyOutputStream< ::libmaus::aio::CheckedOutputStream > SOS(tempfile,snappyoutputbufsize);
									
					// write entries
					for ( uint64_t i = 0; i < numel; ++i )
					{
						uint64_t const off = pp[i];
						char const * data = reinterpret_cast<char const *>(pa + off);
						::libmaus::util::CountGetObject<char const *> G(data);
						uint64_t const blocksize = ::libmaus::bambam::DecoderBase::getLEInteger(G,4);				
						SOS.write(data,4+blocksize);
					}

					SOS.flush();
					uint64_t const postpos = tempfile.tellp();
							
					// file positions
					tmpoffsetintervals.push_back(upair(prepos,postpos));
					// number of elements
					tmpoutcnts.push_back(numel);

					// printBuffer(std::cerr);

					pc = pa;
					pp = B.end();
				
					// std::cerr << "flushed." << std::endl;
				
					// exit(0);
				}
			}
			
			/**
			 * print current content of buffer to out
			 *
			 * @param out output stream
			 **/
			void printBuffer(std::ostream & out) const
			{
				uint64_t const numel = B.end()-pp;
				
				for ( uint64_t i = 0; i < numel; ++i )
				{
					uint64_t const off = pp[i];
					::libmaus::util::CountGetObject<char const *> G( reinterpret_cast<char const *>(pa + off));
					::libmaus::bambam::BamAlignment lalgn(G);

					int32_t const refid = ::libmaus::bambam::BamAlignmentDecoderBase::getRefID(
						reinterpret_cast<uint8_t const *>(pa + off + sizeof(uint32_t))
					);
					int32_t const pos = ::libmaus::bambam::BamAlignmentDecoderBase::getPos(
						reinterpret_cast<uint8_t const *>(pa + off + sizeof(uint32_t))
					);
					
					out << lalgn.getName() << "\t" << refid << "\t" << pos << std::endl;
				}
			}
			
			/**
			 * put an alignment in the buffer
			 *
			 * @param algn alignment to be put in buffer
			 **/
			void putAlignment(::libmaus::bambam::BamAlignment const & algn)
			{
				uint64_t const reqsize = algn.serialisedByteSize() + sizeof(data_type);
				
				if ( reqsize > freeSpace() )
					flush();
				
				// put pointer
				*(--pp) = (pc-pa);
				// put data
				::libmaus::util::PutObject<char *> P(reinterpret_cast<char *>(pc));
				algn.serialise(P);
				pc = reinterpret_cast<uint8_t *>(P.p);
			}
			
			/**
			 * base class for BAM alignments writer class
			 **/
			struct WriterWrapperBase
			{
				//! destructor
				virtual ~WriterWrapperBase() {}
				/**
				 * write blocksize bytes of data
				 *
				 * @param data pointer to data
				 * @param blocksize size of block to be written
				 **/
				virtual void write(char const * data, uint32_t const blocksize) = 0;

				/**
				 * write alignment block starting at data
				 * 
				 * @param data pointer to alignment block prefix with block length
				 **/
				virtual void writeData(char const * data)
				{
					// decode blocksize
					::libmaus::util::CountGetObject<char const *> G(data);
					uint64_t const blocksize = ::libmaus::bambam::DecoderBase::getLEInteger(G,4);
					// write block
					write(data+sizeof(uint32_t),blocksize);
				}
				
				/**
				 * write alignment object
				 *
				 * @param algn alignment object
				 **/
				virtual void writeAlgn(::libmaus::bambam::BamAlignment const & algn)
				{
					write ( reinterpret_cast<char const *>(algn.D.begin()), algn.blocksize );
				}

			};
			
			/**
			 * specialisation of WriterWrapperBase for writing BAM files
			 **/
			template<typename _writer_type>
			struct BamWriterWrapper : public WriterWrapperBase
			{
				typedef _writer_type writer_type;
				
				//! BAM writer object
				writer_type & writer;
				
				/**
				 * constructor
				 *
				 * @param rwriter BAM writer object
				 **/
				BamWriterWrapper(writer_type & rwriter) : writer(rwriter) {}
				
				/**
				 * write alignment block of size blocksize
				 *
				 * @param data alignment data block
				 * @param blocksize size of block
				 **/
				virtual void write(char const * data, uint32_t const blocksize)
				{
					writer.writeBamBlock(reinterpret_cast<uint8_t const *>(data),blocksize);
				}
			};
			
			/**
			 * create final sorted output
			 *
			 * @param stream output stream
			 * @param bamheader BAM file header
			 * @param level compression level
			 * @param verbose if true then progress information will be printed on std::cerr
			 **/
			template<typename stream_type>
			void createOutput(
				stream_type & stream, 
				::libmaus::bambam::BamHeader const & bamheader, 
				int const level = Z_DEFAULT_COMPRESSION, 
				int const verbose = 0,
				std::vector< ::libmaus::lz::BgzfDeflateOutputCallback *> const * blockoutputcallbacks = 0
			)
			{
				::libmaus::bambam::BamWriter writer(stream,bamheader,level,blockoutputcallbacks);
				BamWriterWrapper< ::libmaus::bambam::BamWriter > BWW(writer);
				createOutput(BWW,verbose);
			}

			/**
			 * create final sorted output
			 *
			 * @param stream output stream
			 * @param bamheader BAM file header
			 * @param level compression level
			 * @param verbose if true then progress information will be printed on std::cerr
			 **/
			void createOutput(
				libmaus::bambam::BamBlockWriterBase & writerbase,
				int const level = Z_DEFAULT_COMPRESSION, 
				int const verbose = 0,
				std::vector< ::libmaus::lz::BgzfDeflateOutputCallback *> const * blockoutputcallbacks = 0
			)
			{
				BamWriterWrapper< libmaus::bambam::BamBlockWriterBase > BWW(writerbase);
				createOutput(BWW,verbose);
			}
			
		};
	}
}
#endif
