/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_BAMBAM_BAMSTREAMINGMARKDUPLICATES_HPP)
#define LIBMAUS_BAMBAM_BAMSTREAMINGMARKDUPLICATES_HPP

#include <libmaus/bambam/BamBlockWriterBase.hpp>
#include <libmaus/bambam/BamStreamingMarkDuplicatesSupport.hpp>
#include <libmaus/bambam/DuplicationMetrics.hpp>
#include <libmaus/fastx/FastATwoBitTable.hpp>
#include <libmaus/trie/SimpleTrie.hpp>
#include <libmaus/util/SimpleHashMapInsDel.hpp>
#include <libmaus/util/SimpleHashSetInsDel.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamStreamingMarkDuplicates : public libmaus::bambam::BamStreamingMarkDuplicatesSupport, public libmaus::bambam::BamBlockWriterBase
		{
			typedef BamStreamingMarkDuplicates this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
					
			static int getDefaultMaxReadLen() { return 300; }
			static int getDefaultOptMinPixelDif() { return 100; }

			static void processOpticalList(
				std::vector<OpticalExternalInfoElement> & optlist,
				std::vector<bool> & optb,
				libmaus::bambam::BamHeader const & header,
				std::map<uint64_t,::libmaus::bambam::DuplicationMetrics> & metrics,
				unsigned int const optminpixeldif
			)
			{
				// erase boolean vector
				for ( uint64_t i = 0; i < std::min(optb.size(),optlist.size()); ++i )
					optb[i] = false;

				for ( uint64_t i = 0; i+1 < optlist.size(); ++i )
					for ( uint64_t j = i+1; j < optlist.size(); ++j )
						if (
							optlist[j].x - optlist[i].x <= optminpixeldif
							&&
							libmaus::math::iabs(static_cast<int64_t>(optlist[j].y)-static_cast<int64_t>(optlist[i].y)) <= optminpixeldif
						)
						{
							while ( j >= optb.size() )
								optb.push_back(false);

							optb[j] = true;
						}
						
				uint64_t opt = 0;
				for ( uint64_t i = 1; i < std::min(optb.size(),optlist.size()); ++i )
					if ( optb[i] )
						opt += 1;
						
				if ( opt )
				{
					int64_t const thislib = header.getLibraryId(static_cast<int64_t>(optlist[0].readgroup)-1);
					::libmaus::bambam::DuplicationMetrics & met = metrics[thislib];		
					met.opticalduplicates += opt;

					#if 0
					PairHashKeyType const HK(optlist[0].key);
					printOpt(std::cerr,HK,opt);
					#endif
				}
				
				optlist.resize(0);
			}

			unsigned int const optminpixeldif;
			int64_t const maxreadlen;
			std::string const tmpfilenamebase;
			bool const filterdupmarktags;
			
			libmaus::bambam::BamHeader const & header;

			libmaus::bambam::BamBlockWriterBase & wr;

			libmaus::util::GrowingFreeList<libmaus::bambam::BamAlignment> BAFL;
			libmaus::util::FreeList<OpticalInfoListNode> OILNFL;

			OutputQueue<libmaus::bambam::BamBlockWriterBase> OQ;

			std::string const optfn;
			libmaus::sorting::SortingBufferedOutputFile<OpticalExternalInfoElement> optSBOF;

			std::priority_queue< PairHashKeyType, std::vector<PairHashKeyType>, PairHashKeyHeapComparator > Qpair;
			libmaus::util::SimpleHashMapInsDel<
				PairHashKeyType,
				std::pair<libmaus::bambam::BamAlignment *, OpticalInfoList>,
				PairHashKeySimpleHashConstantsType,
				::libmaus::util::SimpleHashMapKeyPrint<PairHashKeyType>,
				PairHashKeySimpleHashComputeType,
				PairHashKeySimpleHashNumberCastType
			> SHpair;

			std::priority_queue< FragmentHashKeyType, std::vector<FragmentHashKeyType>, FragmentHashKeyHeapComparator > Qfragment;
			libmaus::util::SimpleHashMapInsDel<
				FragmentHashKeyType,
				libmaus::bambam::BamAlignment *,
				FragmentHashKeySimpleHashConstantsType,
				::libmaus::util::SimpleHashMapKeyPrint<FragmentHashKeyType>,
				FragmentHashKeySimpleHashComputeType,
				FragmentHashKeySimpleHashNumberCastType
				> SHfragment;
			
			std::priority_queue< FragmentHashKeyType, std::vector<FragmentHashKeyType>, FragmentHashKeyHeapComparator > Qpairfragments;
			libmaus::util::SimpleHashSetInsDel<
				FragmentHashKeyType,
				FragmentHashKeySimpleHashConstantsType,
				::libmaus::util::SimpleHashMapKeyPrint<FragmentHashKeyType>,
				FragmentHashKeySimpleHashComputeType,
				FragmentHashKeySimpleHashNumberCastType
			> SHpairfragments;

			libmaus::autoarray::AutoArray<OpticalInfoListNode *> optA;
			libmaus::autoarray::AutoArray<bool> optB;

			std::map<uint64_t,::libmaus::bambam::DuplicationMetrics> metrics;
			
			double const hashloadfactor;
			int64_t prevcheckrefid;
			int64_t prevcheckpos;
			
			libmaus::bambam::BamAlignment tmpalgn;

			enum tag_type_enum
			{
				tag_type_none,
				tag_type_string,
				tag_type_nucleotide
			};
			
			bool const havetag;
			std::string const tag;
			libmaus::trie::SimpleTrie::unique_ptr_type Ptagtrie;

			bool const havenucltag;
			std::string const nucltag;

			tag_type_enum tag_type;

			char const * ctag;
			char const * cnucltag;

			libmaus::autoarray::AutoArray<char> tagbuffer;
			libmaus::fastx::FastATwoBitTable const FATBT;
			
			bool const putrank;
			uint64_t nextrank;
			
			static std::vector<std::string> getFilterTags()
			{
				std::vector<std::string> V;
				V.push_back("MQ");
				V.push_back("MS");
				V.push_back("MC");
				V.push_back("MT");
				return V;
			}

			BamStreamingMarkDuplicates(
				libmaus::util::ArgInfo const & arginfo, 
				libmaus::bambam::BamHeader const & rheader,
				libmaus::bambam::BamBlockWriterBase & rwr,
				bool const rfilterdupmarktags,
				bool const rputrank = false
			)
			: optminpixeldif(arginfo.getValue<unsigned int>("optminpixeldif",getDefaultOptMinPixelDif())),
			  maxreadlen(arginfo.getValue<uint64_t>("maxreadlen",getDefaultMaxReadLen())),
			  tmpfilenamebase(arginfo.getUnparsedValue("tmpfile",arginfo.getDefaultTmpFileName())),
			  filterdupmarktags(rfilterdupmarktags),
			  header(rheader),
			  wr(rwr),
			  BAFL(),
			  OILNFL(32*1024),
			  OQ(wr,BAFL,tmpfilenamebase,filterdupmarktags ? getFilterTags() : std::vector<std::string>()),
			  optfn(tmpfilenamebase+"_opt"),
			  optSBOF(optfn),
			  Qpair(),
			  SHpair(0),
			  Qfragment(),
			  SHfragment(0),
			  Qpairfragments(),
			  SHpairfragments(0),
			  optA(),
			  optB(),
			  metrics(),
			  hashloadfactor(.8),
			  prevcheckrefid(std::numeric_limits<int64_t>::min()),
			  prevcheckpos(std::numeric_limits<int64_t>::min()),
			  havetag(arginfo.hasArg("tag")),
			  tag(arginfo.getUnparsedValue("tag","no tag")),
			  Ptagtrie(havetag ? new libmaus::trie::SimpleTrie : 0),
			  havenucltag(arginfo.hasArg("nucltag")),
			  nucltag(arginfo.getUnparsedValue("nucltag","no tag")),
			  tag_type(havetag ? tag_type_string : (havenucltag ? tag_type_nucleotide : tag_type_none)),
			  ctag(havetag ? tag.c_str() : 0),
			  cnucltag(havenucltag ? nucltag.c_str() : 0),
			  putrank(rputrank),
			  nextrank(0)
			{
				libmaus::util::TempFileRemovalContainer::addTempFile(optfn);

				// tag field
				if ( havetag && (tag.size() != 2 || (!isalpha(tag[0])) || (!isalnum(tag[1])) ) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "tag " << tag << " is invalid" << std::endl;
					se.finish();
					throw se;			
				}
	
				if ( havetag )
				{
					// allocate tag id 0 for empty tag
					uint8_t const * p = 0;
					Ptagtrie->insert(p,p);
				}

				if ( havenucltag && (nucltag.size() != 2 || (!isalpha(nucltag[0])) || (!isalnum(nucltag[1])) ) )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "nucltag " << tag << " is invalid" << std::endl;
					se.finish();
					throw se;			
				}
	
				if ( havetag && havenucltag )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "tag and nucltag are mutually exclusive" << std::endl;
					se.finish();
					throw se;					
				}
			}

			void addAlignment(libmaus::bambam::BamAlignment & algn)
			{
				if ( putrank )
					algn.putRank("ZR",nextrank++);
			
				int64_t const thisref = algn.getRefID();
				int64_t const thispos = algn.getPos();

				// map negative to maximum positive for checking order
				int64_t const thischeckrefid = (thisref >= 0) ? thisref : std::numeric_limits<int64_t>::max();
				int64_t const thischeckpos   = (thispos >= 0) ? thispos : std::numeric_limits<int64_t>::max();
							
				// true iff order is ok
				bool const orderok =
					(thischeckrefid > prevcheckrefid)
					||
					(thischeckrefid == prevcheckrefid && thischeckpos >= prevcheckpos);

				if ( ! orderok )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "[E] input file is not coordinate sorted." << std::endl;
					lme.finish();
					throw lme;
				}

				prevcheckrefid = thischeckrefid;
				prevcheckpos = thischeckpos;

				if ( algn.getLseq() > maxreadlen )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "[E] file contains read of length " << algn.getLseq() << " > maxreadlen=" << maxreadlen << std::endl;
					lme.getStream() << "[E] please increase the maxreadlen option accordingly." << std::endl;
					lme.finish();
					throw lme;		
				}

				uint64_t const thislib = algn.getLibraryId(header);
				::libmaus::bambam::DuplicationMetrics & met = metrics[thislib];
				
				if ( ! algn.isMapped() )
					++met.unmapped;
				else if ( (!algn.isPaired()) || algn.isMateUnmap() )
					++met.unpaired;

				uint64_t tagid = 0;

				// compute tagid (if any)
				switch ( tag_type )
				{
					case tag_type_string:
					{
						// length of tags for read1 and read2
						uint64_t l1 = 0, l2 = 0;
						// tags
						char const * tag1 = 0;
						char const * tag2 = 0;
						// 
						bool const is1 = (!algn.isPaired()) || algn.isRead1();
						
						assert ( ctag );
						tag1 = is1  ? algn.getAuxString(ctag) : algn.getAuxString("MT");
						l1 = tag1 ? strlen(tag1) : 0;

						if ( algn.isPaired() )
						{
							tag2 = is1  ? algn.getAuxString("MT") : algn.getAuxString(ctag);
							l2 = tag2 ? strlen(tag2) : 0;
						}
						
						// length of concatenated tag
						uint64_t const taglen = l1 + l2 + 2;
						// expand buffer if necessary
						if ( taglen > tagbuffer.size() )
							tagbuffer = libmaus::autoarray::AutoArray<char>(taglen,false);

						// concatenate tags
						char * outptr = tagbuffer.begin();

						memcpy(outptr,tag1,l1);
						outptr += l1;
						*(outptr++) = 0;

						memcpy(outptr,tag2,l2);
						outptr += l2;
						*(outptr++) = 0;

						assert ( outptr - tagbuffer.begin() == static_cast<ptrdiff_t>(taglen) );

						// look up tag id			
						tagid = Ptagtrie->insert(
							tagbuffer.begin(),
							outptr
						);

						break;
					}
					case tag_type_nucleotide:
					{
						assert ( cnucltag );

						// tags
						char const * tag1 = 0;
						char const * tag2 = 0;
						
						if ( algn.isPaired() )
						{
							if ( algn.isRead1() )
							{
								tag1 = algn.getAuxString(cnucltag);						
								tag2 = algn.getAuxString("MT");
							}
							else
							{
								tag2 = algn.getAuxString(cnucltag);						
								tag1 = algn.getAuxString("MT");							
							}
						}
						else
						{
							tag1 = algn.getAuxString(cnucltag);
						}						

						tagid = (FATBT(tag1) << 32) | FATBT(tag2);
						
						break;
					}
					default:
					{
						tagid = 0;
						break;
					}
				}

				if ( 
					// supplementary alignment
					algn.isSupplementary() 
					|| 
					// secondary alignment
					algn.isSecondary() 
					||
					// failed quality control
					algn.isQCFail()
					||
					// single, unmapped
					((!algn.isPaired()) && (!algn.isMapped()))
					||
					// paired, end unmapped
					(algn.isPaired() && (!algn.isMapped()))
				)
				{
					// pass through
					libmaus::bambam::BamAlignment * palgn = BAFL.get();
					palgn->swap(algn);
					OQ.push(palgn);
				}
				// paired end, both mapped
				else if ( 
					algn.isPaired()
					&&
					algn.isMapped()
					&&
					algn.isMateMapped()
				)
				{
					/* 
					 * build the hash key containing
					 *
					 * - ref id
					 * - coordinate (position plus softclipping)
					 * - mate ref id
					 * - mate coordinate (position plus softclipping)
					 * - library id
					 * - flag whether right side read of pair (the one with the higher coordinate)
					 * - orientation
					 * - tag
					 * - mate tag
					 */
					PairHashKeyType HK(algn,header,tagid);

					if ( algn.isRead1() )
						met.readpairsexamined++;

					// pair fragment for marking single mappings on same coordinate as duplicates
					FragmentHashKeyType HK1(algn,header,false /* use mate */,tagid);
					if ( !SHpairfragments.contains(HK1) )
					{
						SHpairfragments.insertExtend(HK1,hashloadfactor);
						Qpairfragments.push(HK1);					
						assert ( SHpairfragments.contains(HK1) );
					}
					
					uint64_t keyindex;

					// type has been seen before
					if ( SHpair.containsKey(HK,keyindex) )
					{
						// pair reference
						std::pair<libmaus::bambam::BamAlignment *, OpticalInfoList> & SHpairinfo = SHpair.getValue(keyindex);
						// add optical info
						SHpairinfo.second.addOpticalInfo(algn,OILNFL,HK,optSBOF,header);
						// stored previous alignment
						libmaus::bambam::BamAlignment * oalgn = SHpairinfo.first;
						// score for other alignment
						int64_t const oscore = oalgn->getScore() + oalgn->getAuxAsNumber<int32_t>("MS");
						// score for this alignment
						int64_t const tscore =   algn.getScore() +   algn.getAuxAsNumber<int32_t>("MS");
						// decide on read name if score is the same
						int64_t const tscoreadd = (tscore == oscore) ? ((strcmp(algn.getName(),oalgn->getName()) < 0)?1:-1) : 0;

						// update metrics
						met.readpairduplicates++;

						// this score is greater
						if ( (tscore+tscoreadd) > oscore )
						{
							// mark previous alignment as duplicate
							oalgn->putFlags(oalgn->getFlags() | libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FDUP);

							// get alignment from free list
							libmaus::bambam::BamAlignment * palgn = BAFL.get();
							// swap 
							palgn->swap(*oalgn);
							// 
							OQ.push(palgn);
							
							// swap alignment data
							oalgn->swap(algn);
						}
						// other score is greater, mark this alignment as duplicate
						else
						{
							algn.putFlags(algn.getFlags() | libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FDUP);
							
							libmaus::bambam::BamAlignment * palgn = BAFL.get();
							palgn->swap(algn);
							OQ.push(palgn);
						}
					}
					// type is new
					else
					{
						libmaus::bambam::BamAlignment * palgn = BAFL.get();
						palgn->swap(algn);
						
						Qpair.push(HK);
						
						keyindex = SHpair.insertExtend(
							HK,
							std::pair<libmaus::bambam::BamAlignment *, OpticalInfoList>(palgn,OpticalInfoList()),
							hashloadfactor
						);
						// pair reference
						std::pair<libmaus::bambam::BamAlignment *, OpticalInfoList> & SHpairinfo = SHpair.getValue(keyindex);
						// add optical info
						SHpairinfo.second.addOpticalInfo(*(SHpairinfo.first),OILNFL,HK,optSBOF,header);
					}
				}
				// single end or pair with one end mapping only
				else
				{
					FragmentHashKeyType HK(algn,header,false /* use mate */,tagid);

					libmaus::bambam::BamAlignment * oalgn;
			
					if ( SHfragment.contains(HK,oalgn) )
					{
						// score for other alignment
						int64_t const oscore = oalgn->getScore();
						// score for this alignment
						int64_t const tscore = algn.getScore();

						// update metrics
						met.unpairedreadduplicates += 1;
						
						// this score is greater
						if ( tscore > oscore )
						{
							// mark previous alignment as duplicate
							oalgn->putFlags(oalgn->getFlags() | libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FDUP);

							// put oalgn value in output queue
							libmaus::bambam::BamAlignment * palgn = BAFL.get();
							palgn->swap(*oalgn);
							OQ.push(palgn);

							// swap alignment data
							oalgn->swap(algn);
						}
						// other score is greater, mark this alignment as duplicate
						else
						{
							algn.putFlags(algn.getFlags() | libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FDUP);

							libmaus::bambam::BamAlignment * palgn = BAFL.get();
							palgn->swap(algn);
							OQ.push(palgn);
						}
					}
					else
					{
						libmaus::bambam::BamAlignment * palgn = BAFL.get();
						palgn->swap(algn);
						
						SHfragment.insertExtend(HK,palgn,hashloadfactor);
						Qfragment.push(HK);
					}
				}
				
				while ( 
					Qpair.size ()
					&&
					(
						(Qpair.top().getRefId() != thisref)
						||
						(Qpair.top().getRefId() == thisref && Qpair.top().getCoord()+maxreadlen < thispos)
					)
				)
				{
					PairHashKeyType const HK = Qpair.top(); Qpair.pop();

					uint64_t const SHpairindex = SHpair.getIndexUnchecked(HK);
					std::pair<libmaus::bambam::BamAlignment *, OpticalInfoList> SHpairinfo = SHpair.getValue(SHpairindex);
					libmaus::bambam::BamAlignment * palgn = SHpairinfo.first;

					uint64_t const opt = SHpairinfo.second.countOpticalDuplicates(optA,optB,optminpixeldif);
					
					if ( opt )
					{
						uint64_t const thislib = palgn->getLibraryId(header);
						::libmaus::bambam::DuplicationMetrics & met = metrics[thislib];	
						met.opticalduplicates += opt;
						
						#if 0		
						printOpt(std::cerr,HK,opt);
						#endif
					}

					OQ.push(palgn);

					SHpairinfo.second.deleteOpticalInfo(OILNFL);
					SHpair.eraseIndex(SHpairindex);
				}

				while ( 
					Qfragment.size ()
					&&
					(
						(Qfragment.top().getRefId() != thisref)
						||
						(Qfragment.top().getRefId() == thisref && Qfragment.top().getCoord()+maxreadlen < thispos)
					)
				)
				{
					FragmentHashKeyType const HK = Qfragment.top(); Qfragment.pop();

					uint64_t const SHfragmentindex = SHfragment.getIndexUnchecked(HK);
					libmaus::bambam::BamAlignment * palgn = SHfragment.getValue(SHfragmentindex);
					
					if ( SHpairfragments.contains(HK) )
					{
						// update metrics
						met.unpairedreadduplicates += 1;
						
						palgn->putFlags(palgn->getFlags() | libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FDUP);				
					}
				
					OQ.push(palgn);
					
					SHfragment.eraseIndex(SHfragmentindex);
				}

				while ( 
					Qpairfragments.size ()
					&&
					(
						(Qpairfragments.top().getRefId() != thisref)
						||
						(Qpairfragments.top().getRefId() == thisref && Qpairfragments.top().getCoord()+maxreadlen < thispos)
					)
				)
				{
					FragmentHashKeyType const & HK = Qpairfragments.top();
					
					SHpairfragments.eraseIndex(SHpairfragments.getIndexUnchecked(HK));
					
					Qpairfragments.pop();
				}
			}
			
			void flush()
			{
				// flush remaining pair information
				while ( Qpair.size () )
				{
					PairHashKeyType const HK = Qpair.top(); Qpair.pop();
					
					uint64_t const SHpairindex = SHpair.getIndexUnchecked(HK);
					std::pair<libmaus::bambam::BamAlignment *, OpticalInfoList> SHpairinfo = SHpair.getValue(SHpairindex);
					libmaus::bambam::BamAlignment * palgn = SHpairinfo.first;
						
					OQ.push(palgn);

					uint64_t const opt = SHpairinfo.second.countOpticalDuplicates(optA,optB,optminpixeldif);
						
					if ( opt )
					{
						uint64_t const thislib = palgn->getLibraryId(header);
						::libmaus::bambam::DuplicationMetrics & met = metrics[thislib];		
						met.opticalduplicates += opt;

						#if 0
						printOpt(std::cerr,HK,opt);
						#endif
					}

					SHpairinfo.second.deleteOpticalInfo(OILNFL);
					SHpair.eraseIndex(SHpairindex);
				}

				// flush remaining fragment information
				while ( Qfragment.size () )
				{
					FragmentHashKeyType const HK = Qfragment.top(); Qfragment.pop();

					uint64_t const SHfragmentindex = SHfragment.getIndexUnchecked(HK);
					libmaus::bambam::BamAlignment * palgn = SHfragment.getValue(SHfragmentindex);
					
					if ( SHpairfragments.contains(HK) )
					{
						// update metrics
						metrics[HK.getLibrary()].unpairedreadduplicates += 1;
						
						palgn->putFlags(palgn->getFlags() | libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FDUP);				
					}
				
					OQ.push(palgn);

					SHfragment.eraseIndex(SHfragmentindex);
				}

				// clear pair fragment data structures
				while ( Qpairfragments.size () )
				{
					FragmentHashKeyType const & HK = Qpairfragments.top();
					
					SHpairfragments.eraseIndex(SHpairfragments.getIndexUnchecked(HK));

					Qpairfragments.pop();
				}

				// flush output queue
				OQ.flush();
				
				// process expunged optical data
				libmaus::sorting::SortingBufferedOutputFile<OpticalExternalInfoElement>::merger_ptr_type Poptmerger =
					optSBOF.getMerger();
				libmaus::sorting::SortingBufferedOutputFile<OpticalExternalInfoElement>::merger_type & optmerger =
					*Poptmerger;

				OpticalExternalInfoElement optin;
				std::vector<OpticalExternalInfoElement> optlist;
				std::vector<bool> optb;
				
				while ( optmerger.getNext(optin) )
				{
					if ( optlist.size() && !optlist.back().sameTile(optin) )
						processOpticalList(optlist,optb,header,metrics,optminpixeldif);
					
					optlist.push_back(optin);
				}
				
				if ( optlist.size() )
					processOpticalList(optlist,optb,header,metrics,optminpixeldif);

				// we have counted duplicated pairs for both ends, so divide by two
				for ( std::map<uint64_t,::libmaus::bambam::DuplicationMetrics>::iterator ita = metrics.begin(); ita != metrics.end();
					++ita )
					ita->second.readpairduplicates /= 2;		
			}

			void writeMetrics(libmaus::util::ArgInfo const & arginfo, std::ostream & metricsstr)
			{
				::libmaus::bambam::DuplicationMetrics::printFormatHeader(arginfo.commandline,metricsstr);
				for ( std::map<uint64_t,::libmaus::bambam::DuplicationMetrics>::const_iterator ita = metrics.begin(); ita != metrics.end();
					++ita )
					ita->second.format(metricsstr, header.getLibraryName(ita->first));
				
				if ( metrics.size() == 1 )
				{
					metricsstr << std::endl;
					metricsstr << "## HISTOGRAM\nBIN\tVALUE" << std::endl;
					metrics.begin()->second.printHistogram(metricsstr);
				}
				
				metricsstr.flush();
			}

			void writeMetrics(libmaus::util::ArgInfo const & arginfo)
			{
				::libmaus::aio::CheckedOutputStream::unique_ptr_type pM;
				std::ostream * pmetricstr = 0;
				
				if ( arginfo.hasArg("M") && (arginfo.getValue<std::string>("M","") != "") )
				{
					::libmaus::aio::CheckedOutputStream::unique_ptr_type tpM(
							new ::libmaus::aio::CheckedOutputStream(arginfo.getValue<std::string>("M",std::string("M")))
						);
					pM = UNIQUE_PTR_MOVE(tpM);
					pmetricstr = pM.get();
				}
				else
				{
					pmetricstr = & std::cerr;
				}

				std::ostream & metricsstr = *pmetricstr;

				writeMetrics(arginfo,metricsstr);
				
				pM.reset();
			}

			void writeBamBlock(uint8_t const * D, uint64_t const bs)
			{
				tmpalgn.copyFrom(D,bs);
				addAlignment(tmpalgn);								
			}
			
			void writeAlignment(libmaus::bambam::BamAlignment const & A)
			{
				tmpalgn.copyFrom(A);
				addAlignment(tmpalgn);
			}			
		};
	}
}
#endif
