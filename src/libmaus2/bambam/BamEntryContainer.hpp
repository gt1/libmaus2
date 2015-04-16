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
#include <libmaus/bambam/BamAlignmentHeapComparator.hpp>
#include <libmaus/bambam/BamAlignmentNameComparator.hpp>
#include <libmaus/bambam/BamAlignmentPosComparator.hpp>
#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/bambam/BamWriter.hpp>
#include <libmaus/bambam/MdNmRecalculation.hpp>
#include <libmaus/lz/SimpleCompressedConcatInputStream.hpp>
#include <libmaus/lz/SimpleCompressedOutputStream.hpp>
#include <libmaus/lz/SimpleCompressedStreamInterval.hpp>
#include <libmaus/lz/SnappyCompressorObjectFactory.hpp>
#include <libmaus/lz/SnappyDecompressorObjectFactory.hpp>
#include <libmaus/sorting/ParallelStableSort.hpp>
#include <libmaus/util/CountGetObject.hpp>
#include <libmaus/util/GetObject.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>
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
			//! compressed stream type
			typedef ::libmaus::lz::SimpleCompressedOutputStream< ::libmaus::aio::CheckedOutputStream > compressed_stream_type;
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

			//! temporary file name base
			std::string const tmpfileoutnamebase;
			//! temporary file names
			std::vector<std::string> tmpfileoutnames;
			//! temporary output streams
			::libmaus::autoarray::AutoArray< ::libmaus::aio::CheckedOutputStream::unique_ptr_type> Ptmpfileout;
			//! temporary output compressed streams
			::libmaus::autoarray::AutoArray< compressed_stream_type::unique_ptr_type> Ptmpfilecompout;
			//! intervals
			std::vector < std::vector< ::libmaus::lz::SimpleCompressedStreamInterval> > tmpfileintervals;
						
			//! uint64_t pair
			typedef std::pair<uint64_t,uint64_t> upair;
			//! file position intervals of temp file blocks
			std::vector<upair> tmpoffsetintervals;
			//! number of alignments in each block in temp file
			std::vector<uint64_t> tmpoutcnts;
			
			//! comparator for alignments
			comparator_type const BAPC;
			
			//! parallel processing (number of threads used for block sorting)
			uint64_t const parallel;

			/**
			 * pack alignment pointers at end of array
			 **/
			void packPointers()
			{
				if ( parallel > 1 )
				{
					uint64_t * outpp = B.end();
					uint64_t * inpp = B.end();
				
					while ( inpp != pp )
					{
						*(--outpp) = *(--inpp);
						--inpp;
					}
				
					pp = outpp;
				}
			}

			/**
			 * @return temporary file output stream
			 **/
			compressed_stream_type & getTmpFileOut(uint64_t const j)
			{
				if ( ! Ptmpfileout.size() )
				{
					uint64_t const numfiles = std::max(parallel,static_cast<uint64_t>(1ull));

					Ptmpfileout = ::libmaus::autoarray::AutoArray< ::libmaus::aio::CheckedOutputStream::unique_ptr_type>(numfiles);
					Ptmpfilecompout = ::libmaus::autoarray::AutoArray< ::libmaus::lz::SimpleCompressedOutputStream< ::libmaus::aio::CheckedOutputStream >::unique_ptr_type>(numfiles);

					tmpfileoutnames.resize(numfiles);
					
					libmaus::lz::SnappyCompressorObjectFactory compfact;

					for ( uint64_t i = 0; i < numfiles; ++i )
					{
						std::ostringstream fnostr;
						fnostr << tmpfileoutnamebase << "_" << std::setw(6) << std::setfill('0') << i;
						std::string const fn = fnostr.str();
						tmpfileoutnames[i] = fn;
						
						libmaus::util::TempFileRemovalContainer::addTempFile(fn);
					
						::libmaus::aio::CheckedOutputStream::unique_ptr_type t(new ::libmaus::aio::CheckedOutputStream(fn));
						Ptmpfileout[i] = UNIQUE_PTR_MOVE(t);
						
						::libmaus::lz::SimpleCompressedOutputStream< ::libmaus::aio::CheckedOutputStream >::unique_ptr_type tscos(
							new ::libmaus::lz::SimpleCompressedOutputStream< ::libmaus::aio::CheckedOutputStream >(*(Ptmpfileout[i]),compfact,64*1024));
						Ptmpfilecompout[i] = UNIQUE_PTR_MOVE(tscos);
					}
				}
				
				assert ( Ptmpfileout.size() );
				assert ( j < Ptmpfileout.size() );
				
				return *(Ptmpfilecompout[j]);
			}
			/**
			 * flush temp files
			 **/
			void flushTmpFileOut()
			{
				for ( uint64_t i = 0; i < Ptmpfileout.size(); ++i )
				{
					Ptmpfilecompout[i]->flush();
					Ptmpfilecompout[i].reset();
					Ptmpfileout[i]->flush();
					Ptmpfileout[i]->close();						
					Ptmpfileout[i].reset();
				}
			}

			/**
			 * @return amount of free space in buffer in bytes
			 **/
			uint64_t freeSpace() const
			{
				return reinterpret_cast<uint8_t const *>(pp)-pc;
			}

			public:
			/**
			 * constructor
			 *
			 * @param bufsize size of buffer in bytes
			 * @param rtmpfileoutnamebase temp file name
			 **/
			BamEntryContainer(uint64_t const bufsize, std::string const & rtmpfileoutnamebase, uint64_t const rparallel = 1)
			: B( bufsize/sizeof(data_type), false ), pp(B.end()),
			  pa(reinterpret_cast<uint8_t *>(B.begin())),
			  pc(pa),
			  tmpfileoutnamebase(rtmpfileoutnamebase),
			  BAPC(pa),
			  parallel(rparallel)
			{
				assert ( B.size() ) ;
			}
			
			/**
			 * flush buffer
			 **/
			void flush()
			{
				// if buffer is not empty
				if ( (B.end() - pp) )
				{
					packPointers();

					// number of elements in buffer
					uint64_t const numel = B.end()-pp;
						
					// construct comparator
					comparator_type BAPC(pa);
					// reverse pointer array (top to bottom)
					std::reverse(pp,B.end());

					// sort the internal memory buffer
					if ( parallel > 1 )
						libmaus::sorting::ParallelStableSort::parallelSort(
							pp,B.end(),
							pp-numel,pp,
							BAPC,
							parallel
						);
					else
						std::stable_sort(pp,B.end(),BAPC);		
					
					if ( parallel <= 1 )
					{
						compressed_stream_type & tempfile = getTmpFileOut(0);
						
						std::pair<uint64_t,uint64_t> const preoff = tempfile.getOffset();
										
						// write entries
						for ( uint64_t i = 0; i < numel; ++i )
						{
							uint64_t const off = pp[i];
							char const * data = reinterpret_cast<char const *>(pa + off);
							::libmaus::util::CountGetObject<char const *> G(data);
							uint64_t const blocksize = ::libmaus::bambam::DecoderBase::getLEInteger(G,4);
							tempfile.write(data,4+blocksize);						
						}

						std::pair<uint64_t,uint64_t> const postoff = tempfile.getOffset();
						
						// file positions
						tmpfileintervals.push_back(
							std::vector< ::libmaus::lz::SimpleCompressedStreamInterval >(
								1,::libmaus::lz::SimpleCompressedStreamInterval(preoff,postoff)
							)
						);
					}
					else
					{
						if ( !tmpfileintervals.size() )
							for ( int64_t t = 0; t < static_cast<int64_t>(parallel); ++t )
								getTmpFileOut(t);
					
						assert ( parallel > 1 );
						// target elements per thread
						uint64_t const telperthread = (numel + parallel-1)/parallel;
						// number of threads
						uint64_t const threads = (numel + telperthread-1)/telperthread;
						// elements per thread
						uint64_t const elperthread = (numel+threads-1)/threads;

						std::vector< ::libmaus::lz::SimpleCompressedStreamInterval > tmpintervalvec(threads);
						for ( uint64_t i = 0; i < threads; ++i )
							tmpintervalvec[i] = ::libmaus::lz::SimpleCompressedStreamInterval(
								std::pair<uint64_t,uint64_t>(0,0),
								std::pair<uint64_t,uint64_t>(0,0)
							);
							
						#if defined(_OPENMP)
						#pragma omp parallel for num_threads(parallel)
						#endif
						for ( int64_t t = 0; t < static_cast<int64_t>(threads); ++t )
						{
							uint64_t const low = t * elperthread;
							uint64_t const high = std::min(low+elperthread,numel);
		
							compressed_stream_type & tempfile = getTmpFileOut(t);
						
							std::pair<uint64_t,uint64_t> const preoff = tempfile.getOffset();

							// write entries
							for ( uint64_t i = low; i < high; ++i )
							{
								uint64_t const off = pp[i];
								char const * data = reinterpret_cast<char const *>(pa + off);
								::libmaus::util::CountGetObject<char const *> G(data);
								uint64_t const blocksize = ::libmaus::bambam::DecoderBase::getLEInteger(G,4);
								tempfile.write(data,4+blocksize);						
							}

							std::pair<uint64_t,uint64_t> const postoff = tempfile.getOffset();
							
							tmpintervalvec[t] = ::libmaus::lz::SimpleCompressedStreamInterval(preoff,postoff);
						}

						tmpfileintervals.push_back(tmpintervalvec);
					}
					// number of elements
					tmpoutcnts.push_back(numel);

					pc = pa;
					pp = B.end();
				}
			}
			
			/**
			 * put an alignment in the buffer
			 *
			 * @param algn alignment to be put in buffer
			 **/
			void putAlignment(::libmaus::bambam::BamAlignment const & algn)
			{
				uint64_t const reqsize = 
					algn.serialisedByteSize() + 
					((parallel > 1) ? 2 : 1) * sizeof(data_type);
				
				if ( reqsize > freeSpace() )
				{
					flush();
					
					if ( reqsize > freeSpace() )
					{
						libmaus::exception::LibMausException se;
						se.getStream() << "BamEntryContainer::putAlignment(): buffer is too small for single alignment" << std::endl;
						se.finish();
						throw se;
					}
				}
				
				// put pointer
				*(--pp) = (pc-pa);

				// reserve space for parallel processing
				if ( parallel > 1 )
					*(--pp) = 0;
				
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

			private:
			/**
			 * create final sorted output
			 *
			 * @param writer BAM writer object
			 * @param verbose if true then progress information will be printed on std::cerr
			 **/
			template<typename writer_type, bool computemdnm>
			void createOutput(
				writer_type & writer, 
				libmaus::bambam::MdNmRecalculation * mdcalc = 0,
				int const verbose = 0
			)
			{
				if ( computemdnm )
				{
					assert(mdcalc != 0);
				}
			
				if ( verbose )
					std::cerr << "[V] producing sorted output" << std::endl;

				// if we already wrote data to disk
				if ( tmpoutcnts.size() )
				{
					// flush buffer
					flush();
					
					// flush file
					flushTmpFileOut();

					// deallocate buffer
					B.release();
		
					// number of input blocks to be merged			
					uint64_t const nummerge = tmpoutcnts.size();
					
					::libmaus::autoarray::AutoArray< ::libmaus::bambam::BamAlignment > algns(nummerge);
					::libmaus::bambam::BamAlignmentHeapComparator<comparator_type> heapcmp(BAPC,algns.begin());
					
					::libmaus::autoarray::AutoArray< ::libmaus::aio::CheckedInputStream::unique_ptr_type > infiles(tmpfileoutnames.size());
					for ( uint64_t i = 0; i < tmpfileoutnames.size(); ++i )
					{
						::libmaus::aio::CheckedInputStream::unique_ptr_type tptr(new ::libmaus::aio::CheckedInputStream(tmpfileoutnames[i]));
						infiles[i] = UNIQUE_PTR_MOVE(tptr);						
					}
					libmaus::lz::SnappyDecompressorObjectFactory decfact;
					::libmaus::autoarray::AutoArray< ::libmaus::lz::SimpleCompressedConcatInputStream< ::libmaus::aio::CheckedInputStream>::unique_ptr_type > instreams(nummerge);
					
					typedef ::libmaus::lz::SimpleCompressedConcatInputStreamFragment< ::libmaus::aio::CheckedInputStream > fragment_type;
					
					for ( uint64_t i = 0; i < tmpfileintervals.size(); ++i )
					{
						std::vector< ::libmaus::lz::SimpleCompressedStreamInterval> const & subint = tmpfileintervals[i];
						std::vector< fragment_type > frags;
						
						for ( uint64_t j = 0; j < subint.size(); ++j )
							frags.push_back(fragment_type(subint[j].start,subint[j].end,infiles[j].get()));
							
						::libmaus::lz::SimpleCompressedConcatInputStream< ::libmaus::aio::CheckedInputStream>::unique_ptr_type tptr(
							new ::libmaus::lz::SimpleCompressedConcatInputStream< ::libmaus::aio::CheckedInputStream>(
								frags,decfact
							)
						);

						instreams[i] = UNIQUE_PTR_MOVE(tptr);
					}

					::std::priority_queue< uint64_t, std::vector<uint64_t>, ::libmaus::bambam::BamAlignmentHeapComparator<comparator_type> > Q(heapcmp);
					for ( uint64_t i = 0; i < nummerge; ++i )
						if ( tmpoutcnts[i]-- )
						{
							::libmaus::bambam::BamDecoder::readAlignmentGz(*instreams[i],algns[i],0 /* no header for validation */,false /* no validation */);
							Q.push(i);
						}
						
					uint64_t outcnt = 0;
						
					while ( Q.size() )
					{
						uint64_t const t = Q.top(); Q.pop();				
						
						if ( computemdnm )
						{
							if ( mdcalc->calmdnm(algns[t]) )
								algns[t].fillMd(mdcalc->context);			
						}
						
						writer.writeAlgn(algns[t]);
						
						if ( verbose && (++outcnt % (1024*1024) == 0) )
							std::cerr << "[V] " << outcnt/(1024*1024) << "M" << std::endl;

						if ( tmpoutcnts[t]-- )
						{
							::libmaus::bambam::BamDecoder::readAlignmentGz(*instreams[t],algns[t],0 /* bamheader */, false /* do not validate */);
							Q.push(t);
						}
					}
					
					if ( verbose )
						std::cerr << "[V] wrote " << outcnt << " alignments" << std::endl;			
				}
				// all alignments are still in memory
				else
				{
					packPointers();

					// number of alignments in buffer
					uint64_t const numel = B.end()-pp;
					// alignment block for recomputing md/nm
					libmaus::bambam::BamAlignment recompalgn;
				
					// sort entries
					comparator_type BAPC(pa);
					std::reverse(pp,B.end());
					
					if ( parallel > 1 )
						libmaus::sorting::ParallelStableSort::parallelSort(
							pp,B.end(),
							pp-numel,pp,
							BAPC,
							parallel
						);
					else
						std::stable_sort(pp,B.end(),BAPC);		

					// write entries
					uint64_t outcnt = 0;
					for ( uint64_t i = 0; i < numel; ++i )
					{
						uint64_t const off = pp[i];
						char const * data = reinterpret_cast<char const *>(pa + off);
						
						if ( computemdnm )
						{
							// decode blocksize
							::libmaus::util::CountGetObject<char const *> G(data);
							uint64_t const blocksize = ::libmaus::bambam::DecoderBase::getLEInteger(G,4);
							
							// extend block if necessary
							if ( blocksize > recompalgn.D.size() )
								recompalgn.D = libmaus::bambam::BamAlignment::D_array_type(blocksize,false);
							
							// copy data
							recompalgn.blocksize = blocksize;
							std::copy(data + sizeof(uint32_t), data + sizeof(uint32_t) + blocksize, recompalgn.D.begin());

							// calculate md/nm
							if ( mdcalc->calmdnm(recompalgn) )
								recompalgn.fillMd(mdcalc->context);			
							
							// write block
							writer.writeAlgn(recompalgn);
						}
						else
						{
							writer.writeData(data);
						}
						
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
				std::vector< ::libmaus::lz::BgzfDeflateOutputCallback *> const * blockoutputcallbacks = 0,
				libmaus::bambam::MdNmRecalculation * mdnmcalc = 0
			)
			{
				::libmaus::bambam::BamWriter writer(stream,bamheader,level,blockoutputcallbacks);
				BamWriterWrapper< ::libmaus::bambam::BamWriter > BWW(writer);
				
				if ( mdnmcalc )
					createOutput< BamWriterWrapper< ::libmaus::bambam::BamWriter >, true >(BWW,mdnmcalc,verbose);
				else
					createOutput< BamWriterWrapper< ::libmaus::bambam::BamWriter >, false >(BWW,mdnmcalc,verbose);
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
				int const verbose = 0,
				libmaus::bambam::MdNmRecalculation * mdnmcalc = 0
			)
			{
				BamWriterWrapper< libmaus::bambam::BamBlockWriterBase > BWW(writerbase);
				if ( mdnmcalc )
					createOutput< BamWriterWrapper< libmaus::bambam::BamBlockWriterBase >, true  >(BWW,mdnmcalc,verbose);
				else
					createOutput< BamWriterWrapper< libmaus::bambam::BamBlockWriterBase >, false >(BWW,mdnmcalc,verbose);
			}
		};
	}
}
#endif
