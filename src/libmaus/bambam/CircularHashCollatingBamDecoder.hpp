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
#if ! defined(LIBMAUS_BAMBAM_CIRCULARHASHCOLLATINGBAMDECODER_HPP)
#define LIBMAUS_BAMBAM_CIRCULARHASHCOLLATINGBAMDECODER_HPP

#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/bambam/BamRangeDecoder.hpp>
#include <libmaus/bambam/ScramDecoder.hpp>
#include <libmaus/bambam/BamMergeCoordinate.hpp>
#include <libmaus/bambam/BamMergeQueryName.hpp>
#include <libmaus/bambam/BamAlignmentSortingCircularHashEntryOverflow.hpp>
#include <libmaus/bambam/CollatingBamDecoderAlignmentInputCallback.hpp>
#include <libmaus/hashing/CircularHash.hpp>

namespace libmaus
{
	namespace bambam
	{
		/**
		 * BAM name collation base class based on ring buffer hash
		 **/
		struct CircularHashCollatingBamDecoder
		{
			//! this type
			typedef CircularHashCollatingBamDecoder this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			//! alignment pointer pair type
			typedef libmaus::bambam::BamAlignment const * alignment_ptr_type;

			/**
			 * process result type
			 **/
			struct OutputBufferEntry
			{
				//! first alignment block pointer
				uint8_t const * Da;
				//! first alignment block size
				uint64_t blocksizea;
				//! second alignment block pointer
				uint8_t const * Db;
				//! second alignment block size
				uint64_t blocksizeb;
				
				//! single end (Da valid, Db invalid)
				bool fsingle;
				//! pair (Da valid, Db valid)
				bool fpair;
				//! orphan 1 (Da valid, Db invalid)
				bool forphan1;
				//! orphan 2 (Da invalid, Db valid)
				bool forphan2;
				
				/**
				 * constructor
				 **/
				OutputBufferEntry()
				: Da(0), blocksizea(0), Db(0), blocksizeb(0), fsingle(false), fpair(false),
				  forphan1(false), forphan2(false)
				{}
			};

			private:
			//! hash table overflow type
			typedef libmaus::bambam::BamAlignmentSortingCircularHashEntryOverflow overflow_type;
			//! hash table overflow pointer type
			typedef overflow_type::unique_ptr_type overflow_ptr_type;
			//! hash table type
			typedef libmaus::hashing::CircularHash<overflow_type> cht;
			//! hash table pointer type
			typedef cht::unique_ptr_type cht_ptr;

			//! collator state enum
			enum circ_hash_collator_state {
				state_sortbuffer_flushing_intermediate,
				state_sortbuffer_flushing_intermediate_readout,
				state_sortbuffer_flushing_final,
				state_sortbuffer_flushing_final_readout,
				state_reading,
				state_setup_merging,
				state_merging,
				state_done,
				state_failed
			};

			/**
			 * return string representation of collar state
			 *
			 * @param state collator state
			 * @return string representation of collar state
			 **/
			static std::string stateToString(circ_hash_collator_state state)
			{
				std::ostringstream out;
				
				switch ( state )
				{	
					case state_sortbuffer_flushing_intermediate:
						out << "state_sortbuffer_flushing_intermediate";
						break;
					case state_sortbuffer_flushing_intermediate_readout:
						out << "state_sortbuffer_flushing_intermediate_readout";
						break;
					case state_sortbuffer_flushing_final:
						out << "state_sortbuffer_flushing_final";
						break;
					case state_sortbuffer_flushing_final_readout:
						out << "state_sortbuffer_flushing_final_readout";
						break;
					case state_reading:
						out << "state_reading";
						break;
					case state_setup_merging:
						out << "state_setup_merging";
						break;
					case state_merging:
						out << "state_merging";
						break;
					case state_done:
						out << "state_done";
						break;
					case state_failed:
						out << "state_failed";
						break;
				}
				
				return out.str();
			}

