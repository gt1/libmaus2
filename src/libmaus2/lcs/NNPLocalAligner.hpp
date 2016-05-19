/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_LCS_NNPLOCALALIGNER_HPP)
#define LIBMAUS2_LCS_NNPLOCALALIGNER_HPP

#include <libmaus2/lcs/NNP.hpp>
#include <libmaus2/util/FiniteSizeHeap.hpp>
#include <libmaus2/util/GrowingFreeList.hpp>
#include <libmaus2/geometry/RangeSet.hpp>

namespace libmaus2
{
	namespace lcs
	{
		/**
		 * local aligner based on NNP class.
		 **/
		struct NNPLocalAligner
		{
			typedef NNPLocalAligner this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			struct NNPLocalAlignerKmer
			{
				uint64_t kmer;
				uint64_t pos;

				NNPLocalAlignerKmer() {}
				NNPLocalAlignerKmer(
					uint64_t const rkmer,
					uint64_t const rpos
				) : kmer(rkmer), pos(rpos) {}

				bool operator<(NNPLocalAlignerKmer const & O) const
				{
					if ( kmer != O.kmer )
						return kmer < O.kmer;
					else
						return pos < O.pos;
				}
			};

			struct NNPLocalAlignerKmerMatches
			{
				uint64_t apos;
				uint64_t bpos;
				int64_t score;
				NNPLocalAlignerKmerMatches * next;

				NNPLocalAlignerKmerMatches() {}
				NNPLocalAlignerKmerMatches(
					uint64_t const rapos, uint64_t rbpos
				) : apos(rapos), bpos(rbpos), score(0), next(0) {}

				int64_t getAntiDiag() const
				{
					return static_cast<int64_t>(apos) + static_cast<int64_t>(bpos);
				}

				int64_t getDiag() const
				{
					return static_cast<int64_t>(apos) - static_cast<int64_t>(bpos);
				}

				bool operator<(NNPLocalAlignerKmerMatches const & O) const
				{
					return getAntiDiag() < O.getAntiDiag();
				}
			};

			struct NNPLocalAlignerKmerMatchesHeapComparator
			{
				bool operator()(NNPLocalAlignerKmerMatches const * A, NNPLocalAlignerKmerMatches const * B) const
				{
					return A->score > B->score;
				}
			};


			// log_2 of bucket size
			unsigned int const bucketlog;
			// analysis kmer size
			unsigned int const anak;
			// analysis kmer mask
			uint64_t const anakmask;

			// last position for skipping
			libmaus2::autoarray::AutoArray < uint64_t > Alasta;
			// score per band
			libmaus2::autoarray::AutoArray < uint64_t > Alastscore;
			// next hit pointer for band
			libmaus2::autoarray::AutoArray < NNPLocalAlignerKmerMatches * > Alastnext;
			// last position for scoring
			libmaus2::autoarray::AutoArray < int64_t > Alastp;

			// maximum number of matches to be stored/computed
			uint64_t const maxmatches;
			// kmer matches between A and B sequence
			libmaus2::autoarray::AutoArray < NNPLocalAlignerKmerMatches > Amatches;

			// list of extracted kmers
			libmaus2::autoarray::AutoArray < NNPLocalAlignerKmer > Akmers;

			// kmer frequency histogram
			uint64_t const histlow;
			libmaus2::autoarray::AutoArray<uint64_t> Ahistlow;

			// minimum score for a band to be considred
			int64_t minbandscore;

			// minimum alignment length on A read
			int64_t minlength;

			// aligner object
			libmaus2::lcs::NNP nnp;
			// trace container
			libmaus2::lcs::NNPTraceContainer tracecontainer;
			// alignment result
			libmaus2::lcs::NNPAlignResult algnres;

			// priority queue
			libmaus2::util::FiniteSizeHeap<NNPLocalAlignerKmerMatches *,NNPLocalAlignerKmerMatchesHeapComparator> Q;

			private:
			// free list for trace objects
			libmaus2::util::GrowingFreeList < libmaus2::lcs::NNPTraceContainer, libmaus2::lcs::NNPTraceContainerAllocator, libmaus2::lcs::NNPTraceContainerTypeInfo > tracefreelist;

			public:
			uint64_t alloccount;

			libmaus2::lcs::AlignmentTraceContainer ATC0;
			libmaus2::lcs::AlignmentTraceContainer ATC1;
			libmaus2::lcs::AlignmentTraceContainer ATCc;

			libmaus2::autoarray::AutoArray< libmaus2::lcs::NNPTraceContainer::TracePointId > AT;
			libmaus2::autoarray::AutoArray< uint64_t > Ashare;
			libmaus2::autoarray::AutoArray< uint8_t > Amodified;

