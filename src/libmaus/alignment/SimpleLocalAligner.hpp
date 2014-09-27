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
#if ! defined(LIBMAUS_ALIGNMENT_SIMPLELOCALALIGNER_HPP)
#define LIBMAUS_ALIGNMENT_SIMPLELOCALALIGNER_HPP

#include <libmaus/alignment/BamLineInfo.hpp>
#include <libmaus/alignment/FMIIntervalComparator.hpp>
#include <libmaus/alignment/KmerTuple.hpp>
#include <libmaus/alignment/KmerInfo.hpp>
#include <libmaus/fastx/FastAIndex.hpp>
#include <libmaus/fm/BidirectionalDnaIndexTemplate.hpp>
#include <libmaus/fm/KmerCache.hpp>
#include <libmaus/fm/SampledISA.hpp>
#include <libmaus/lcs/MetaLocalEditDistance.hpp>

#if defined(LIBMAUS_HAVE_UNSIGNED_INT128)
namespace libmaus
{
	namespace alignment
	{
		template<typename _lf_type>
		struct SimpleLocalAligner
		{
			typedef _lf_type lf_type;
			typedef libmaus::fm::BidirectionalDnaIndexTemplate<lf_type> index_type;

			// default length for the kmer cache			
			static unsigned int getDefaultCacheLen()
			{
				return 12;
			}
			
			// prefix for file names
			std::string const prefix;
			// suffix for hwt file
			std::string const suffix;
			// full name of hwt file
			std::string const hwtname;
			// kmer length
			uint64_t const kmerlen;

			// the fm index			
			index_type index;
			// sampled suffix array
			typename index_type::sa_type const & SA;
			// sampled inverse suffix array
			typename libmaus::fm::SampledISA<lf_type>::unique_ptr_type PISA;
			libmaus::fm::SampledISA<lf_type> const & ISA;
			// lf object
			lf_type const & LF;
			// kmer cache
			libmaus::fm::KmerCache::unique_ptr_type Pcache;
			libmaus::fm::KmerCache const & cache;
			// sequence start offsets in decoded text
			std::vector<uint64_t> const seqstarts;
			
			// kmer mask
			libmaus::uint128_t const mask;
			// indeterminate symbols mask
			libmaus::uint128_t const indetmask;
			
			// maximum error rate in mapped part
			double const errrate;
			// minimum required fraction of hit compared with already achieved score
			double const hitfrac;

			std::string const fafaifile;
			libmaus::fastx::FastAIndex::unique_ptr_type const Pfaindex;

			uint64_t const maxkmerfreqthres;
			
			static libmaus::uint128_t getMask(unsigned int const kmerlen)
			{
				libmaus::uint128_t mask = 0;
				for ( uint64_t i = 0; i < kmerlen; ++i )
				{
					mask <<= 3;
					mask |= static_cast<libmaus::uint128_t>(0x7);
				}
				return mask;
			}

			static libmaus::uint128_t getIndetMask(unsigned int const kmerlen)
			{
				libmaus::uint128_t indetmask = 0;
				for ( uint64_t i = 0; i < kmerlen; ++i )
				{
					indetmask <<= 3;
					indetmask |= static_cast<libmaus::uint128_t>(4);
				}
				return indetmask;
			}

			static uint64_t findSequence(std::vector<uint64_t> const & seqstarts, uint64_t const pos)
			{
				std::vector<uint64_t>::const_iterator ita = std::lower_bound(seqstarts.begin(),seqstarts.end(),pos);

				if ( ita == seqstarts.end() )
					return (ita-seqstarts.begin())-1;
				else
				{
					if ( pos == *ita )
						return ita-seqstarts.begin();
					else
						return (ita-seqstarts.begin())-1;
				}
			}

			static uint64_t getSeqLen(std::vector<uint64_t> const & seqstarts, uint64_t const seqid)
			{
				uint64_t const seqid2 = seqid>>1;
				return seqstarts.at(2*seqid2+1)	- seqstarts.at(2*seqid2) - 1;
			}

