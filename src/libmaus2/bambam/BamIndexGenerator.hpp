/*
    libmaus2
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
#if ! defined(LIBMAUS2_BAMBAM_BAMINDEXGENERATOR_HPP)
#define LIBMAUS2_BAMBAM_BAMINDEXGENERATOR_HPP

#include <libmaus2/aio/Buffer.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>
#include <libmaus2/aio/SingleFileFragmentMerge.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/bambam/EncoderBase.hpp>
#include <libmaus2/bambam/BamIndexLinearChunk.hpp>
#include <libmaus2/bambam/BamIndexBinChunk.hpp>
#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/bambam/BamHeader.hpp>
#include <libmaus2/bambam/BamIndexMetaInfo.hpp>
#include <libmaus2/util/GetObject.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamIndexGenerator
		{
			typedef BamIndexGenerator this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			private:
			template<typename stream_type>
			static std::pair<int64_t,uint64_t> countDistinctBins(stream_type & stream)
			{
				int64_t prevbin = -1;
				int64_t refid = -1;
				uint64_t bins = 0;
				::libmaus2::bambam::BamIndexBinChunk BC;
				
				while ( stream.peek() != stream_type::traits_type::eof() )
				{
					stream.read(reinterpret_cast<char *>(&BC),sizeof(::libmaus2::bambam::BamIndexBinChunk));
					
					if ( refid < 0 )
						refid = BC.refid;
					
					if ( refid != static_cast<int64_t>(BC.refid) )
					{
						stream.clear();
						stream.seekg(-static_cast<int64_t>(sizeof(::libmaus2::bambam::BamIndexBinChunk)),std::ios::cur);
						break;
					}
					
					if ( static_cast<int64_t>(BC.bin) != prevbin )
						bins++;
						
					prevbin = BC.bin;
				}
					
				return std::pair<uint64_t,uint64_t>(refid,bins);
			}

			template<typename stream_type>
			static std::pair<int64_t,uint64_t> countBinChunks(stream_type & stream)
			{
				int64_t bin = -1;
				int64_t refid = -1;
				uint64_t chunks = 0;

				::libmaus2::bambam::BamIndexBinChunk BC;
				
				while ( stream.peek() != stream_type::traits_type::eof() )
				{
					stream.read(reinterpret_cast<char *>(&BC),sizeof(::libmaus2::bambam::BamIndexBinChunk));

					if ( refid < 0 )
						refid = BC.refid;
					if ( bin < 0 )
						bin = BC.bin;
					
					if ( 
						refid != static_cast<int64_t>(BC.refid) 
						||
						bin != static_cast<int64_t>(BC.bin)
					)
					{
						stream.clear();
						stream.seekg(-static_cast<int64_t>(sizeof(::libmaus2::bambam::BamIndexBinChunk)),std::ios::cur);
						stream.clear();
						break;
					}
					
					chunks++;
				}
					
				return std::pair<uint64_t,uint64_t>(bin,chunks);
			}

			template<typename stream_type>
			static std::pair<int64_t,uint64_t> countLinearChunks(stream_type & stream)
			{
				int64_t refid = -1;
				uint64_t cnt = 0;
				
				::libmaus2::bambam::BamIndexLinearChunk LC;
				
				while ( stream.peek() != stream_type::traits_type::eof() )
				{
					stream.read(reinterpret_cast<char *>(&LC),sizeof(::libmaus2::bambam::BamIndexLinearChunk));
					
					if ( refid == -1 )
						refid = LC.refid;
					
					// put back element, if it has a different refid	
					if ( static_cast<int64_t>(LC.refid) != refid )
					{
						stream.clear();
						stream.seekg(-static_cast<int64_t>(sizeof(::libmaus2::bambam::BamIndexLinearChunk)),std::ios::cur);
						break;
					}
					
					cnt++;
				}
				
				return std::pair<int64_t,uint64_t>(refid,cnt);
			}

			template<typename stream_type>
			static std::pair<int64_t,uint64_t> getLinearMaxPos(stream_type & stream)
			{
				int64_t refid = -1;
				uint64_t maxpos = 0;
				
				::libmaus2::bambam::BamIndexLinearChunk LC;
				
				while ( stream.peek() != stream_type::traits_type::eof() )
				{
					stream.read(reinterpret_cast<char *>(&LC),sizeof(::libmaus2::bambam::BamIndexLinearChunk));
					
					if ( refid == -1 )
						refid = LC.refid;
					
					// put back element, if it has a different refid	
					if ( static_cast<int64_t>(LC.refid) != refid )
					{
						stream.clear();
						stream.seekg(-static_cast<int64_t>(sizeof(::libmaus2::bambam::BamIndexLinearChunk)),std::ios::cur);
						break;
					}
					
					maxpos = std::max(maxpos,LC.pos);
				}
				
				return std::pair<int64_t,uint64_t>(refid,maxpos);
			}

			template<typename stream_type>
			static std::pair<int64_t,int64_t> getLinearMaxChunk(stream_type & stream)
			{
				int64_t refid = -1;
				int64_t maxpos = 0;
				
				::libmaus2::bambam::BamIndexLinearChunk LC;
				
				while ( stream.peek() != stream_type::traits_type::eof() )
				{
					stream.read(reinterpret_cast<char *>(&LC),sizeof(::libmaus2::bambam::BamIndexLinearChunk));
					
					if ( refid == -1 )
						refid = LC.refid;
					
					// put back element, if it has a different refid	
					if ( static_cast<int64_t>(LC.refid) != refid )
					{
						stream.clear();
						stream.seekg(-static_cast<int64_t>(sizeof(::libmaus2::bambam::BamIndexLinearChunk)),std::ios::cur);
						break;
					}
					
					maxpos = std::max(maxpos,LC.chunkid);
				}
				
				return std::pair<int64_t,int64_t>(refid,maxpos);
			}

			template<typename stream_type>
			static bool peekLinearChunk(stream_type & stream, uint64_t const refid)
			{
				::libmaus2::bambam::BamIndexLinearChunk LC;

				if ( stream.peek() == stream_type::traits_type::eof() )
					return false;
				
				stream.read(reinterpret_cast<char *>(&LC),sizeof(::libmaus2::bambam::BamIndexLinearChunk));
				stream.clear();
				stream.seekg(-static_cast<int64_t>(sizeof(::libmaus2::bambam::BamIndexLinearChunk)),std::ios::cur);
				
				return LC.refid == refid;
			}

			template<typename stream_type>
			static bool peekLinearChunk(stream_type & stream, uint64_t const refid, int64_t const chunkid)
			{
				::libmaus2::bambam::BamIndexLinearChunk LC;

				if ( stream.peek() == stream_type::traits_type::eof() )
					return false;
				
				stream.read(reinterpret_cast<char *>(&LC),sizeof(::libmaus2::bambam::BamIndexLinearChunk));
				stream.clear();
				stream.seekg(-static_cast<int64_t>(sizeof(::libmaus2::bambam::BamIndexLinearChunk)),std::ios::cur);
				
				return LC.refid == refid && LC.chunkid == chunkid;
			}

			template<typename stream_type>
			static bool peekLinearChunk(stream_type & stream, uint64_t const refid, uint64_t const pos, unsigned int const posshift)
			{
				::libmaus2::bambam::BamIndexLinearChunk LC;

				if ( stream.peek() == stream_type::traits_type::eof() )
					return false;
				
				stream.read(reinterpret_cast<char *>(&LC),sizeof(::libmaus2::bambam::BamIndexLinearChunk));
				stream.clear();
				stream.seekg(-static_cast<int64_t>(sizeof(::libmaus2::bambam::BamIndexLinearChunk)),std::ios::cur);
				
				return (LC.refid == refid) && ((LC.pos >> posshift)==(pos>>posshift));
			}

			template<typename stream_type>
			static int64_t peekBin(stream_type & stream)
			{
				::libmaus2::bambam::BamIndexBinChunk BC;
				
				if ( stream.peek() == stream_type::traits_type::eof() )
					return -1;
					
				stream.read(
					reinterpret_cast<char *>(&BC),
					sizeof(::libmaus2::bambam::BamIndexBinChunk)
				);
				
				assert ( stream.gcount() == sizeof(::libmaus2::bambam::BamIndexBinChunk) );
				
				stream.clear();
				stream.seekg(-static_cast<int64_t>(sizeof(::libmaus2::bambam::BamIndexBinChunk)),std::ios::cur);
				
				return BC.refid;
			}

			static bool checkConsisteny(std::string const & binfn, std::string const & linfn, uint64_t const numrefseq)
			{
				::libmaus2::aio::InputStream::unique_ptr_type PbinCIS(::libmaus2::aio::InputStreamFactoryContainer::constructUnique(binfn));
				std::istream & binCIS = *PbinCIS;
				::libmaus2::aio::InputStream::unique_ptr_type PlinCIS(::libmaus2::aio::InputStreamFactoryContainer::constructUnique(linfn));
				std::istream & linCIS = *PlinCIS;
				
				while ( 
					binCIS.peek() != ::std::istream::traits_type::eof()
					&&
					linCIS.peek() != ::std::istream::traits_type::eof()
				)
				{
					std::pair<int64_t,uint64_t> const L = countLinearChunks(linCIS);
					std::pair<int64_t,uint64_t> const B = countDistinctBins(binCIS);
					
					if ( L.first != B.first )
					{
						std::cerr << "BamIndexGenerator::checkConsisteny(): inconsistent L.first=" << L.first << " != B.first=" << B.first << std::endl;
						return false;
					}
						
					if ( L.first >= static_cast<int64_t>(numrefseq) )
					{
						std::cerr << "BamIndexGenerator::checkConsisteny(): inconsistent L.first=" << L.first << " >= numrefseq=" << numrefseq << std::endl;
						return false;
					}
				}
				
				if ( binCIS.peek() != linCIS.peek() )
				{
					std::cerr << "BamIndexGenerator::checkConsisteny(): inconsistent binCIS.peek()=" << binCIS.peek() << " != linCIS.peek()=" << linCIS.peek() << std::endl;				
				}

				return binCIS.peek() == linCIS.peek();
			}


			/* parser state types */
			enum parsestate { state_reading_blocklen,  state_post_skip };
			
			::libmaus2::bambam::BamHeader header;
			
			bool haveheader;
			
			uint64_t cacct; // accumulated compressed bytes we have read from file
			std::pair<uint64_t,uint64_t> rinfo;

			::libmaus2::bambam::BamHeaderParserState bamheaderparsestate;

			parsestate state;

			unsigned int blocklenred;
			uint32_t blocklen;

			uint64_t alcnt;
			uint64_t alcmpstart;
			uint64_t alstart;
			
			uint64_t binalcmpstart;
			uint64_t binalstart;

			::libmaus2::bambam::BamAlignment algn;
			uint8_t * copyptr;
			
			int64_t prevrefid;
			int64_t prevpos;
			int64_t prevbin;
			int64_t prevcheckrefid;
			int64_t prevcheckpos;
			int64_t prevlinchunkid;
			
			uint64_t mappedcnt;
			uint64_t unmappedcnt;
			uint64_t refstart;
			
			std::string binchunktmpfilename;
			std::string linchunktmpfilename;
			std::string const metatmpfilename;

			::libmaus2::aio::OutputStream::unique_ptr_type chunkCOS;
			::libmaus2::aio::OutputStream::unique_ptr_type linCOS;
			::libmaus2::aio::OutputStream::unique_ptr_type metaOutFile;
			::libmaus2::aio::SynchronousGenericOutput< ::libmaus2::bambam::BamIndexMetaInfo >::unique_ptr_type metaOut;
			uint64_t metaOutWords;

			::libmaus2::aio::Buffer< ::libmaus2::bambam::BamIndexBinChunk> BCB;
			std::vector< std::pair<uint64_t,uint64_t> > binchunkfrags;
			::libmaus2::aio::Buffer< ::libmaus2::bambam::BamIndexLinearChunk> BLC;
			std::vector< std::pair<uint64_t,uint64_t> > linearchunkfrags;
			
			unsigned int const verbose;
			bool const validate;
			bool const debug;
			
			bool blocksetup;

			public:
			BamIndexGenerator(
				std::string const & tmpfileprefix, 
				unsigned int const rverbose = 0,
				bool const rvalidate = true, bool const rdebug = false
			)
			: header(), haveheader(false),
			  cacct(0), rinfo(), bamheaderparsestate(), state(state_reading_blocklen), blocklenred(0),
			  blocklen(0), alcnt(0), alcmpstart(0), alstart(0), binalcmpstart(0), binalstart(0), algn(),
			  copyptr(0), prevrefid(-1), prevpos(-1), prevbin(-1), prevcheckrefid(-1), prevcheckpos(-1),
			  prevlinchunkid(-1),
			  mappedcnt(0), unmappedcnt(0), refstart(0),
			  binchunktmpfilename(tmpfileprefix+".bin"), linchunktmpfilename(tmpfileprefix+".lin"),
			  metatmpfilename(tmpfileprefix+".meta"),
			  chunkCOS(), linCOS(), metaOut(), metaOutWords(0),
			  BCB(), binchunkfrags(), BLC(), linearchunkfrags(),
			  verbose(rverbose), validate(rvalidate), debug(rdebug),
			  blocksetup(false)
			{
				::libmaus2::util::TempFileRemovalContainer::addTempFile(binchunktmpfilename);
				::libmaus2::util::TempFileRemovalContainer::addTempFile(linchunktmpfilename);
				::libmaus2::util::TempFileRemovalContainer::addTempFile(metatmpfilename);

				::libmaus2::aio::OutputStream::unique_ptr_type tchunkCOS(::libmaus2::aio::OutputStreamFactoryContainer::constructUnique(binchunktmpfilename));
				chunkCOS = UNIQUE_PTR_MOVE(tchunkCOS);
				
				::libmaus2::aio::OutputStream::unique_ptr_type tlinCOS(::libmaus2::aio::OutputStreamFactoryContainer::constructUnique(linchunktmpfilename));
				linCOS = UNIQUE_PTR_MOVE(tlinCOS);
				
				::libmaus2::aio::OutputStream::unique_ptr_type tmetaOutFile(::libmaus2::aio::OutputStreamFactoryContainer::constructUnique(metatmpfilename));				
				metaOutFile = UNIQUE_PTR_MOVE(tmetaOutFile);

				::libmaus2::aio::SynchronousGenericOutput< ::libmaus2::bambam::BamIndexMetaInfo >::unique_ptr_type tmetaOut(
					new ::libmaus2::aio::SynchronousGenericOutput< ::libmaus2::bambam::BamIndexMetaInfo >(*metaOutFile,128)
				);
				metaOut = UNIQUE_PTR_MOVE(tmetaOut);
			}
			
			//! add block
			void addBlock(uint8_t const * Bbegin, uint64_t const compsize, uint64_t const uncompsize)
			{
				uint8_t const * pa = Bbegin; // buffer current pointer
				uint8_t const * pc = pa + uncompsize; // buffer end pointer
				
				rinfo.first = compsize;
				rinfo.second = uncompsize;

				cacct += rinfo.first; // accumulator for compressed data bytes read

				if ( (! haveheader) && (pa != pc) )
				{			
					::libmaus2::util::GetObject<uint8_t const *> G(Bbegin);
					std::pair<bool,uint64_t> const P = bamheaderparsestate.parseHeader(G,uncompsize);

					// header complete?
					if ( P.first )
					{
						header.init(bamheaderparsestate);
						haveheader = true;
						pa = Bbegin + P.second;
						pc = Bbegin + uncompsize;
						
						if ( pa != pc )
						{
							// start of compressed block
							alcmpstart = cacct - rinfo.first;
							// start of alignment in uncompressed block
							alstart = pa - Bbegin;
				
							// start of bin in compressed data
							binalcmpstart = alcmpstart;
							// start of bin in uncompressed block
							binalstart = alstart;
					
							blocksetup = true;	
						}
					}
				}
				
				if ( (haveheader) && (pa != pc) )
				{
					if ( ! blocksetup )
					{
						// start of compressed block
						alcmpstart = cacct - rinfo.first;
						// start of alignment in uncompressed block
						alstart = pa - Bbegin;
				
						// start of bin in compressed data
						binalcmpstart = alcmpstart;
						// start of bin in uncompressed block
						binalstart = alstart;
					
						blocksetup = true;
					}

					while ( pa != pc )
					{
						switch ( state )
						{
							/* read length of next alignment block */
							case state_reading_blocklen:
								/* if this is a little endian machine allowing unaligned access */
								#if defined(LIBMAUS2_HAVE_i386)
								if ( (!blocklenred) && ((pc-pa) >= static_cast<ptrdiff_t>(sizeof(uint32_t))) )
								{
									blocklen = *(reinterpret_cast<uint32_t const *>(pa));
									blocklenred = sizeof(uint32_t);
									pa += sizeof(uint32_t);
									
									state = state_post_skip;
									
									if ( algn.D.size() < blocklen )
										algn.D = ::libmaus2::bambam::BamAlignment::D_array_type(blocklen,false);
									algn.blocksize = blocklen;
									copyptr = algn.D.begin();
								}
								else
								#endif
								{
									while ( pa != pc && blocklenred < sizeof(uint32_t) )
										blocklen |= static_cast<uint32_t>(*(pa++)) << ((blocklenred++)*8);

									if ( blocklenred == sizeof(uint32_t) )
									{
										state = state_post_skip;

										if ( algn.D.size() < blocklen )
											algn.D = ::libmaus2::bambam::BamAlignment::D_array_type(blocklen,false);
										algn.blocksize = blocklen;
										copyptr = algn.D.begin();
									}
								}
								
								break;
							/* skip data after part we modify */
							case state_post_skip:
							{
								uint32_t const skip = std::min(pc-pa,static_cast<ptrdiff_t>(blocklen));
								std::copy(pa,pa+skip,copyptr);
								copyptr += skip;
								pa += skip;
								blocklen -= skip;
								
								if ( ! blocklen )
								{
									if ( validate )
										algn.checkAlignment();
								
									// information for this new alignment
									int64_t const thisrefid = algn.getRefID();
									int64_t const thispos = algn.getPos();
									int64_t const thisbin = 
										(thisrefid >= 0 && thispos >= 0) ? algn.computeBin() : -1;

									if ( thisrefid != prevrefid )
									{
										if ( mappedcnt + unmappedcnt )
										{
											if ( verbose )
												std::cerr << "[V] " << header.getRefIDName(prevrefid) << " " << mappedcnt << " " << unmappedcnt << std::endl;
											
											metaOut->put ( ::libmaus2::bambam::BamIndexMetaInfo(prevrefid,refstart,(alcmpstart << 16) | alstart,mappedcnt,unmappedcnt) );
											metaOutWords += 1;
										}
									
										mappedcnt = 0;
										unmappedcnt = 0;
										refstart = (alcmpstart << 16) | alstart;
									}
									
									if ( algn.isUnmap() )
										unmappedcnt++;
									else
										mappedcnt++;

									// map negative to maximum positive for checking order
									int64_t const thischeckrefid = (thisrefid >= 0) ? thisrefid : std::numeric_limits<int64_t>::max();
									int64_t const thischeckpos   = (thispos >= 0) ? thispos : std::numeric_limits<int64_t>::max();
									
									// true iff order is ok
									bool const orderok =
										(thischeckrefid > prevcheckrefid)
										||
										(thischeckrefid == prevcheckrefid && thischeckpos >= prevcheckpos);

									if ( thisrefid != prevrefid )
										prevlinchunkid = -1;
									
									// throw exception if alignment stream is not sorted by coordinate
									if ( ! orderok )
									{
										::libmaus2::exception::LibMausException se;
										se.getStream() << "File is not sorted by coordinate." << std::endl;
										se.finish();
										throw se;
									}
									
									// linear chunk of last mapped position (or position if unmapped)
									int64_t const thislinchunkid =
										(algn.isMapped() ?((algn.getPos() + (int64_t)algn.getReferenceLength())-1) : algn.getPos()) >> 14;

									// check if this alignment is in a different linear chunk than the previous one
									if ( 
										algn.isMapped() 
										&&
										(
											thisrefid != prevrefid 
											||
											thislinchunkid > prevlinchunkid
										)
									)
									{
										// note linear chunk start if position is valid
										if ( thisrefid >= 0 && thispos >= 0 && thislinchunkid >= 0 )
										{
											int64_t const linstart = (thispos >> 14);
											int64_t const linend = thislinchunkid;
											
											for ( int64_t lini = linstart; lini <= linend; ++lini )
												if ( lini > prevlinchunkid )
												{
													::libmaus2::bambam::BamIndexLinearChunk LC(
														thisrefid,thispos,alcmpstart,alstart,lini
													);
													
													if ( BLC.put(LC) )
														linearchunkfrags.push_back(BLC.flush(*linCOS));
													
													prevlinchunkid = lini;
												}
										}

										if ( debug )
										{
											std::cerr << "new lin: " << thisrefid << "," << thispos 
												<< " alcmpstart=" << alcmpstart
												<< " alstart=" << alstart
												<< std::endl;
										}
									}	

									// check if this alignment is in a different bin than the previous one
									if ( 
										thisbin != prevbin 
										||
										thisrefid != prevrefid
									)
									{
										// write previous bin
										if ( prevbin >= 0 )
										{
											uint64_t binalcmpend = alcmpstart;
											uint64_t binalend = alstart;
										
											if ( debug )
											{
												std::cerr << "end of bin " << prevbin
													<< " on ref id " << prevrefid
													<< " binalcmpstart=" << binalcmpstart
													<< " binalstart=" << binalstart
													<< " binalcmpend=" << binalcmpend
													<< " binalend=" << binalend
													<< std::endl;
											}

											::libmaus2::bambam::BamIndexBinChunk BC(prevrefid,prevbin,binalcmpstart,binalstart,binalcmpend,binalend);
											
											if ( debug )
											{
												std::cerr << BC << std::endl;
											}
											
											if ( BCB.put(BC) )
												binchunkfrags.push_back(BCB.flush(*chunkCOS));
										}

										binalcmpstart = alcmpstart;
										binalstart = alstart;
									}
									
									if ( debug )
									{
										std::cerr 
											<< "alcnt=" << alcnt
											<< " alcmpstart=" << alcmpstart
											<< " alstart=" << alstart
											<< " refid=" << thisrefid
											<< " pos=" << thispos
											<< " bin=" << thisbin
											<< std::endl;
									}
								
									// finished an alignment, set up for next one
									state = state_reading_blocklen;
									
									blocklenred = 0;
									blocklen = 0;

									// start of next alignment:
									//   - if pc != pa, then next alignment starts in this block
									//   - if pc == pa, then next alignment starts in next block
									// block start
									uint64_t const nextalcmpstart = 
										(pc != pa) ? (cacct - rinfo.first) : cacct;
									// offset in block
									uint64_t const nextalstart = 
										(pc != pa) ? (pa - Bbegin) : 0;

									alcnt++;
									alcmpstart = nextalcmpstart;
									alstart = nextalstart;

									prevrefid = thisrefid;
									prevpos = thispos;
									prevbin = thisbin;

									prevcheckrefid = thischeckrefid;
									prevcheckpos = thischeckpos;
				       
									if ( verbose && (alcnt % (1024*1024) == 0) )
										std::cerr << "[V] " << alcnt/(1024*1024) << std::endl;
								}
								break;
							}
						}
					}
				}
			}
			
			template<typename output_stream_type>
			void flush(output_stream_type & out)
			{
				if ( prevbin >= 0 )
				{
					uint64_t binalcmpend = alcmpstart;
					uint64_t binalend = alstart;
				
					if ( debug )
					{
						std::cerr << "*end of bin " << prevbin
							<< " on ref id " << prevrefid
							<< " binalcmpstart=" << binalcmpstart
							<< " binalstart=" << binalstart
							<< " binalcmpend=" << binalcmpend
							<< " binalend=" << binalend
							<< std::endl;
					}

					::libmaus2::bambam::BamIndexBinChunk BC(prevrefid,prevbin,binalcmpstart,binalstart,binalcmpend,binalend);

					if ( BCB.put(BC) )
						binchunkfrags.push_back(BCB.flush(*chunkCOS));
				}

				if ( ! BCB.empty() )
					binchunkfrags.push_back(BCB.flush(*chunkCOS));
				if ( !BLC.empty() )
					linearchunkfrags.push_back(BLC.flush(*linCOS));

				if ( mappedcnt + unmappedcnt )
				{
					if ( verbose )
						std::cerr << "[V] " << header.getRefIDName(prevrefid) << " " << mappedcnt << " " << unmappedcnt << std::endl;

					metaOut->put ( ::libmaus2::bambam::BamIndexMetaInfo(prevrefid,refstart,(alcmpstart << 16) | alstart,mappedcnt,unmappedcnt) );
					metaOutWords += 1;
				}

				chunkCOS->flush();
				// chunkCOS->close();
				chunkCOS.reset();
				
				linCOS->flush();
				// linCOS->close();
				linCOS.reset();

				metaOut->flush();
				metaOut.reset();
				metaOutFile->flush();
				metaOutFile.reset();
				
				if ( verbose )
					std::cerr << "[V] " << alcnt << std::endl;
					
				std::string const binchunktmpfilename_unmerged = binchunktmpfilename;
				std::string const linchunktmpfilename_unmerged = linchunktmpfilename;
					
				binchunktmpfilename = ::libmaus2::aio::SingleFileFragmentMerge< ::libmaus2::bambam::BamIndexBinChunk>::merge(binchunktmpfilename,binchunkfrags);
				linchunktmpfilename = ::libmaus2::aio::SingleFileFragmentMerge< ::libmaus2::bambam::BamIndexLinearChunk>::merge(linchunktmpfilename,linearchunkfrags);
				
				libmaus2::aio::FileRemoval::removeFile(binchunktmpfilename_unmerged);
				libmaus2::aio::FileRemoval::removeFile(linchunktmpfilename_unmerged);

				/* check consistency */
				bool const consistent = checkConsisteny(binchunktmpfilename,linchunktmpfilename,header.getNumRef());
				
				if ( ! consistent )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "bin index and linear index are inconsistent." << std::endl;
					se.finish();
					throw se;
				}

				/* write index */
				::libmaus2::aio::InputStream::unique_ptr_type PbinCIS(::libmaus2::aio::InputStreamFactoryContainer::constructUnique(binchunktmpfilename));
				::libmaus2::aio::InputStream & binCIS = *PbinCIS;
				::libmaus2::aio::InputStream::unique_ptr_type PlinCIS(::libmaus2::aio::InputStreamFactoryContainer::constructUnique(linchunktmpfilename));
				::libmaus2::aio::InputStream & linCIS = *PlinCIS;

				::libmaus2::aio::InputStream::unique_ptr_type PmetaIn(::libmaus2::aio::InputStreamFactoryContainer::constructUnique(metatmpfilename));
				::libmaus2::aio::SynchronousGenericInput< ::libmaus2::bambam::BamIndexMetaInfo > metaIn(*PmetaIn,128,metaOutWords);

				out.put('B');
				out.put('A');
				out.put('I');
				out.put('\1');
				::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint32_t>(out,header.getNumRef());
				
				for ( uint64_t i = 0; i < header.getNumRef(); ++i )
				{
					if ( debug )
						std::cerr << "chromosome " << header.getRefIDName(i) << std::endl;
				
					if ( peekBin(binCIS) == static_cast<int64_t>(i) )
					{
						::libmaus2::bambam::BamIndexMetaInfo meta;

						if ( metaIn.peekNext(meta) && meta.refid == static_cast<int64_t>(i) )
							metaIn.getNext(meta);
							
						bool const havemeta = (meta.refid == static_cast<int64_t>(i));
					
						uint64_t const bprepos = binCIS.tellg();
						if ( debug )
							std::cerr << "bprepos=" << bprepos << std::endl;
						std::pair<int64_t,uint64_t> const P = countDistinctBins(binCIS);
						assert ( P.first == static_cast<int64_t>(i) );
						binCIS.clear();
						binCIS.seekg(bprepos,std::ios::beg);
						
						if ( debug )
							std::cerr << "Distinct bins " << P.second << std::endl;
						
						// number of distinct bins
						::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint32_t>(out,P.second + (havemeta?1:0));
						
						// iterate over bins
						for ( uint64_t b = 0; b < P.second; ++b )
						{
							uint64_t const lprepos = binCIS.tellg();
							std::pair<int64_t,uint64_t> Q = countBinChunks(binCIS);

							binCIS.seekg(lprepos);
							binCIS.clear();

							if ( debug )
								std::cerr << "ref " << P.first << " dist bins " << P.second 
									<< " bin " << Q.first << " chunks " << Q.second
									<< std::endl;

							// bin
							::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint32_t>(out,Q.first);	
							// number of chunks
							::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint32_t>(out,Q.second);	
				
							// iterate over chunks
							for ( uint64_t c = 0; c < Q.second; ++c )
							{
								::libmaus2::bambam::BamIndexBinChunk BC;
								binCIS.read(reinterpret_cast<char *>(&BC),sizeof(::libmaus2::bambam::BamIndexBinChunk));
								
								// chunk data
								::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint64_t>(out,(BC.alcmpstart<<16)|(BC.alstart));
								::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint64_t>(out,(BC.alcmpend<<16)|(BC.alend));	
								
								if ( debug )
								{
									std::cerr << BC << std::endl;
								}
							}
						}
						
						if ( havemeta )
						{
							// bin id
							::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint32_t>(out,37450 /* meta bin id */);	
							// number of chunks
							::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint32_t>(out,2);	
							// start of ref id
							::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint64_t>(out,meta.start);	
							// end of ref id
							::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint64_t>(out,meta.end);	
							// number of mapped reads
							::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint64_t>(out,meta.mapped);	
							// number of unmapped reads
							::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint64_t>(out,meta.unmapped);	
						}

						uint64_t const lprepos = linCIS.tellg();
						std::pair<int64_t,int64_t> const Q = getLinearMaxChunk(linCIS);
						assert ( Q.first == static_cast<int64_t>(i) );
						linCIS.seekg(lprepos,std::ios::beg);
						linCIS.clear();
						
						uint64_t const numchunks = Q.second+1; // maximum chunk id + 1

						// number of linear chunks bins
						::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint32_t>(out,numchunks);
						// previous offset value
						uint64_t prevoff = 0;
						
						for ( uint64_t c = 0; c < numchunks; ++c )
						{
							if ( peekLinearChunk(linCIS,P.first,c /*c*posperchunk,14*/) )
							{
								::libmaus2::bambam::BamIndexLinearChunk LC;
								linCIS.read(reinterpret_cast<char *>(&LC),sizeof(::libmaus2::bambam::BamIndexLinearChunk));
								prevoff = LC.getOffset();
								::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint64_t>(out,prevoff);	

								if ( debug )
									std::cerr << "LC[" << c << "]=" << LC << " -> " << prevoff << std::endl;
							}
							else
							{
								::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint64_t>(out,prevoff);
								if ( debug )
									std::cerr << "LC[" << c << "]=" << "null -> " << prevoff << std::endl;
							}
						}
					}
					else
					{
						// number of distinct bins
						::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint32_t>(out,0);
						// number of linear index chunks
						::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint32_t>(out,0);
					}
				}

				::libmaus2::bambam::BamIndexMetaInfo meta;

				if ( metaIn.peekNext(meta) && (meta.refid == -1) )
				{
					metaIn.getNext(meta);
					::libmaus2::bambam::EncoderBase::putLE<std::ostream,uint64_t>(out,meta.unmapped);	
				}

				out.flush();	
			}

			void flush(std::string const & filename)
			{
				::libmaus2::aio::OutputStream::unique_ptr_type pCOS(::libmaus2::aio::OutputStreamFactoryContainer::constructUnique(filename));
				flush(*pCOS);
			}
		};
	}
}
#endif