			/**
			 * local aligner constructor
			 *
			 * rbucketlog log width of score bucket
			 * ranak analysis kmer length
			 * rmaxmatches maximum number of matches computed
			 * rminbandscore minimum score required for a band to be considered
			 **/
			NNPLocalAligner(
				unsigned int const rbucketlog,
				unsigned int const ranak,
				uint64_t const rmaxmatches,
				int64_t const rminbandscore,
				int64_t const rminlength
			)
			:
				bucketlog(rbucketlog),
				anak(ranak),
				anakmask(libmaus2::math::lowbits(2*anak)),
				maxmatches(rmaxmatches),
				Amatches(maxmatches),
				histlow(8*1024),
				Ahistlow(histlow+1),
				minbandscore(rminbandscore),
				minlength(rminlength),
				Q(1024),
				alloccount(0)
			{
				assert ( anak );
			}

			template<typename iterator>
			static NNPLocalAlignerKmer * extractKmers(
				iterator aa, iterator ae,
				NNPLocalAlignerKmer * op,
				unsigned int const anak,
				uint64_t const anakmask,
				int64_t const plow,
				int64_t const phigh
			)
			{
				// if string is long enough
				if ( ae-aa >= anak )
				{
					// start of query
					iterator const as = aa;

					// start of window pointer
					iterator ac = aa;

					// end of first k+1 mer
					iterator at = aa + (anak-1);

					// current value
					uint64_t v = 0;
					// number of errors
					unsigned int e = 0;
					// process first k-1 symbols
					while ( aa != at )
					{
						int64_t const sym = *(aa++);
						v <<= 2;
						v |= (sym&3);
						e += (sym>3);
					}

					// process rest of symbols
					while ( aa != ae )
					{
						int64_t const sym = *(aa++);
						v <<= 2;
						v &= anakmask;
						v |= (sym&3);
						e += (sym>3);

						if ( ! e )
						{
							int64_t const p = (aa-anak)-as;
							if ( p >= plow && p+anak <= phigh )
								*(op++) = NNPLocalAlignerKmer(v,p);
						}

						// update error count at front of current window
						int64_t const tsym = *(ac++);
						e -= (tsym>3);
					}
				}

				return op;
			}