			protected:
			/**
			 * check whether alignment at hash value hv in hash CH and algn form a pair
			 *
			 * @param CH hash table
			 * @param algn alignment outside CH
			 * @param hv hash value
			 * @return true iff CH[h] and algn form a pair
			 **/
			bool isPair(cht const & CH, libmaus::bambam::BamAlignment const & algn, uint32_t const hv)
			{
				std::pair<cht::pos_type,cht::entry_size_type> const hentry = CH.getEntry(hv);

				// decode length of name in hash entry
				unsigned int const hrnl = ::libmaus::bambam::BamAlignmentDecoderBase::getLReadNameWrapped(
					CH.B.begin(),
					CH.B.size(),
					hentry.first
				);
				unsigned int const algnrnl = algn.getLReadName();
				
				// name length mismatch?
				if ( algnrnl != hrnl )
					return false;
				
				uint64_t rno = ::libmaus::bambam::BamAlignmentDecoderBase::getReadNameOffset(CH.B.size(),hentry.first);
				
				bool samename;
				
				// can we compare the names without wrap around?
				if ( rno + hrnl <= CH.B.size() )
				{
					samename = (memcmp(CH.B.begin()+rno,::libmaus::bambam::BamAlignmentDecoderBase::getReadName(algn.D.begin()),hrnl) == 0);
				}
				else
				{
					uint8_t const * algnrn = reinterpret_cast<uint8_t const *>(algn.getName());
					samename = true;	
					
					for ( unsigned int i = 0; i < hrnl; ++i, rno = (rno+1)&(CH.bmask) )
						if ( CH.B[rno] != algnrn[i] )
							samename = false;	
				}
				
				if ( ! samename )
					return false;

				// names are the same, now check whether one is marked as read1 and the other as read2
				uint32_t const flagsa = algn.getFlags();
				uint32_t const flagsb = libmaus::bambam::BamAlignmentDecoderBase::getFlagsWrapped(CH.B.begin(),CH.B.size(),hentry.first);
				
				return
					(libmaus::bambam::BamAlignmentDecoderBase::isRead1(flagsa)
					&&
					libmaus::bambam::BamAlignmentDecoderBase::isRead2(flagsb))
					||
					(libmaus::bambam::BamAlignmentDecoderBase::isRead2(flagsa)
					&&
					libmaus::bambam::BamAlignmentDecoderBase::isRead1(flagsb));
			}

			private:
			//! bam decoder pointer
			libmaus::bambam::BamDecoder::unique_ptr_type Pbamdec;
			//! bam decoder reference
			libmaus::bambam::BamAlignmentDecoder & bamdec;
			//! current bam decoder alignment
			libmaus::bambam::BamAlignment const & algn;
			//! alignment pair used during merging
			libmaus::bambam::BamAlignment mergealgn[2];
			//! alignment pointer into mergelalgn (0 or 1)
			unsigned int mergealgnptr;
			//! name of temporary file
			std::string const tmpfilename;

			//! overflow structure pointer
			overflow_ptr_type NCHEO;
			//! hash table pointer			
			cht_ptr CH;
			//! temporary memory
			libmaus::autoarray::AutoArray<uint8_t> T;
			//! object for reading back alignments
			libmaus::bambam::SnappyAlignmentMergeInput::unique_ptr_type mergeinput;
			//! state of collator
			circ_hash_collator_state state;
			//! BAM alignment input callback for passing alignments to rewriting
			CollatingBamDecoderAlignmentInputCallback * inputcallback;
			//! current output buffer
			OutputBufferEntry outputBuffer;
			//! output alignment pair for tryPair
			libmaus::bambam::BamAlignment outputAlgn[2];
			//! exclude alignments matching any of these flags from processing
			uint32_t const excludeflags;
			//! put back flag
			bool cbputbackflag;
			
			public:
			