			static void mergeKmers(std::vector<libmaus::alignment::KmerInfo> const & KV, std::vector < libmaus::fm::FactorMatchInfo > & FMI, uint64_t const k)
			{
				uint64_t low = 0;

				while ( low != KV.size() )
				{
					uint64_t high = low;
					
					while ( high != KV.size() && (KV[low].pos + (high-low) == KV[high].pos) )
						++high;
								
					FMI.push_back(libmaus::fm::FactorMatchInfo(KV[low].pos,KV[low].offset,k+(high-low)));
					
					low = high;
				}
			}
			
			public:
			SimpleLocalAligner(
				std::string const & rprefix, 
				std::string const & rsuffix, 
				unsigned int const rkmerlen,
				double const rerrrate,
				double const rhitfrac,
				uint64_t rmaxkmerfreqthres,
				uint64_t const cachelen = getDefaultCacheLen()
			)
			: prefix(rprefix), suffix(rsuffix), hwtname(prefix+suffix), kmerlen(rkmerlen), index(hwtname),
			  SA(*(index.SA)), PISA(libmaus::fm::SampledISA<lf_type>::load(index.LF.get(),prefix+".isa")),
			  ISA(*PISA), LF(*(index.LF)),
			  Pcache(libmaus::fm::KmerCache::construct<index_type,1,4>(index,cachelen)),
			  cache(*Pcache), seqstarts(index.getSeqStartPositions()),
			  mask(getMask(kmerlen)), indetmask(getIndetMask(kmerlen)),
			  errrate(rerrrate), hitfrac(rhitfrac),
			  fafaifile(prefix + ".fa.fai"),
			  Pfaindex(libmaus::fastx::FastAIndex::load(fafaifile)),
			  maxkmerfreqthres(rmaxkmerfreqthres)
			{
			
			}