			template<typename iterator>
			static void extractKmers(
				iterator aa, iterator ae,
				int64_t const alow, int64_t const ahigh,
				iterator ba, iterator be,
				int64_t const blow, int64_t const bhigh,
				unsigned int const anak,
				uint64_t const anakmask,
				libmaus2::autoarray::AutoArray < NNPLocalAlignerKmer > & Akmers,
				uint64_t const histlow,
				libmaus2::autoarray::AutoArray<uint64_t> & Ahistlow,
				uint64_t const maxmatches,
				NNPLocalAlignerKmerMatches * & m_e
			)
			{
				uint64_t const n = ae-aa;
				uint64_t const m = be-ba;

				// number of kmers in a
				uint64_t const ka = (n >= anak) ? (n-anak+1) : 0;
				// number of kmers in b
				uint64_t const kb = (m >= anak) ? (m-anak+1) : 0;
				// sum
				uint64_t const ks = ka + kb;

				Akmers.ensureSize(ks);

				NNPLocalAlignerKmer * op_a = Akmers.begin();
				NNPLocalAlignerKmer * op_b = extractKmers(aa,ae,op_a,anak,anakmask,alow,ahigh);
				std::sort(op_a,op_b);

				NNPLocalAlignerKmer * op_e = extractKmers(ba,be,op_b,anak,anakmask,blow,bhigh);
				std::sort(op_b,op_e);

				bool const self = (aa == ba) && (ae == be);

				// fill match freq histogram
				NNPLocalAlignerKmer * c_a = op_a;
				NNPLocalAlignerKmer * c_b = op_b;
				while ( c_a != op_b && c_b != op_e )
				{
					if ( c_a->kmer < c_b->kmer )
						++c_a;
					else if ( c_b->kmer < c_a->kmer )
						++c_b;
					else
					{
						#if 0
						assert ( c_a->kmer == c_b->kmer );
						#endif
						NNPLocalAlignerKmer * e_a = c_a+1;
						NNPLocalAlignerKmer * e_b = c_b+1;
						while ( e_a != op_b && e_a->kmer == c_a->kmer )
							++e_a;
						while ( e_b != op_e && e_b->kmer == c_b->kmer )
							++e_b;

						#if 0
						for ( NNPLocalAlignerKmer * i_a = c_a; i_a != e_a; ++i_a )
							assert ( i_a->kmer == c_a->kmer );
						for ( NNPLocalAlignerKmer * i_b = c_b; i_b != e_b; ++i_b )
							assert ( i_b->kmer == c_b->kmer );

						assert ( e_a == op_b || e_a->kmer != c_a->kmer );
						assert ( e_b == op_e || e_b->kmer != c_b->kmer );
						#endif

						if ( self )
						{
							#if 0
							assert ( e_a-c_a == e_b-c_b );
							#endif

							uint64_t const afreq = (e_a-c_a);
							uint64_t const freq = (afreq*(afreq-1))/2;

							if ( freq < histlow )
								Ahistlow[freq] += 1;
							else
								Ahistlow[histlow] += freq;
						}
						else
						{
							uint64_t const a_freq = (e_a-c_a);
							uint64_t const b_freq = (e_b-c_b);
							uint64_t const freq = a_freq*b_freq;

							if ( freq < histlow )
								Ahistlow[freq] += 1;
							else
								Ahistlow[histlow] += freq;
						}

						c_a = e_a;
						c_b = e_b;
					}
				}

				uint64_t maxf = 0;
				// compute how many matches we can store
				uint64_t matches = 0;
				while ( maxf < histlow && (matches + maxf * Ahistlow[maxf] <= maxmatches) )
				{
					matches += maxf * Ahistlow[maxf];
					maxf += 1;
				}
				// see if we can also get the "rest" bucket
				if ( maxf == histlow && matches + Ahistlow[histlow] <= maxmatches )
				{
					matches += Ahistlow[histlow];
					maxf = std::numeric_limits<uint64_t>::max();
				}

				// extract matches
				c_a = op_a;
				c_b = op_b;
				while ( c_a != op_b && c_b != op_e )
				{
					if ( c_a->kmer < c_b->kmer )
						++c_a;
					else if ( c_b->kmer < c_a->kmer )
						++c_b;
					else
					{
						#if 0
						assert ( c_a->kmer == c_b->kmer );
						#endif

						NNPLocalAlignerKmer * e_a = c_a+1;
						NNPLocalAlignerKmer * e_b = c_b+1;
						while ( e_a != op_b && e_a->kmer == c_a->kmer )
							++e_a;
						while ( e_b != op_e && e_b->kmer == c_b->kmer )
							++e_b;

						#if 0
						for ( NNPLocalAlignerKmer * i_a = c_a; i_a != e_a; ++i_a )
							assert ( i_a->kmer == c_a->kmer );
						for ( NNPLocalAlignerKmer * i_b = c_b; i_b != e_b; ++i_b )
							assert ( i_b->kmer == c_b->kmer );
						assert ( e_a == op_b || e_a->kmer != c_a->kmer );
						assert ( e_b == op_e || e_b->kmer != c_b->kmer );
						#endif

						if ( self )
						{
							#if 0
							assert ( e_a-c_a == e_b-c_b );
							#endif

							uint64_t const afreq = (e_a-c_a);
							uint64_t const freq = (afreq*(afreq-1))/2;

							if ( freq < maxf )
								for ( NNPLocalAlignerKmer * i_a = c_a; i_a != e_a; ++i_a )
									for ( NNPLocalAlignerKmer * i_b = i_a+1; i_b != e_a; ++i_b )
										*(m_e++) = NNPLocalAlignerKmerMatches(i_a->pos,i_b->pos);
						}
						else
						{
							uint64_t const a_freq = (e_a-c_a);
							uint64_t const b_freq = (e_b-c_b);
							uint64_t const freq = a_freq*b_freq;

							if ( freq < maxf )
								for ( NNPLocalAlignerKmer * i_a = c_a; i_a != e_a; ++i_a )
									for ( NNPLocalAlignerKmer * i_b = c_b; i_b != e_b; ++i_b )
										*(m_e++) = NNPLocalAlignerKmerMatches(i_a->pos,i_b->pos);
						}

						c_a = e_a;
						c_b = e_b;
					}
				}

				// clear histogram
				clearHistogram(aa,ae,ba,be,op_a,op_b,op_e,histlow,Ahistlow);
			}