			/**
			 * constructor from input stream
			 *
			 * @param in input stream
			 * @param rputrank put a rank (line number in file) on each alignment
			 * @param rtmpfilename name of temporary file for overflow
			 * @param rexcludeflags for excluding alignments from processing based on flags
			 * @param hlog log_2 of collation hash table size
			 * @param sortbufsize sort buffer size
			 **/
			CircularHashCollatingBamDecoder(
				std::istream & in,
				bool const rputrank,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			)
			: Pbamdec(new libmaus::bambam::BamDecoder(in,rputrank)), bamdec(*Pbamdec), algn(bamdec.getAlignment()), mergealgnptr(0), tmpfilename(rtmpfilename), 
			  NCHEO(new overflow_type(tmpfilename,sortbufsize)), CH(new cht(*NCHEO,hlog)), state(state_reading), inputcallback(0),
			  excludeflags(rexcludeflags), cbputbackflag(false)
			{
			
			}

			/**
			 * constructor from input stream
			 *
			 * @param rbamdec bam decoder object
			 * @param rtmpfilename name of temporary file for overflow
			 * @param rexcludeflags for excluding alignments from processing based on flags
			 * @param hlog log_2 of collation hash table size
			 * @param sortbufsize sort buffer size
			 **/
			CircularHashCollatingBamDecoder(
				libmaus::bambam::BamAlignmentDecoder & rbamdec,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			)
			: Pbamdec(), bamdec(rbamdec), algn(bamdec.getAlignment()), mergealgnptr(0), tmpfilename(rtmpfilename), 
			  NCHEO(new overflow_type(tmpfilename,sortbufsize)), CH(new cht(*NCHEO,hlog)), state(state_reading), inputcallback(0),
			  excludeflags(rexcludeflags), cbputbackflag(false)
			{
			
			}
			
			/**
			 * destructor
			 **/
			virtual ~CircularHashCollatingBamDecoder() {}
			
