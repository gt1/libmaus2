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
#if ! defined(LIBMAUS_BAMBAM_CIRCULARHASHCOLLATINGBAMDECODER_HPP)
#define LIBMAUS_BAMBAM_CIRCULARHASHCOLLATINGBAMDECODER_HPP

#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/bambam/BamAlignmentSortingCircularHashEntryOverflow.hpp>
#include <libmaus/hashing/CircularHash.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct CircularHashCollatingBamDecoder
		{
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


			template<typename cht>
			bool isPair(cht const & CH, libmaus::bambam::BamAlignment const & algn, uint32_t const hv)
			{
				std::pair<typename cht::pos_type,typename cht::entry_size_type> const hentry = CH.getEntry(hv);

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

			libmaus::bambam::BamDecoder bamdec;
			libmaus::bambam::BamAlignment const & algn;
			libmaus::bambam::BamAlignment mergealgn[2];
			unsigned int mergealgnptr;
			std::string const tmpfilename;

			typedef libmaus::bambam::BamAlignmentSortingCircularHashEntryOverflow overflow_type;
			overflow_type NCHEO;

			libmaus::autoarray::AutoArray<uint8_t> T;
			
			typedef libmaus::hashing::CircularHash<overflow_type> cht;
			cht CH;

			libmaus::bambam::SnappyAlignmentMergeInput::unique_ptr_type mergeinput;
			circ_hash_collator_state state;
			
			struct OutputBufferEntry
			{
				uint8_t const * Da;
				uint64_t blocksizea;
				uint8_t const * Db;
				uint64_t blocksizeb;
				
				bool fsingle;
				bool fpair;
				bool forphan1;
				bool forphan2;
				
				OutputBufferEntry()
				: Da(0), blocksizea(0), Db(0), blocksizeb(0), fsingle(false), fpair(false),
				  forphan1(false), forphan2(false)
				{}
			};
			
			OutputBufferEntry outputBuffer;
			libmaus::bambam::BamAlignment outputAlgn[2];
			
			uint32_t const excludeflags;
			
			CircularHashCollatingBamDecoder(
				std::istream & in,
				std::string const & rtmpfilename,
				uint32_t const rexcludeflags,
				unsigned int const hlog = 18,
				uint64_t const sortbufsize = 128ull*1024ull*1024ull
			)
			: bamdec(in), algn(bamdec.alignment), mergealgnptr(0), tmpfilename(rtmpfilename), 
			  NCHEO(tmpfilename,sortbufsize), CH(NCHEO,hlog), state(state_reading),
			  excludeflags(rexcludeflags)
			{
			
			}
			
			OutputBufferEntry const * process()
			{
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
			
				while ( 
					state != state_done && state != state_failed
					&&
					(!outputBuffer.fsingle) && (!outputBuffer.fpair) && (!outputBuffer.forphan1) && (!outputBuffer.forphan2)
				)
				{
					if ( state == state_sortbuffer_flushing_intermediate )
					{
						NCHEO.flush();								
						state = state_sortbuffer_flushing_intermediate_readout;
					}
					if ( state == state_sortbuffer_flushing_intermediate_readout )
					{
						uint8_t const * ptr = 0;
						uint64_t length = 0;
						
						// this call produces pairs in steps
						if ( NCHEO.getFlushBufEntry(ptr,length) )
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
						NCHEO.flush();
						state = state_sortbuffer_flushing_final_readout;
					}
					else if ( state == state_sortbuffer_flushing_final_readout )
					{
						uint8_t const * ptr = 0;
						uint64_t length = 0;
						
						// as above, this produces pairs in steps
						if ( NCHEO.getFlushBufEntry(ptr,length) )
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
						mergeinput = UNIQUE_PTR_MOVE(NCHEO.constructMergeInput());
						state = state_merging;			
					}
					else if ( state == state_reading )
					{
						if ( bamdec.readAlignment() )
						{
							if ( algn.getFlags() & excludeflags )
								continue;
						
							uint8_t const * data = algn.D.begin();
							uint64_t const datalen = algn.blocksize;
							
							if ( ! algn.isPaired() )
							{
								outputBuffer.Da = data;
								outputBuffer.blocksizea = datalen;
								outputBuffer.fsingle = true;
							}
							else
							{
								uint32_t const hv = algn.hash32();

								// see if we found a pair
								if ( CH.hasEntry(hv) && isPair(CH,algn,hv) )
								{
									std::pair<cht::pos_type,cht::entry_size_type> const hentry = CH.getEntry(hv);
									
									if ( algn.isRead1() )
									{
										outputBuffer.Da = data;
										outputBuffer.blocksizea = datalen;
										outputBuffer.Db = CH.getEntryData(hentry,T);
										outputBuffer.blocksizeb = hentry.second;
										outputBuffer.fpair = true;
									}
									else
									{
										outputBuffer.Da = CH.getEntryData(hentry,T);
										outputBuffer.blocksizea = hentry.second;
										outputBuffer.Db = data;
										outputBuffer.blocksizeb = datalen;
										outputBuffer.fpair = true;
									}
									
									CH.eraseEntry(hv);
								}
								else
								{
									if ( ! CH.putEntry(data,datalen,hv) )
									{
										bamdec.putback();
										state = state_sortbuffer_flushing_intermediate;
									}
								}
							}
						}
						else
						{
							if ( !CH.flush() )
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
			
			std::pair <
				libmaus::bambam::BamAlignment const *,
				libmaus::bambam::BamAlignment const *
			> tryPair()
			{
				OutputBufferEntry const * ob = process();
				
				if ( ! ob )
				{
					return std::pair <libmaus::bambam::BamAlignment const *, libmaus::bambam::BamAlignment const * >();
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

					return std::pair <libmaus::bambam::BamAlignment const *, libmaus::bambam::BamAlignment const * >(
						&(outputAlgn[0]),&(outputAlgn[1])
					);
				}
				else if ( ob->fsingle )
				{
					if ( outputAlgn[0].D.size() < ob->blocksizea )
						outputAlgn[0].D = libmaus::bambam::BamAlignment::D_array_type(ob->blocksizea);
					std::copy(ob->Da,ob->Da + ob->blocksizea, outputAlgn[0].D.begin());
					outputAlgn[0].blocksize = ob->blocksizea;

					return std::pair <libmaus::bambam::BamAlignment const *, libmaus::bambam::BamAlignment const * >(
						&(outputAlgn[0]),static_cast<libmaus::bambam::BamAlignment const *>(0)
					);
				}
				else if ( ob->forphan1 )
				{
					if ( outputAlgn[0].D.size() < ob->blocksizea )
						outputAlgn[0].D = libmaus::bambam::BamAlignment::D_array_type(ob->blocksizea);
					std::copy(ob->Da,ob->Da + ob->blocksizea, outputAlgn[0].D.begin());
					outputAlgn[0].blocksize = ob->blocksizea;

					return std::pair <libmaus::bambam::BamAlignment const *, libmaus::bambam::BamAlignment const * >(
						&(outputAlgn[0]),static_cast<libmaus::bambam::BamAlignment const *>(0)
					);
				}
				else // if ( ob->forphan2 )
				{
					if ( outputAlgn[0].D.size() < ob->blocksizea )
						outputAlgn[0].D = libmaus::bambam::BamAlignment::D_array_type(ob->blocksizea);
					std::copy(ob->Da,ob->Da + ob->blocksizea, outputAlgn[0].D.begin());
					outputAlgn[0].blocksize = ob->blocksizea;

					return std::pair <libmaus::bambam::BamAlignment const *, libmaus::bambam::BamAlignment const * >(
						static_cast<libmaus::bambam::BamAlignment const *>(0),&(outputAlgn[0])
					);
				}
			}
		};
	}
}
#endif