			template<typename iterator>
			static void clearHistogram(
				iterator aa, iterator ae,
				iterator ba, iterator be,
				NNPLocalAlignerKmer * op_a,
				NNPLocalAlignerKmer * op_b,
				NNPLocalAlignerKmer * op_e,
				uint64_t const histlow,
				libmaus2::autoarray::AutoArray<uint64_t> & Ahistlow
			)
			{
				bool const self = (aa == ba) && (ae == be);

				// reset kmer freq histogram
				NNPLocalAlignerKmer * c_a = op_a;
				NNPLocalAlignerKmer * c_b = op_b;
				while ( c_a != op_b && c_b != op_e )
				{
					if ( c_a->kmer < c_b->kmer )
						++c_a;
					else if ( c_b->kmer < c_a->kmer )
						++c_b;
					else
					{
						#if 0
						assert ( c_a->kmer == c_b->kmer );
						#endif
						NNPLocalAlignerKmer * e_a = c_a+1;
						NNPLocalAlignerKmer * e_b = c_b+1;
						while ( e_a != op_b && e_a->kmer == c_a->kmer )
							++e_a;
						while ( e_b != op_e && e_b->kmer == c_b->kmer )
							++e_b;

						#if 0
						for ( NNPLocalAlignerKmer * i_a = c_a; i_a != e_a; ++i_a )
							assert ( i_a->kmer == c_a->kmer );
						for ( NNPLocalAlignerKmer * i_b = c_b; i_b != e_b; ++i_b )
							assert ( i_b->kmer == c_b->kmer );
						assert ( e_a == op_b || e_a->kmer != c_a->kmer );
						assert ( e_b == op_e || e_b->kmer != c_b->kmer );
						#endif

						if ( self )
						{
							#if 0
							assert ( e_a-c_a == e_b-c_b );
							#endif

							uint64_t const afreq = (e_a-c_a);
							uint64_t const freq = (afreq*(afreq-1))/2;

							if ( freq < histlow )
								Ahistlow[freq] = 0;
							else
								Ahistlow[histlow] = 0;
						}
						else
						{
							uint64_t const a_freq = (e_a-c_a);
							uint64_t const b_freq = (e_b-c_b);
							uint64_t const freq = a_freq*b_freq;

							if ( freq < histlow )
								Ahistlow[freq] = 0;
							else
								Ahistlow[histlow] = 0;
						}

						c_a = e_a;
						c_b = e_b;
					}
				}
			}

			template<typename iterator>
			std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> fuseAlignments(
				std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> P0,
				std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> Pi,
				iterator a,
				iterator b
			)
			{
				libmaus2::lcs::NNPAlignResult algnres0 = P0.first;
				libmaus2::lcs::NNPAlignResult algnresi = Pi.first;

				// decode traces
				P0.second->computeTrace(ATC0);
				Pi.second->computeTrace(ATC1);

				// check decoded traces
				assert (libmaus2::lcs::AlignmentTraceContainer::checkAlignment(ATC0.ta, ATC0.te,a + P0.first.abpos,b + P0.first.bbpos));
				assert (libmaus2::lcs::AlignmentTraceContainer::checkAlignment(ATC1.ta, ATC1.te,a + Pi.first.abpos,b + Pi.first.bbpos));

				// get crossing point
				uint64_t offa = 0, offb = 0;
				bool const ok = libmaus2::lcs::AlignmentTraceContainer::cross(ATC0,algnres0.abpos,algnres0.bbpos,offa,ATC1,algnresi.abpos,algnresi.bbpos,offb);

				// make sure there is one
				assert ( ok );

				// check length is consistent
				assert (
					P0.first.abpos + libmaus2::lcs::AlignmentTraceContainer::getStringLengthUsed(ATC0.ta,ATC0.ta+offa).first ==
					Pi.first.abpos + libmaus2::lcs::AlignmentTraceContainer::getStringLengthUsed(ATC1.ta,ATC1.ta+offb).first
				);
				assert (
					P0.first.bbpos + libmaus2::lcs::AlignmentTraceContainer::getStringLengthUsed(ATC0.ta,ATC0.ta+offa).second ==
					Pi.first.bbpos + libmaus2::lcs::AlignmentTraceContainer::getStringLengthUsed(ATC1.ta,ATC1.ta+offb).second
				);

				ATC0.te = ATC0.ta + offa;
				ATC1.ta += offb;

				ATCc.reset();
				ATCc.push(ATC0);
				assert ( std::equal(ATC0.ta,ATC0.ta + offa, ATCc.ta) );
				ATCc.push(ATC1);

				ATC1.ta -= offb;

				assert ( std::equal(ATC0.ta,ATC0.ta + offa, ATCc.ta) );
				assert ( std::equal(ATC1.ta+offb,ATC1.te, ATCc.ta + offa) );

				// check fused alignment
				assert ( libmaus2::lcs::AlignmentTraceContainer::checkAlignment(ATCc.ta, ATCc.te,a + P0.first.abpos,b + P0.first.bbpos) );

				// return previous trace to free list
				returnAlignment(P0);
				returnAlignment(Pi);

				// encode fused alignment
				P0.second = getAlignment();
				P0.second->traceToSparse(ATCc);

				// check sparse encoded alignment
				assert ( P0.second->checkTrace(a + algnres0.abpos, b + algnres0.bbpos) );

				// check consistency
				std::pair<uint64_t,uint64_t> SLA = P0.second->getStringLengthUsed();
				std::pair<uint64_t,uint64_t> SLB = ATCc.getStringLengthUsed();
				assert ( SLA == SLB );

				// set new meta data
				algnres0.aepos = algnres0.abpos + SLA.first;
				algnres0.bepos = algnres0.bbpos + SLA.second;
				algnres0.dif = P0.second->getNumDif();

				P0.first = algnres0;
				assert ( P0.second->checkTrace(a + P0.first.abpos, b + P0.first.bbpos) );

				return P0;
			}