			void align(
				std::string const & sid,
				std::string const & query,
				std::string const & queryquality,
				libmaus::autoarray::AutoArray<char> & mapped,
				std::vector<libmaus::alignment::BamLineInfo> & BLIs,
				double const minexactfrac = 0.05
			) const
			{
				BLIs.resize(0);

				uint64_t const m = query.size();
				
				// increase array size if necessary
				if ( m > mapped.size() )
					mapped = libmaus::autoarray::AutoArray<char>(m,false);
					
				// map query
				for ( uint64_t i = 0; i < m; ++i )
					mapped[i] = libmaus::fastx::mapChar(query[i])+1;

				// bit mask building
				libmaus::uint128_t v = 0;
				libmaus::uint128_t w = 0;
				std::vector<libmaus::alignment::KmerTuple> kmers;
				for ( uint64_t z = 0; z < std::min(kmerlen-1,m); ++z )
				{
					v <<= 3;
					v |= mapped[z];
					w <<= 3;
					w |= (mapped[z]-1);
				}
				// create kmer list (shifted and unshifted)
				for ( uint64_t z = std::min(kmerlen-1,m); z < m; ++z )
				{
					v <<= 3;
					v &= mask;
					v |= mapped[z];
					w <<= 3;
					w &= mask;
					assert ( mapped[z] >= 1 );
					assert ( mapped[z] <= 5 );
					assert ( mapped[z]-1 >= 0 );
					assert ( mapped[z]-1 <= 4 );
					w |= (mapped[z]-1);
					kmers.push_back(libmaus::alignment::KmerTuple(v,w,z-(kmerlen-1)));
				}
				// sort by kmer
				std::sort(kmers.begin(),kmers.end());
				// vector of repeating kmer positions
				std::vector<bool> Brepet(((m>=kmerlen) ? (m-kmerlen+1) : 0),false);
				{
					std::vector< uint64_t > repet;
					uint64_t low = 0;
					while ( low != kmers.size() )
					{
						uint64_t high = low;
						while ( high != kmers.size() && kmers[high].kmer == kmers[low].kmer )
							++high;
						
						if ( high-low > 1 || ((kmers[low].shiftkmer & indetmask) != 0) )
						{
							for ( uint64_t i = low; i < high; ++i )
								repet.push_back(kmers[i].pos);
						}
					
						low = high;
					}
					std::sort(repet.begin(),repet.end());
					for ( uint64_t i = 0; i < repet.size(); ++i )
						Brepet[repet[i]] = true;
				}
				
				std::vector < libmaus::alignment::KmerInfo > info;
				// maximum allowed kmer frequency
				uint64_t maxkmerfreq = 16;
				
				// look up non repetetive kmers
				while ( !info.size() )
				{
					// minimum frequency of a kmer in the query
					uint64_t minocckmerfreq = std::numeric_limits<uint64_t>::max();
					// number of kmers found
					uint64_t kmersfound = 0;
					
					libmaus::util::unordered_map<uint64_t,uint64_t>::type M;

					// look for non repetetive kmers, front to back
					for ( uint64_t z = 0; z < ((m>=kmerlen) ? (m-kmerlen+1) : 0); ++z )
						if ( !Brepet[z] )
						{
							#if 1
							libmaus::fm::BidirectionalIndexInterval bint = cache.lookup(index,mapped.begin()+z,kmerlen);
							#else
							libmaus::fm::BidirectionalIndexInterval bint = index.biSearchBackward(mapped.begin()+z,kmerlen);
							#endif
							
							
							// number of kmers found
							if ( bint.siz )
							{
								// update minimum freq of a kmer
								minocckmerfreq = std::min(bint.siz,minocckmerfreq);
								kmersfound++;
							}

							if ( bint.siz <= maxkmerfreq )
								for ( uint64_t i = 0; i < bint.siz; ++i )
								{
									// M[rank] -> index in info
									M[bint.spf+i] = info.size();
									// insert KmerInfo without position for offset z on query
									info.push_back(libmaus::alignment::KmerInfo(bint.spf+i,0,z));
								}
						}
					
					// look up positions for ranks
					uint64_t lookup_fast = 0, lookup_slow = 0;
					for ( uint64_t i = 0; i < info.size(); ++i )
					{
						// see if we have an entry for the previous position
						libmaus::util::unordered_map<uint64_t,uint64_t>::type::const_iterator ita =
							M.find ( LF ( info[i].rank ) );
						
						// if we have no rank for the previous position, then look up the position in the suffix array	
						if ( ita == M.end() )
						{
							info[i].pos = SA[info[i].rank];
							lookup_slow++;
						}
						// otherwise deduce value by LF mapping
						else
							info[i].pos = (info[ita->second].pos+1);
							lookup_fast++;
					}
					
					// std::cerr << "fast " << lookup_fast << " slow " << lookup_slow << std::endl;
					
					// no kmers kept?
					if ( ! info.size() )
					{
						// if any were found
						if ( kmersfound )
						{
							// then all should be too frequent
							if ( minocckmerfreq <= maxkmerfreq )
							{
								std::cerr << "minocckmerfreq=" << minocckmerfreq << " maxkmerfreq=" << maxkmerfreq << std::endl;
							}
							
							assert ( minocckmerfreq > maxkmerfreq );

							// std::cerr << "[E] all of " << kmersfound << " kmers too frequent in " << query << " minocc=" << minocckmerfreq << " thres=" << maxkmerfreq << std::endl;
								
							if ( minocckmerfreq <= maxkmerfreqthres )
							{
								maxkmerfreq = minocckmerfreq;
								// std::cerr << "[V] increased freq threshold to " << maxkmerfreq << std::endl;
							}
							else
							{
								break;
							}
						}
						else
						{
							// std::cerr << "[E] no kmers found in " << query << std::endl;
							break;			
						}
					}				
				}
				
				// sort kmers by offset on query
				std::sort(info.begin(),info.end());

				#if 0
				for ( uint64_t i = 0; i < info.size(); ++i )
					std::cerr << info[i] << std::endl;
				#endif
			
				std::vector < libmaus::fm::FactorMatchInfo > FMI;
				mergeKmers(info,FMI,kmerlen);

				// sort by position
				std::sort(FMI.begin(),FMI.end(),libmaus::alignment::FactorMatchInfoPosComparator());
				
				// compute intervals on FMI with strictly growing offset values
				// and (indel) error bounded by errrate
				std::vector < std::pair<uint64_t,uint64_t> > FMIintervals;
				uint64_t low = 0;
				while ( low != FMI.size() )
				{
					uint64_t high = low+1;
					while ( 
						high < FMI.size() && 
						FMI[high].offset > FMI[low].offset &&
						(FMI[high].pos + FMI[high].len - FMI[low].pos) <= m * (1.0 + errrate)
						// erate ( ( FMI[high].pos + FMI[high].len - FMI[low].pos ) , ( FMI[high].offset + FMI[high].len - FMI[low].offset ) ) <= errrate
					)
						++high;
					
					FMIintervals.push_back(std::pair<uint64_t,uint64_t>(low,high));
					
					low = high;
				}
				
				// sort by length (stretch of match along reference)
				std::sort(FMIintervals.begin(), FMIintervals.end(), libmaus::alignment::FMIIntervalComparator(FMI));

				// maximum score so far
				int64_t maxscore = -1;
				std::vector<int64_t> scores;

				std::pair<int64_t,int64_t> bestcoord(-1,-1);
				uint64_t maxkk = 0;
				
				for ( uint64_t z = 0; z < FMIintervals.size(); ++z )
				{
					#if 0
					std::cerr << "---" << std::endl;
					for ( uint64_t i = FMIintervals[z].first; i < FMIintervals[z].second; ++i )
					{
						std::cerr << FMI[i] << std::endl;
					}
					#endif
					
					uint64_t kk = 0;
					for ( uint64_t i = FMIintervals[z].first; i < FMIintervals[z].second; ++i )
						kk += FMI[i].len;
						
					maxkk = std::max(maxkk,kk);
					
					if ( kk <= (minexactfrac * m) )
						continue;
									
					// offset,len
					uint64_t const readstartoffset = FMI[FMIintervals[z].first].offset;
					uint64_t const readendoffset   = FMI[FMIintervals[z].second-1].offset + FMI[FMIintervals[z].second-1].len;

					// reference positions of frame matches
					uint64_t const r_refstartpos = FMI[FMIintervals[z].first].pos;
					uint64_t const r_refendpos = FMI[FMIintervals[z].second-1].pos + FMI[FMIintervals[z].second-1].len;
					
					// minimum number of indels necessary
					uint64_t const mindel = 
						std::max(r_refendpos-r_refstartpos,readendoffset-readstartoffset)
						-
						std::min(r_refendpos-r_refstartpos,readendoffset-readstartoffset)
						;
					
					// maximum available score
					int64_t const mscore = 
						static_cast<int64_t>(std::min(r_refendpos-r_refstartpos,readendoffset-readstartoffset))
						-
						static_cast<int64_t>(mindel);
					
					if ( maxscore >= 0 && (mscore <= hitfrac * maxscore) )
					{
						#if 0
						std::cerr << "[V] ignoring region, available score too low" << std::endl;
						#endif
						continue;
					}
					
					// maximum extension of read by error rate
					uint64_t const extend = static_cast<uint64_t> ( std::ceil ( m * errrate ) );
					uint64_t const readleftextend = readstartoffset + extend;
					uint64_t const readrightextend = (m - readendoffset) + extend;

					uint64_t const refstartpos = (r_refstartpos >= readleftextend) ? r_refstartpos - readleftextend : 0;
					uint64_t const refendpos   = r_refendpos + readrightextend;

					std::string const refpatch = index.getTextUnmapped(ISA[refstartpos],refendpos-refstartpos);

					#if 0
					std::cerr << refpatch << std::endl;
					std::cerr << query << std::endl;
					#endif

					uint64_t const maxerr = 
						std::max(
							(std::max(refpatch.size(),query.size())-std::min(refpatch.size(),query.size()))
							,
							static_cast<uint64_t>(std::floor(m * errrate+0.5))
						);

					#if 0
					std::cerr << sid 
						<< " [" << readstartoffset << "," << readendoffset << ")"
						<< " [" << refstartpos     << "," << refendpos << ")"
						<< " maxerr=" << maxerr
						<< std::endl;
						;
					#endif

					libmaus::lcs::MetaLocalEditDistance< ::libmaus::lcs::diag_del_ins > LED;
					libmaus::lcs::LocalEditDistanceResult LEDR = LED.process(
						refpatch.begin(),refpatch.size(),
						query.begin(),query.size(),
						maxerr
					);
					
					uint64_t const numops = LEDR.numins + LEDR.numdel + LEDR.nummat + LEDR.nummis;
					uint64_t const numerr = LEDR.numins + LEDR.numdel + LEDR.nummis;
					
					double const rate = static_cast<double>(numerr)/static_cast<double>(numops);
				
					if ( numerr <= maxerr && rate <= errrate )
					{
						#if 0
						if ( ! laligned )
							std::cerr << std::string(80,'-') << std::endl;
						#endif
					
						// reference sequence id
						uint64_t seq    = findSequence(seqstarts,refstartpos + LEDR.a_clip_left);
						// position on sequence
						uint64_t seqpos = (refstartpos + LEDR.a_clip_left) - seqstarts.at(seq);
						// length of mapping on reference
						uint64_t const reflen = LEDR.nummat + LEDR.nummis + LEDR.numdel;
						// is mapping on reverse complement of ref sequence?
						bool const rc = seq & 1;
						
						uint64_t const scfront = rc ? LEDR.b_clip_right : LEDR.b_clip_left;
						uint64_t const scback = rc ? LEDR.b_clip_left : LEDR.b_clip_right;

						#if 0
						std::cerr << "raw coord " << seq << "," << seqpos << std::endl;
						std::cerr << "control " << (seqstarts[seq] + seqpos) << " " << (refstartpos + LEDR.a_clip_left) << std::endl;
						#endif
						
						// transform coordinates to forward if mapping is on rc
						if ( rc )
						{
							// get length of reference sequence
							uint64_t const seqlen = getSeqLen(seqstarts,seq);
							// get end position of alignment on forward strand
							uint64_t const endpos = seqlen - seqpos - 1;
							// move to start position of alignment on forward strand
							seqpos = endpos - (reflen-1);
							// remove reverse complement marker from seq id
							seq = (seq >> 1) << 1;
						}
						
						#if 0
						std::cerr << "coord " << seq << "," << seqpos << std::endl;

						std::string testseq = index.getTextUnmapped(
							ISA[seqstarts[seq] + seqpos],reflen);
						
						if ( rc )
							testseq = libmaus::fastx::reverseComplementUnmapped(testseq);
						
						std::cerr << testseq << std::endl;
						#endif
						
						#if 0
						std::cerr << LEDR << std::endl;
						std::cerr << "rate=" << rate << std::endl;
						libmaus::lcs::LocalAlignmentPrint::printAlignmentLines(std::cerr,refpatch,query,80,LED.ta,LED.te,LEDR);
						#endif

						libmaus::lcs::LocalAlignmentTraceContainer const & LATC = LED.getTrace();
						libmaus::lcs::LocalAlignmentTraceContainer::step_type const * ta = LED.ta;
						libmaus::lcs::LocalAlignmentTraceContainer::step_type const * te = LED.te;
						
						std::vector<libmaus::lcs::LocalAlignmentTraceContainer::step_type> trace(ta,te);
						if ( rc )
							std::reverse(trace.begin(),trace.end());


						std::vector<libmaus::bambam::BamFlagBase::bam_cigar_ops> cigopvec;

						for ( uint64_t i = 0; i < scfront; ++i )
							cigopvec.push_back(libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CSOFT_CLIP);
						for ( uint64_t i = 0; i < trace.size(); ++i )
							switch ( trace[i] )
							{
								case libmaus::lcs::LocalBaseConstants::STEP_MATCH:
									cigopvec.push_back(libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CEQUAL);
									break;
								case libmaus::lcs::LocalBaseConstants::STEP_MISMATCH:
									cigopvec.push_back(libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CDIFF);
									break;
								case libmaus::lcs::LocalBaseConstants::STEP_INS:
									cigopvec.push_back(libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CINS);
									break;
								case libmaus::lcs::LocalBaseConstants::STEP_DEL:
									cigopvec.push_back(libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CDEL);
									break;
								default:
									break;							
							}
						for ( uint64_t i = 0; i < scback; ++i )
							cigopvec.push_back(libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_CSOFT_CLIP);
				
						std::vector<libmaus::bambam::cigar_operation> cigops;
						uint64_t ciglow = 0;
						while ( ciglow != cigopvec.size() )
						{
							uint64_t cighigh = ciglow;
							while ( cighigh != cigopvec.size() && cigopvec[cighigh] == cigopvec[ciglow] )
								++cighigh;
								
							cigops.push_back(libmaus::bambam::cigar_operation(cigopvec[ciglow],cighigh-ciglow));
								
							ciglow = cighigh;
						}

						int64_t const score = static_cast<int64_t>(LATC.getTraceScore());
							
						#if 0			
						std::cerr << "[V] " << Pfaindex->sequences.at(seq/2).name << ":" << seqpos << " score " << score << std::endl;
						#endif
										
						if ( score > maxscore )
							bestcoord = std::pair<int64_t,int64_t>(seq,seqpos);

						maxscore = std::max ( maxscore, score );
						scores.push_back(score);
						
						std::string bamseq = rc ? libmaus::fastx::reverseComplementUnmapped(query) : query;
						std::string bamqual = queryquality;
						if ( rc )
							std::reverse(bamqual.begin(),bamqual.end());

						BLIs.push_back(libmaus::alignment::BamLineInfo(
							score,sid,seq/2,seqpos,255 /* mapq */,
							rc ? libmaus::bambam::BamFlagBase::LIBMAUS_BAMBAM_FREVERSE : 0 /* flags */,
							cigops,-1,-1,rc ? static_cast<int32_t>(-m) : m,
							bamseq,bamqual,33
						));
					}
				}
				
				std::sort(BLIs.begin(),BLIs.end());
			}