			/**
			 * process input and try to find another pair
			 *
			 * @return OutputBufferEntry pointer or null pointer if no more data was available
			 **/
			OutputBufferEntry * process()
			{
				/** reset output buffer */
				if ( outputBuffer.fsingle )
				{
					outputBuffer.fsingle = false;
					outputBuffer.Da = 0;
					outputBuffer.blocksizea = 0;
				}
				else if ( outputBuffer.fpair )
				{
					outputBuffer.fpair = false;
					outputBuffer.Da = 0;
					outputBuffer.blocksizea = 0;
					outputBuffer.Db = 0;
					outputBuffer.blocksizeb = 0;
				}
				else if ( outputBuffer.forphan1 )
				{
					outputBuffer.forphan1 = false;
					outputBuffer.Da = outputBuffer.Db;
					outputBuffer.blocksizea = outputBuffer.blocksizeb;
					outputBuffer.Db = 0;
					outputBuffer.blocksizeb = 0;
				}
				else if ( outputBuffer.forphan2 )
				{
					outputBuffer.forphan2 = false;
					outputBuffer.Da = outputBuffer.Db;
					outputBuffer.blocksizea = outputBuffer.blocksizeb;
					outputBuffer.Db = 0;
					outputBuffer.blocksizeb = 0;
				}
			
				/* run fsm until we have obtained a result or reached the end of the stream */
				while ( 
					state != state_done && state != state_failed
					&&
					(!outputBuffer.fsingle) && (!outputBuffer.fpair) && (!outputBuffer.forphan1) && (!outputBuffer.forphan2)
				)
				{
					if ( state == state_sortbuffer_flushing_intermediate )
					{
						NCHEO->flush();								
						state = state_sortbuffer_flushing_intermediate_readout;
					}
					if ( state == state_sortbuffer_flushing_intermediate_readout )
					{
						uint8_t const * ptr = 0;
						uint64_t length = 0;
						
						// this call produces pairs in steps
						if ( NCHEO->getFlushBufEntry(ptr,length) )
						{
							// read first
							if ( ! outputBuffer.Da )
							{
								outputBuffer.Da = ptr;
								outputBuffer.blocksizea = length;
							}
							// read second an set flag
							else
							{
								outputBuffer.Db = ptr;
								outputBuffer.blocksizeb = length;
								outputBuffer.fpair = true;
							}
						}
						else
							state = state_reading;
					}
					else if ( state == state_sortbuffer_flushing_final )
					{
						NCHEO->flush();
						state = state_sortbuffer_flushing_final_readout;
					}
					else if ( state == state_sortbuffer_flushing_final_readout )
					{
						uint8_t const * ptr = 0;
						uint64_t length = 0;
						
						// as above, this produces pairs in steps
						if ( NCHEO->getFlushBufEntry(ptr,length) )
						{
							// read first
							if ( ! outputBuffer.Da )
							{
								outputBuffer.Da = ptr;
								outputBuffer.blocksizea = length;
							}
							// read second an set flag
							else
							{
								outputBuffer.Db = ptr;
								outputBuffer.blocksizeb = length;
								outputBuffer.fpair = true;
							}
						}
						else
							state = state_setup_merging;
					}
					else if ( state == state_setup_merging )
					{
						CH.reset();
						libmaus::bambam::SnappyAlignmentMergeInput::unique_ptr_type tmergeinput(NCHEO->constructMergeInput());
						mergeinput = UNIQUE_PTR_MOVE(tmergeinput);
						NCHEO.reset();
						state = state_merging;			
					}
					else if ( state == state_reading )
					{
						if ( bamdec.readAlignment(true /* delay put rank */) )
						{
							if ( inputcallback )
							{
								if ( cbputbackflag )
									cbputbackflag = false;
								else
									(*inputcallback)(algn);
							}

							bamdec.putRank();
															
							if ( algn.getFlags() & excludeflags )
								continue;
						
							uint8_t const * data = algn.D.begin();
							uint64_t const datalen = algn.blocksize;
							
							// output single end immediately
							if ( ! algn.isPaired() )
							{
								outputBuffer.Da = data;
								outputBuffer.blocksizea = datalen;
								outputBuffer.fsingle = true;
							}
							else
							{
								// compute hash value for new alignment
								uint32_t const hv = algn.hash32();

								// see if we found a pair
								if ( CH->hasEntry(hv) && isPair(*CH,algn,hv) )
								{
									std::pair<cht::pos_type,cht::entry_size_type> const hentry = CH->getEntry(hv);
									
									if ( algn.isRead1() )
									{
										outputBuffer.Da = data;
										outputBuffer.blocksizea = datalen;
										outputBuffer.Db = CH->getEntryData(hentry,T);
										outputBuffer.blocksizeb = hentry.second;
										outputBuffer.fpair = true;
									}
									else
									{
										outputBuffer.Da = CH->getEntryData(hentry,T);
										outputBuffer.blocksizea = hentry.second;
										outputBuffer.Db = data;
										outputBuffer.blocksizeb = datalen;
										outputBuffer.fpair = true;
									}
									
									CH->eraseEntry(hv);
								}
								// not a pair, insert alignment into hash
								else
								{
									if ( ! CH->putEntry(data,datalen,hv) )
									{
										bamdec.putback();
										if ( inputcallback )
											cbputbackflag = true;
										state = state_sortbuffer_flushing_intermediate;
									}
								}
							}
						}
						else
						{
							if ( !CH->flush() )
								state = state_sortbuffer_flushing_intermediate;
							else
								state = state_sortbuffer_flushing_final;
						}
					}
					else if ( state == state_merging )
					{
						libmaus::bambam::BamAlignment & malgn = mergealgn[mergealgnptr];
						mergealgnptr = (mergealgnptr + 1) & 1;
					
						if ( mergeinput->readAlignment(malgn) )
						{
							if ( ! outputBuffer.Da )
							{
								outputBuffer.Da = malgn.D.begin();
								outputBuffer.blocksizea = malgn.blocksize;
							}
							else
							{
								outputBuffer.Db = malgn.D.begin();
								outputBuffer.blocksizeb = malgn.blocksize;
								
								if ( 
									libmaus::bambam::BamAlignmentDecoderBase::getLReadName(outputBuffer.Da)
									==
									libmaus::bambam::BamAlignmentDecoderBase::getLReadName(outputBuffer.Db)
									&&
									memcmp(
										libmaus::bambam::BamAlignmentDecoderBase::getReadName(outputBuffer.Da),
										libmaus::bambam::BamAlignmentDecoderBase::getReadName(outputBuffer.Db),
										libmaus::bambam::BamAlignmentDecoderBase::getLReadName(outputBuffer.Da)
									)
									== 0
									&&
									libmaus::bambam::BamAlignmentDecoderBase::isRead1(
										libmaus::bambam::BamAlignmentDecoderBase::getFlags(outputBuffer.Da)
									)
									&&							
									libmaus::bambam::BamAlignmentDecoderBase::isRead2(
										libmaus::bambam::BamAlignmentDecoderBase::getFlags(outputBuffer.Db)
									)
								)
								{
									outputBuffer.fpair = true;
								}
								else
								{
									if (
										libmaus::bambam::BamAlignmentDecoderBase::isRead1(
											libmaus::bambam::BamAlignmentDecoderBase::getFlags(outputBuffer.Da)
										)
									)
										outputBuffer.forphan1 = true;
									else
										outputBuffer.forphan2 = true;
								}
							}
						}	
						else
						{
							if ( outputBuffer.Da && !(outputBuffer.Db) )
							{
								if (
									libmaus::bambam::BamAlignmentDecoderBase::isRead1(
										libmaus::bambam::BamAlignmentDecoderBase::getFlags(outputBuffer.Da)
									)
								)
									outputBuffer.forphan1 = true;
								else
									outputBuffer.forphan2 = true;				
							}
							
							state = state_done;				
						}
					}

				}
				
				if (
					outputBuffer.fsingle ||
					outputBuffer.fpair ||
					outputBuffer.forphan1 ||
					outputBuffer.forphan2
				)
					return &outputBuffer;
				else
					return 0;
			}