			template<typename iterator>
			std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> subsumeAlignments(
				std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> P0,
				std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> Pi,
				iterator /* a */,
				iterator /* b */
			)
			{
				returnAlignment(Pi.second);
				return P0;
			}

			template<typename iterator>
			std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> joinAlignments(
				std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> P0,
				std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> Pi,
				iterator a,
				iterator b
			)
			{
				assert ( P0.second->checkTrace(a + P0.first.abpos, b + P0.first.bbpos) );
				assert ( Pi.second->checkTrace(a + Pi.first.abpos, b + Pi.first.bbpos) );

				// make sure P0 is the "left" alignment
				if ( Pi.first.abpos < P0.first.abpos )
					std::swap(P0,Pi);
				assert ( P0.first.abpos <= Pi.first.abpos );

				libmaus2::lcs::NNPAlignResult algnres0 = P0.first;
				libmaus2::lcs::NNPTraceContainer::shared_ptr_type trace0 = P0.second;

				libmaus2::lcs::NNPAlignResult algnresi = Pi.first;
				libmaus2::lcs::NNPTraceContainer::shared_ptr_type tracei = Pi.second;

				bool const ok = libmaus2::lcs::NNPTraceContainer::cross(*trace0,algnres0.abpos,algnres0.bbpos,*tracei,algnresi.abpos,algnresi.bbpos);
				assert ( ok );

				if ( algnres0.aepos >= algnresi.aepos )
				{
					P0 = subsumeAlignments(P0,Pi,a,b);
					assert ( P0.second->checkTrace(a + P0.first.abpos, b + P0.first.bbpos) );
				}
				else
				{
					P0 = fuseAlignments(P0,Pi,a,b);
					assert ( P0.second->checkTrace(a + P0.first.abpos, b + P0.first.bbpos) );
				}

				return P0;
			}

			// filter alignments (remove contained overlapping alignments and fuse overlapping alignments)
			template<typename iterator>
			void filterAlignments(
				std::vector < std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> > & LLV,
				iterator a,
				iterator b
			)
			{
				bool changed = true;
				while ( changed )
				{
					changed = false;

					// compute common trace points
					uint64_t const ashareo = libmaus2::lcs::NNPTraceContainer::getCommonTracePoints(LLV.begin(),LLV.end(),AT,Ashare,64);
					assert ( ashareo == LLV.size() );
					Amodified.ensureSize(ashareo);
					std::fill(Amodified.begin(),Amodified.begin()+ashareo,false);

					for ( uint64_t i = 0; i < ashareo; ++i )
						if ( Ashare[i] != i && !Amodified[     i  ] && !Amodified[ Ashare[i] ] )
						{
							uint64_t const partner = Ashare[i];
							Amodified[i] = Amodified[partner] = true;
							LLV[i] = joinAlignments(LLV[i],LLV[partner],a,b);
							LLV[partner] = std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type>();
						}

					// filter out removed alignments
					uint64_t o = 0;
					for ( uint64_t i = 0; i < LLV.size(); ++i )
						if ( LLV[i].second )
							LLV[o++] = LLV[i];
						else
							changed = true;
					LLV.resize(o);
				}
			}