			static std::string shortName(std::string const & s)
			{
				uint64_t q = s.size();
				for ( uint64_t i = 0; i < s.size(); ++i )
					if ( isspace( reinterpret_cast<unsigned char const *>(s.c_str())[i] ) )
					{
						q = i;
						break;
					}
					
				return s.substr(0,q);
			}
			
			::libmaus::bambam::BamHeader::unique_ptr_type getBamHeader(libmaus::util::ArgInfo const & arginfo, std::string const packageversion) const
			{
				::libmaus::bambam::BamHeader::unique_ptr_type Pbamheader(new ::libmaus::bambam::BamHeader);
				::libmaus::bambam::BamHeader & bamheader = *Pbamheader;
				std::ostringstream headerostr;
				headerostr << "@HD\tVN:1.4\tSO:unknown\n";
				headerostr 
					<< "@PG"<< "\t" 
					<< "ID:" << "tipi" << "\t" 
					<< "PN:" << "tipi" << "\t"
					<< "CL:" << arginfo.commandline << "\t"
					<< "VN:" << std::string(packageversion)
					<< std::endl;
					
				for ( uint64_t i = 0; i < Pfaindex->sequences.size(); ++i )
				{
					headerostr << "@SQ\tSN:" << shortName(Pfaindex->sequences[i].name) << "\tLN:" << Pfaindex->sequences[i].length << "\n";
					bamheader.addChromosome(shortName(Pfaindex->sequences[i].name),Pfaindex->sequences[i].length);
				}

				bamheader.text = headerostr.str();		
			
				return UNIQUE_PTR_MOVE(Pbamheader);
			}
		};
	}
}
#else
#error "libmaus::alignment::SimpleLocalAligner<> requires 128 bit integer support which is not present on this system"
#endif

#endif