			/**
			 * try to find next pair
			 *
			 * @param P reference to pair of alignment pointers used to store pair information
			 * @return true if P was assigned at least one alignment
			 **/
			bool tryPair(std::pair <libmaus::bambam::BamAlignment const *, libmaus::bambam::BamAlignment const *> & P)
			{
				OutputBufferEntry const * ob = process();
				
				if ( ! ob )
				{
					return false;
				}
				else if ( ob->fpair )
				{
					if ( outputAlgn[0].D.size() < ob->blocksizea )
						outputAlgn[0].D = libmaus::bambam::BamAlignment::D_array_type(ob->blocksizea);
					std::copy(ob->Da,ob->Da + ob->blocksizea, outputAlgn[0].D.begin());
					outputAlgn[0].blocksize = ob->blocksizea;

					if ( outputAlgn[1].D.size() < ob->blocksizeb )
						outputAlgn[1].D = libmaus::bambam::BamAlignment::D_array_type(ob->blocksizeb);
					std::copy(ob->Db,ob->Db + ob->blocksizeb, outputAlgn[1].D.begin());
					outputAlgn[1].blocksize = ob->blocksizeb;

					P = std::pair <libmaus::bambam::BamAlignment const *, libmaus::bambam::BamAlignment const * >(
						&(outputAlgn[0]),&(outputAlgn[1])
					);
					
					return true;
				}
				else if ( ob->fsingle )
				{
					if ( outputAlgn[0].D.size() < ob->blocksizea )
						outputAlgn[0].D = libmaus::bambam::BamAlignment::D_array_type(ob->blocksizea);
					std::copy(ob->Da,ob->Da + ob->blocksizea, outputAlgn[0].D.begin());
					outputAlgn[0].blocksize = ob->blocksizea;

					P = std::pair <libmaus::bambam::BamAlignment const *, libmaus::bambam::BamAlignment const * >(
						&(outputAlgn[0]),static_cast<libmaus::bambam::BamAlignment const *>(0)
					);
					
					return true;
				}
				else if ( ob->forphan1 )
				{
					if ( outputAlgn[0].D.size() < ob->blocksizea )
						outputAlgn[0].D = libmaus::bambam::BamAlignment::D_array_type(ob->blocksizea);
					std::copy(ob->Da,ob->Da + ob->blocksizea, outputAlgn[0].D.begin());
					outputAlgn[0].blocksize = ob->blocksizea;

					P = std::pair <libmaus::bambam::BamAlignment const *, libmaus::bambam::BamAlignment const * >(
						&(outputAlgn[0]),static_cast<libmaus::bambam::BamAlignment const *>(0)
					);
					
					return true;
				}
				else // if ( ob->forphan2 )
				{
					if ( outputAlgn[0].D.size() < ob->blocksizea )
						outputAlgn[0].D = libmaus::bambam::BamAlignment::D_array_type(ob->blocksizea);
					std::copy(ob->Da,ob->Da + ob->blocksizea, outputAlgn[0].D.begin());
					outputAlgn[0].blocksize = ob->blocksizea;

					P = std::pair <libmaus::bambam::BamAlignment const *, libmaus::bambam::BamAlignment const * >(
						static_cast<libmaus::bambam::BamAlignment const *>(0),&(outputAlgn[0])
					);
					
					return true;
				}
			}
			