			// filter alignments (remove contained overlapping alignments and fuse overlapping alignments)
			template<typename iterator>
			void filterAlignmentsStrict(
				std::vector < std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> > & LLV,
				iterator a,
				iterator b
			)
			{
				bool running = true;

				while ( running )
				{
					running = false;

					std::vector< std::pair < std::pair<int64_t,int64_t>, uint64_t > > diagbands(LLV.size());
					for ( uint64_t i = 0; i < LLV.size(); ++i )
						diagbands[i] =
							std::pair < std::pair<int64_t,int64_t>, uint64_t >(
								LLV[i].second->getDiagonalBand(LLV[i].first.abpos,LLV[i].first.bbpos),
								i
							);

					std::sort(diagbands.begin(),diagbands.end());

					uint64_t low = 0;
					while ( (!running) && (low < diagbands.size()) )
					{
						uint64_t high = low+1;
						while (
							high < diagbands.size() &&
							diagbands[high-1].first.second >= diagbands[high].first.first
						)
							++high;

						if ( high-low > 1 )
						{
							std::vector< std::pair < std::pair<int64_t,int64_t>, uint64_t > > antidiagbands(high-low);

							for ( uint64_t j = low; j < high; ++j )
							{
								uint64_t const i = diagbands[j].second;
								antidiagbands[j-low] = std::pair < std::pair<int64_t,int64_t>, uint64_t >(
									LLV[i].second->getAntiDiagonalBand(LLV[i].first.abpos,LLV[i].first.bbpos),i
								);

							}
							std::sort(antidiagbands.begin(),antidiagbands.end());

							uint64_t alow = 0;
							while ( (!running) && alow < antidiagbands.size() )
							{
								uint64_t ahigh = alow+1;
								while (
									ahigh < antidiagbands.size() &&
									antidiagbands[ahigh-1].first.second >= antidiagbands[ahigh].first.first
								)
									++ahigh;

								if ( ahigh-alow > 1 )
								{
									for ( uint64_t i = alow; (!running) && i < ahigh; ++i )
										for ( uint64_t j = i+1; (!running) && j < ahigh; ++j )
										{
											std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> P0 =
												LLV[ antidiagbands[i].second ];
											std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> Pi =
												LLV[ antidiagbands[j].second ];
											bool const cross = libmaus2::lcs::NNPTraceContainer::cross(
												*(P0.second),P0.first.abpos,P0.first.bbpos,
												*(Pi.second),Pi.first.abpos,Pi.first.bbpos
											);
											if ( cross )
											{
												#if 0
												std::cerr << "*" << LLV[antidiagbands[i].second].first << std::endl;
												std::cerr << "*" << LLV[antidiagbands[j].second].first << std::endl;
												#endif

												LLV [ antidiagbands[i].second ] = joinAlignments(P0,Pi,a,b);
												LLV [ antidiagbands[j].second ] = std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type>();
												running = true;
											}
										}
								}

								alow = ahigh;
							}
						}

						low = high;
					}

					if ( running )
					{
						uint64_t o = 0;
						for ( uint64_t i = 0; i < LLV.size(); ++i )
							if ( LLV[i].second )
								LLV[o++] = LLV[i];
						assert ( o < LLV.size() );
						LLV.resize(o);
					}
				}
			}

			template<typename iterator>
			static bool isOverlapping(iterator aa, iterator ae, iterator ba, iterator be)
			{
				if ( aa > ba )
				{
					std::swap(aa,ba);
					std::swap(ae,be);
				}

				assert ( aa <= ba );

				return (ba < ae);
			}

			libmaus2::lcs::NNPTraceContainer::shared_ptr_type getAlignment()
			{
				alloccount += 1;
				return tracefreelist.get();
			}

			void returnAlignment(libmaus2::lcs::NNPTraceContainer::shared_ptr_type P)
			{
				tracefreelist.put(P);
				alloccount -= 1;
			}

			void returnAlignment(std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> P)
			{
				returnAlignment(P.second);
			}

			void returnAlignments(std::vector< std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> > & V)
			{
				for ( uint64_t i = 0; i < V.size(); ++i )
					returnAlignment(V[i]);
			}