			/**
			 * @return BAM header
			 **/
			libmaus::bambam::BamHeader const & getHeader() const
			{
				return bamdec.getHeader();
			}

			/**
			 * set input call back which is called every time a single alignment line is read from the BAM file
			 *
			 * @param rinputcallback pointer to callback object (null for none)
			 **/
			void setInputCallback(CollatingBamDecoderAlignmentInputCallback * rinputcallback)
			{
				inputcallback = rinputcallback;			
			}

			/**
			 * set first expunge callback
			 *
			 * @param callback
			 **/
			void setPrimaryExpungeCallback(libmaus::bambam::BamAlignmentExpungeCallback * callback)
			{
				CH->setExpungeCallback(callback);
			}

			/**
			 * set secondary expunge callback
			 *
			 * @param callback
			 **/
			void setSecondaryExpungeCallback(libmaus::bambam::BamAlignmentExpungeCallback * callback)
			{
				NCHEO->setExpungeCallback(callback);
			}

			/**
			 * @return next rank
			 **/
			uint64_t getRank() const
			{
				return bamdec.getRank();
			}

			/**
			 * disable input validation for BAM decoder
			 **/
			void disableValidation()
                        {
                        	bamdec.disableValidation();
			}

			/**
			 * print circular hash expunge counters (if present)
			 *
			 * @param out stream used for printing
			 * @return out
			 **/
			std::ostream & printCounters(std::ostream & out) const
			{
				if ( CH )
					return CH->printCounters(out);
				else
					return out;
			}
		};

		/**
		 * circular hash based BAM collation class based on serial bgzf input
		 **/
		struct BamCircularHashCollatingBamDecoder :
			public BamDecoderWrapper, public CircularHashCollatingBamDecoder
		{
			//! this type
			typedef BamCircularHashCollatingBamDecoder this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			/**
			 * constructor from input stream
			 *
			 * @param in input stream
			 * @param rtmpfilename temporary file name
			 * @param rexcludeflags ignore alignments matching any of these flags
			 * @param rputrank put rank (line number) on alignments at input time
			 * @param hlog log_2 of hash table size used for collation
			 * @param sortbufsize overflow sort buffer size in bytes
			 **/
			BamCircularHashCollatingBamDecoder(
				std::istream & in,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags = 0,
				bool const rputrank = false,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			) : BamDecoderWrapper(in,rputrank), 
			    CircularHashCollatingBamDecoder(BamDecoderWrapper::bamdec,rtmpfilename,rexcludeflags,hlog,sortbufsize)
			{
			
			}

			/**
			 * constructor from input stream and output stream; the compressed input
			 * data will be copied to the output stream as is
			 *
			 * @param in input stream
			 * @param copyout copy output stream
			 * @param rtmpfilename temporary file name
			 * @param rexcludeflags ignore alignments matching any of these flags
			 * @param rputrank put rank (line number) on alignments at input time
			 * @param hlog log_2 of hash table size used for collation
			 * @param sortbufsize overflow sort buffer size in bytes
			 **/
			BamCircularHashCollatingBamDecoder(
				std::istream & in,
				std::ostream & copyout,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags = 0,
				bool const rputrank = false,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			) : BamDecoderWrapper(in,copyout,rputrank), 
			    CircularHashCollatingBamDecoder(BamDecoderWrapper::bamdec,rtmpfilename,rexcludeflags,hlog,sortbufsize)
			{
			
			}
		};