			/**
			 * align the sequences given by [aa,ae) and [ba,be). Returns a set of alignments.
			 * The alignments returned need to be freed using the returnAlignments method.
			 **/
			template<typename iterator>
			std::vector< std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> >
				align(
					iterator aa, iterator ae, iterator ba, iterator be,
					int64_t const aseedlow = std::numeric_limits<int64_t>::min(),
					int64_t const aseedhigh = std::numeric_limits<int64_t>::max(),
					int64_t const bseedlow = std::numeric_limits<int64_t>::min(),
					int64_t const bseedhigh = std::numeric_limits<int64_t>::max()
				)
			{
				uint64_t const alloccountbef = alloccount;

				bool const overlap = isOverlapping(aa,ae,ba,be);

				uint64_t const n = ae-aa;
				uint64_t const m = be-ba;

				NNPLocalAlignerKmerMatches * m_a = Amatches.begin();
				NNPLocalAlignerKmerMatches * m_e = m_a;

				// extract matches
				extractKmers(aa,ae,aseedlow,aseedhigh,ba,be,bseedlow,bseedhigh,anak,anakmask,Akmers,histlow,Ahistlow,maxmatches,m_e);

				// sort seeds by anti diagonal
				std::sort(m_a,m_e);

				// filter seeds if a and b are on the same sequence
				if ( overlap )
				{
					NNPLocalAlignerKmerMatches * n_m_e = m_a;
					for ( NNPLocalAlignerKmerMatches * m_c = m_a; m_c != m_e; ++m_c )
						if ( aa+m_c->apos != ba+m_c->bpos )
							*(n_m_e++) = *m_c;
					m_e = n_m_e;
				}

				bool const self = (aa == ba) && (ae == be);

				int64_t const mindiag = -static_cast<int64_t>(m);
				int64_t const maxdiag = static_cast<int64_t>(n);

				// minimum bucket
				int64_t const minbucket = mindiag >> bucketlog;
				// maximum bucket
				int64_t const maxbucket = maxdiag >> bucketlog;
				int64_t const allocbuckets = (maxbucket - minbucket + 1) + 2;

				NNPLocalAlignerKmerMatches * const lastnextnull = 0;

				if ( allocbuckets > Alasta.size() )
				{
					uint64_t const oldsize = Alasta.size();

					Alasta.ensureSize(allocbuckets);
					Alastp.ensureSize(allocbuckets);
					Alastscore.ensureSize(allocbuckets);
					Alastnext.ensureSize(allocbuckets);

					uint64_t const newsize = Alasta.size();

					std::fill(Alasta.begin() + oldsize,     Alasta.begin() + newsize    , 0                            );
					std::fill(Alastp.begin() + oldsize,     Alastp.begin() + newsize    , -static_cast<int64_t>(2*anak));
					std::fill(Alastscore.begin() + oldsize, Alastscore.begin() + newsize, 0);
					std::fill(Alastnext.begin() + oldsize,  Alastnext.begin()+newsize   , lastnextnull);
				}

				#if 0
				for ( uint64_t i = 0; i < Ahistlow.size(); ++i )
					assert ( Ahistlow[i] == 0 );

				for ( uint64_t i = 0; i < allocbuckets; ++i )
				{
					assert ( Alasta[i] == 0 );
					assert ( Alastp[i] == -static_cast<int64_t>(2*anak) );
					assert ( Alastscore[i] ==  0 );
					assert ( Alastnext[i] == lastnextnull );
				}
				#endif

				uint64_t * const lasta = Alasta.begin() - (minbucket-1);
				// previous position in diagonal band
				int64_t * const lastp = Alastp.begin() - (minbucket-1);
				uint64_t * const lastscore = Alastscore.begin() - (minbucket-1);
				NNPLocalAlignerKmerMatches ** const lastnext = Alastnext.begin() - (minbucket-1);

				#if 0
				// check seeds
				for ( NNPLocalAlignerKmerMatches * m_c = m_a; m_c != m_e; ++m_c )
					assert ( std::equal( aa + m_c->apos, aa + m_c->apos + anak, ba + m_c->bpos ) );
				#endif

				// compute single seed scores
				for ( NNPLocalAlignerKmerMatches * m_c = m_a; m_c != m_e; ++m_c )
				{
					NNPLocalAlignerKmerMatches const & cur = *m_c;
					int64_t const bucket = cur.getDiag() >> bucketlog;
					int64_t const anti = cur.getAntiDiag();

					#if 0
					assert ( anti >= lastp[bucket] );
					#endif

					// available score taking last hit into account (divide by 2 because we operate on anti diagonals)
					int64_t const av    = (anti - lastp[bucket]) >> 1;
					// actual kmer score
					int64_t const score = std::min(av,static_cast<int64_t>(anak));

					#if 0
					assert ( score >= 0 );
					#endif

					m_c->score = score;

					lastp[bucket] = anti;
				}

				// traverse in decreasing anti diag order
				uint64_t activebands = 0;
				for ( NNPLocalAlignerKmerMatches * m_c = m_e; m_c != m_a; )
				{
					NNPLocalAlignerKmerMatches & cur = *(--m_c);
					int64_t const bucket = cur.getDiag() >> bucketlog;

					lastscore[bucket] += m_c->score;
					m_c->score = lastscore[bucket-1] + lastscore[bucket] + lastscore[bucket+1];
					m_c->next = lastnext[bucket];

					if ( (! lastnext[bucket]) && m_c->score >= minbandscore )
						++activebands;

					lastnext[bucket] = &cur;
				}

				Q.ensureSize(activebands);

				// fill queue
				for ( NNPLocalAlignerKmerMatches * m_c = m_a; m_c != m_e; ++m_c )
				{
					NNPLocalAlignerKmerMatches const & cur = *m_c;
					int64_t const bucket = cur.getDiag() >> bucketlog;

					if ( lastnext[bucket] )
					{
						if ( lastnext[bucket]->score >= minbandscore )
						{
							assert ( ! Q.full() );
							Q.push(lastnext[bucket]);
						}
						lastnext[bucket] = lastnextnull;
					}
				}

				#if 0
				// check next sorting
				for ( NNPLocalAlignerKmerMatches * m_c = m_a; m_c != m_e; ++m_c )
					if ( m_c->next )
						 assert ( m_c->next->getAntiDiag() >= m_c->getAntiDiag() );
				#endif

				std::vector < std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type> > VR;

				// min and max diagonal bands used
				int64_t ldbmin = std::numeric_limits<int64_t>::max();
				int64_t ldbmax = std::numeric_limits<int64_t>::min();

				// process suspected hit bands
				while ( ! Q.empty() )
				{
					NNPLocalAlignerKmerMatches * p = Q.pop();

					// diagonal bucket
					int64_t const bucket = p->getDiag() >> bucketlog;

					// if hit is superseded by a previous one
					if ( p->getAntiDiag() < lasta[bucket] )
					{
						// skip
						while ( p && p->getAntiDiag() < lasta[bucket] )
							p = p->next;
						// reque if score is still sufficienlty high
						if ( p /* && p->score >= minbandscore */ )
							Q.push(p);
					}
					else
					{
						// push next in band if score still sufficiently high
						if ( p->next /* && p->next->score >= minbandscore */ )
							Q.push(p->next);

						#if 0
						assert ( p->getAntiDiag() >= lasta[bucket] );
						assert ( p->getDiag() >> bucketlog == bucket );
						assert ( p->score >= minbandscore );
						#endif

						uint64_t const apos = p->apos;
						uint64_t const bpos = p->bpos;

						#if 0
						// check seed
						assert ( std::equal( aa + apos, aa + apos + anak, ba + bpos ) );
						#endif

						// run alignment
						if ( self )
						{
							assert ( apos < bpos );
							uint64_t const pdif = bpos-apos;
							algnres = nnp.align(aa,ae,apos,ba,be,bpos,tracecontainer,true, /* self check */-static_cast<int64_t>(pdif/2),static_cast<int64_t>(pdif/2));
						}
						else
						{
							algnres = nnp.align(aa,ae,apos,ba,be,bpos,tracecontainer,overlap /* self check */);
						}

						// check alignment
						assert ( tracecontainer.checkTrace(aa+algnres.abpos,ba+algnres.bbpos) );

						// std::cerr << algnres << " " << n << " " << m  << std::endl;

						if ( algnres.aepos - algnres.abpos >= minlength )
						{
							std::pair<int64_t,int64_t> DB = tracecontainer.getDiagonalBand(apos,bpos);
							DB.first >>= bucketlog;
							DB.second >>= bucketlog;

							// set end of alignment for bands
							for ( int64_t db = DB.first; db <= DB.second; ++db )
								lasta[db] = algnres.aepos + algnres.bepos;

							ldbmin = std::min(ldbmin,DB.first);
							ldbmax = std::max(ldbmax,DB.second);


							#if 0
							tracecontainer.printTraceLines(
								std::cerr,
								aa + algnres.abpos,
								ba + algnres.bbpos,
								80,
								std::string() /* indent */,
								std::string("\n") /* linesep */,
								libmaus2::fastx::remapChar
							);
							#endif

							libmaus2::lcs::NNPTraceContainer::shared_ptr_type tracecopy = getAlignment();
							tracecopy->copyFrom(tracecontainer);

							VR.push_back(
								std::pair<libmaus2::lcs::NNPAlignResult,libmaus2::lcs::NNPTraceContainer::shared_ptr_type>(
									algnres,
									tracecopy
								)
							);
						}
					}
				}

				// reset lastp, lastscore, lastnext, lasta
				for ( NNPLocalAlignerKmerMatches * m_c = m_a; m_c != m_e; ++m_c )
				{
					NNPLocalAlignerKmerMatches const & cur = *m_c;
					int64_t const bucket = cur.getDiag() >> bucketlog;

					lastp[bucket] = -static_cast<int64_t>(2*anak);
					lastscore[bucket] = 0;
					lasta[bucket] = 0;
					lastnext[bucket] = lastnextnull;
				}

				for ( int64_t db = ldbmin; db <= ldbmax; ++db )
					lasta[db] = 0;

				filterAlignments(VR,aa,ba);
				for ( uint64_t i = 0; i < VR.size(); ++i )
					// check alignment
					assert ( VR[i].second->checkTrace(aa+VR[i].first.abpos,ba+VR[i].first.bbpos) );

				uint64_t const alloccountafter = alloccount;
				uint64_t const alloccountdiff = alloccountafter - alloccountbef;
				assert ( alloccountdiff == VR.size() );

				return VR;
			}
		};
	}
}
#endif