		/**
		 * circular hash based BAM collation class based on merging by coordinate input
		 **/
		struct BamMergeCoordinateCircularHashCollatingBamDecoder :
			public BamMergeCoordinateWrapper, public CircularHashCollatingBamDecoder
		{
			//! this type
			typedef BamMergeCoordinateCircularHashCollatingBamDecoder this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			/**
			 * constructor from input stream
			 *
			 * @param in input stream
			 * @param rtmpfilename temporary file name
			 * @param rexcludeflags ignore alignments matching any of these flags
			 * @param rputrank put rank (line number) on alignments at input time
			 * @param hlog log_2 of hash table size used for collation
			 * @param sortbufsize overflow sort buffer size in bytes
			 **/
			BamMergeCoordinateCircularHashCollatingBamDecoder(
				std::vector<std::string> const & filenames,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags = 0,
				bool const rputrank = false,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			) : BamMergeCoordinateWrapper(filenames,rputrank), 
			    CircularHashCollatingBamDecoder(BamMergeCoordinateWrapper::object,rtmpfilename,rexcludeflags,hlog,sortbufsize)
			{
			
			}
		};

		/**
		 * circular hash based BAM collation class based on queryname based merging
		 **/
		struct BamMergeQueryNameCircularHashCollatingBamDecoder :
			public BamMergeQueryNameWrapper, public CircularHashCollatingBamDecoder
		{
			//! this type
			typedef BamMergeQueryNameCircularHashCollatingBamDecoder this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			/**
			 * constructor from input stream
			 *
			 * @param in input stream
			 * @param rtmpfilename temporary file name
			 * @param rexcludeflags ignore alignments matching any of these flags
			 * @param rputrank put rank (line number) on alignments at input time
			 * @param hlog log_2 of hash table size used for collation
			 * @param sortbufsize overflow sort buffer size in bytes
			 **/
			BamMergeQueryNameCircularHashCollatingBamDecoder(
				std::vector<std::string> const & filenames,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags = 0,
				bool const rputrank = false,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			) : BamMergeQueryNameWrapper(filenames,rputrank), 
			    CircularHashCollatingBamDecoder(BamMergeQueryNameWrapper::object,rtmpfilename,rexcludeflags,hlog,sortbufsize)
			{
			
			}
		};

		/**
		 * circular hash based BAM collation class based on parallel bgzf input
		 **/
		struct BamParallelCircularHashCollatingBamDecoder :
			public BamParallelDecoderWrapper, public CircularHashCollatingBamDecoder
		{
			//! this type
			typedef BamParallelCircularHashCollatingBamDecoder this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			/**
			 * constructor from input stream
			 *
			 * @param in input stream
			 * @param numthreads number of decoding threads
			 * @param rtmpfilename temporary file name
			 * @param rexcludeflags ignore alignments matching any of these flags
			 * @param rputrank put rank (line number) on alignments at input time
			 * @param hlog log_2 of hash table size used for collation
			 * @param sortbufsize overflow sort buffer size in bytes
			 **/
			BamParallelCircularHashCollatingBamDecoder(
				std::istream & in,
				uint64_t const numthreads,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags = 0,
				bool const rputrank = false,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			) : BamParallelDecoderWrapper(in,numthreads,rputrank), 
			    CircularHashCollatingBamDecoder(BamParallelDecoderWrapper::bamdec,rtmpfilename,rexcludeflags,hlog,sortbufsize)
			{
			
			}

			/**
			 * constructor from input stream and output stream; the compressed input
			 * data will be copied to the output stream as is
			 *
			 * @param in input stream
			 * @param copyout copy output stream
			 * @param numthreads number of decoding threads
			 * @param rtmpfilename temporary file name
			 * @param rexcludeflags ignore alignments matching any of these flags
			 * @param rputrank put rank (line number) on alignments at input time
			 * @param hlog log_2 of hash table size used for collation
			 * @param sortbufsize overflow sort buffer size in bytes
			 **/
			BamParallelCircularHashCollatingBamDecoder(
				std::istream & in,
				std::ostream & copyout,
				uint64_t const numthreads,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags = 0,
				bool const rputrank = false,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			) : BamParallelDecoderWrapper(in,copyout,numthreads,rputrank), 
			    CircularHashCollatingBamDecoder(BamParallelDecoderWrapper::bamdec,rtmpfilename,rexcludeflags,hlog,sortbufsize)
			{
			
			}
		};

		/**
		 * circular hash based BAM collation class based on io_lib input (for SAM, BAM and CRAM)
		 **/
		struct ScramCircularHashCollatingBamDecoder :
			public ScramDecoderWrapper, public CircularHashCollatingBamDecoder
		{
			//! this type
			typedef ScramCircularHashCollatingBamDecoder this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			/**
			 * constructor
			 * 
			 * @param filename input file name (- for stdin)
			 * @param mode file mode, "r" for sam, "rb" for BAM, "rc" for CRAM
			 * @param reference file name of reference FastA file (for CRAM, pass empty string for SAM or BAM)
			 * @param rtmpfilename temporary file name for collation
			 * @param rexcludeflags ignore alignments matching any of these flags
			 * @param rputrank put rank (line number) on alignments at input time
			 * @param hlog log_2 of hash table size used for collation
			 * @param sortbufsize overflow sort buffer size in bytes
			 **/
			ScramCircularHashCollatingBamDecoder(
				std::string const & filename,
				std::string const & mode,
				std::string const & reference,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags = 0,
				bool const rputrank = false,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			) : ScramDecoderWrapper(filename,mode,reference,rputrank), 
			    CircularHashCollatingBamDecoder(ScramDecoderWrapper::scramdec,rtmpfilename,rexcludeflags,hlog,sortbufsize)
			{
			
			}
		};

		/**
		 * circular hash based BAM collation class based on range restricted BAM decoding
		 **/
		struct BamRangeCircularHashCollatingBamDecoder :
			public BamRangeDecoderWrapper, public CircularHashCollatingBamDecoder
		{
			//! this type
			typedef BamRangeCircularHashCollatingBamDecoder this_type;
			//! unique pointer type
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			/**
			 * constructor
			 * 
			 * @param filename input file name
			 * @param ranges range selection string
			 * @param rtmpfilename temporary file name for collation
			 * @param rexcludeflags ignore alignments matching any of these flags
			 * @param rputrank put rank (line number) on alignments at input time
			 * @param hlog log_2 of hash table size used for collation
			 * @param sortbufsize overflow sort buffer size in bytes
			 **/
			BamRangeCircularHashCollatingBamDecoder(
				std::string const & filename,
				std::string const & ranges,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags = 0,
				bool const rputrank = false,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			) : BamRangeDecoderWrapper(filename,ranges,rputrank), 
			    CircularHashCollatingBamDecoder(BamRangeDecoderWrapper::decoder,rtmpfilename,rexcludeflags,hlog,sortbufsize)
			{
			
			}
		};
	}
}
#endif
