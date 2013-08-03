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
#if ! defined(LIBMAUS_LCS_HAMMINGOVERLAPDETECTION_HPP)
#define LIBMAUS_LCS_HAMMINGOVERLAPDETECTION_HPP

#include <libmaus/fastx/QReorder.hpp>
#include <libmaus/fastx/acgtnMap.hpp>
#include <libmaus/lcs/OverlapOrientation.hpp>
#include <libmaus/lcs/OrientationWeightEncoding.hpp>
#include <libmaus/util/Terminal.hpp>

namespace libmaus
{
	namespace lcs
	{
		struct HammingOverlapDetection : public libmaus::lcs::OrientationWeightEncoding
		{
			typedef HammingOverlapDetection this_type;

			enum { seedlen = 21 };
			enum { seedk = 3 };
			enum { symsperword = (sizeof(uint64_t)*8)/seedk };
			enum { match_gain = 1 };
			enum { mismatch_penalty = 3 };

			libmaus::autoarray::AutoArray<uint8_t> const R0;
			libmaus::autoarray::AutoArray<uint8_t> const R1;
			
			static libmaus::autoarray::AutoArray<uint8_t> computeR0()
			{
				libmaus::autoarray::AutoArray<uint8_t> R0(256,false);
				std::fill(R0.begin(),R0.end(),4);
				R0['a'] = R0['A'] = 0;
				R0['c'] = R0['C'] = 1;
				R0['g'] = R0['G'] = 2;
				R0['t'] = R0['T'] = 3;
				return R0;
			}

			static libmaus::autoarray::AutoArray<uint8_t> computeR1()
			{
				libmaus::autoarray::AutoArray<uint8_t> R1(256,false);
				std::fill(R1.begin(),R1.end(),5);
				R1['a'] = R1['A'] = 0;
				R1['c'] = R1['C'] = 1;
				R1['g'] = R1['G'] = 2;
				R1['t'] = R1['T'] = 3;
				return R1;
			}
			
			HammingOverlapDetection()
			: R0(computeR0()), R1(computeR1())
			{
			
			}
			
			static void printStringPair(
				std::ostream & out, 
				std::string s, std::string t, 
				uint64_t const prelinelength = ::libmaus::util::Terminal::getColumns()
			)
			{
				uint64_t const linelength = prelinelength-2;
				
				uint64_t const maxlen = std::max(s.size(),t.size());
				
				if ( s.size() < maxlen )
					s = s + std::string(maxlen-s.size(),' ');
				if ( t.size() < maxlen )
					t = t + std::string(maxlen-t.size(),' ');
					
				for ( uint64_t l = 0; l < (maxlen+linelength-1)/linelength; ++l )
				{
					uint64_t const low  = l*linelength;
					uint64_t const high = std::min(low+linelength,maxlen);
					uint64_t const len = high-low;
					
					out << "A " << s.substr(low,len) << std::endl;
					out << "B " << t.substr(low,len) << std::endl;
				}
			}
			
			void detect(
				std::string const & a, std::string const & b, unsigned int const maxmisperc, libmaus::lcs::OverlapOrientation::overlap_orientation & orientation,				
				libmaus::lcs::OverlapOrientation::overlap_orientation const cover_complete,
				libmaus::lcs::OverlapOrientation::overlap_orientation const cover_a_b,
				libmaus::lcs::OverlapOrientation::overlap_orientation const cover_b_a,
				libmaus::lcs::OverlapOrientation::overlap_orientation const dovetail,
				bool const overhangab,
				uint64_t & overhang,
				int64_t & maxscore,
				bool const verbose = false
			) const
			{
				uint64_t seed = 0;		
				std::string::const_iterator ita = a.end()-seedlen, ite = a.end();
				
				// std::cerr << "seed: " << std::string(ita,a.end()) << std::endl;
				
				while ( ita != ite )
				{
					seed <<= seedk;
					seed |= R0[*(ita++)];
				}
				
				std::string::const_reverse_iterator bita = b.rbegin();
				uint64_t q = 0;
				for ( unsigned int i = 0; i < seedlen-1; ++i )
				{
					q >>= seedk;
					q |= static_cast<uint64_t>(R1[*(bita++)]) << (seedk*(seedlen-1));
				}
				
				while ( bita != b.rend() )
				{
					q >>= seedk;
					q |= static_cast<uint64_t>(R1[*(bita++)]) << (seedk*(seedlen-1));
					uint64_t const seedmis = libmaus::fastx::KBlockBitCount<seedk>::diff(seed,q);

					if ( seedmis <= 2 )
					{
						uint64_t const bseedoffset = (b.size()-(bita-b.rbegin()));
						uint64_t const overlaplength = std::min(bseedoffset + seedlen,static_cast<uint64_t>(a.size()));
						uint64_t const boverlapstart = (bseedoffset + seedlen - overlaplength);
						uint64_t const aoverlapstart = a.size() - overlaplength;

						uint64_t const aboverhang = b.size() - (boverlapstart + overlaplength);
						uint64_t const baoverhang = aoverlapstart;

						uint64_t const restoverlap = overlaplength - seedlen;
						uint64_t const fullrestwords = restoverlap / symsperword;
						uint64_t const fracrestsyms = restoverlap - fullrestwords * symsperword;
						
						std::string::const_iterator rita = a.begin() + aoverlapstart;
						std::string::const_iterator ritb = b.begin() + boverlapstart;
						uint64_t mis = seedmis;
						for ( uint64_t i = 0; i < fullrestwords; ++i )
						{
							uint64_t wa = 0, wb = 0;
							for ( uint64_t j = 0; j < symsperword; ++j )
							{
								wa <<= seedk;
								wa |= R0[*(rita++)];
								wb <<= seedk;
								wb |= R1[*(ritb++)];
							}
						
							mis += libmaus::fastx::KBlockBitCount<seedk>::diff(wa,wb);					
						}
						if ( fracrestsyms )
						{
							uint64_t wa = 0, wb = 0;
							for ( uint64_t j = 0; j < fracrestsyms; ++j )
							{
								wa <<= seedk;
								wa |= R0[*(rita++)];
								wb <<= seedk;
								wb |= R1[*(ritb++)];
							}
							mis += libmaus::fastx::KBlockBitCount<seedk>::diff(wa,wb);
						}
						
						uint64_t const maxmis = (maxmisperc * overlaplength)/100;
						
						if ( mis <=  maxmis )
						{
							uint64_t const matches = overlaplength - mis;
							int64_t const score = static_cast<int64_t>(matches * match_gain) - static_cast<int64_t>(mis * mismatch_penalty);
											
							if ( score > maxscore )
							{
								maxscore = score;
														
								if ( overlaplength == a.size() && overlaplength == b.size() )
									orientation = cover_complete;
								else if ( overlaplength == a.size() )
									orientation = cover_b_a;
								else if ( overlaplength == b.size() )
									orientation = cover_a_b;
								else
								{
									orientation = dovetail;
									
									if ( overhangab )
										overhang = aboverhang;
									else
										overhang = baoverhang;
								}

								if ( verbose )
								{
									std::cerr 
										<< "mismatches " << mis << "/" << maxmis << " score " << score
										<< " overlap " << overlaplength
										<< " aboverhang " << aboverhang
										<< " baoverhang " << baoverhang
										<< " overhang " << overhang
										<< "\n";
									
									uint64_t const linelen = ::libmaus::util::Terminal::getColumns();
																		
									if ( orientation == dovetail || orientation == cover_complete )
									{
										printStringPair(
											std::cerr,
											a,
											std::string(a.size()-overlaplength,' ') + b,
											linelen
										);
									}
									else if ( orientation == cover_a_b )
									{
										printStringPair(
											std::cerr,
											a,
											std::string(aoverlapstart,' ') + b,
											linelen
										);
									}
									else if ( orientation == cover_b_a )
									{
										printStringPair(
											std::cerr,
											std::string(boverlapstart,' ') + a,
											b,
											linelen
										);
									}
								}
							}
						}
					}
				}		
			}

			static void printOverlap(
				std::ostream & out,
				std::string const & a,
				std::string const & b,
				libmaus::lcs::OverlapOrientation::overlap_orientation const o,
				uint64_t const overhang
				)
			{
				uint64_t const overlap = b.size() - overhang;
				uint64_t const indent = a.size()-overlap;
				
				switch ( o )
				{
					case libmaus::lcs::OverlapOrientation::overlap_a_back_dovetail_b_front:
						printStringPair(out,a,std::string(indent,' ')+b);
						break;
					case libmaus::lcs::OverlapOrientation::overlap_a_front_dovetail_b_front:
						printStringPair(out,libmaus::fastx::reverseComplementUnmapped(a),std::string(indent,' ')+b);
						break;
					case libmaus::lcs::OverlapOrientation::overlap_a_back_dovetail_b_back:
						printStringPair(out,a,std::string(indent,' ')+libmaus::fastx::reverseComplementUnmapped(b));
						break;
					case libmaus::lcs::OverlapOrientation::overlap_a_front_dovetail_b_back:
						printStringPair(out,libmaus::fastx::reverseComplementUnmapped(a),std::string(indent,' ')+libmaus::fastx::reverseComplementUnmapped(b));
						break;
					case libmaus::lcs::OverlapOrientation::overlap_a_complete_b:
						printStringPair(out,a,b);
						break;
					case libmaus::lcs::OverlapOrientation::overlap_ar_complete_b:
						printStringPair(out,libmaus::fastx::reverseComplementUnmapped(a),b);
						break;
					default:
						break;
				}
			}
			
			bool detect(
				std::string const & a, std::string const & b, unsigned int const maxmisperc, libmaus::lcs::OverlapOrientation::overlap_orientation & orientation,
				uint64_t & overhang,
				int64_t & maxscore,
				bool const verbose = false
			) const
			{
				std::string const ar = libmaus::fastx::reverseComplementUnmapped(a);
				std::string const br = libmaus::fastx::reverseComplementUnmapped(b);

				maxscore = ::std::numeric_limits<int64_t>::min();

				detect(a,b,maxmisperc,orientation,libmaus::lcs::OverlapOrientation::overlap_a_complete_b,libmaus::lcs::OverlapOrientation::overlap_a_covers_b,libmaus::lcs::OverlapOrientation::overlap_b_covers_a,libmaus::lcs::OverlapOrientation::overlap_a_back_dovetail_b_front,true,overhang,maxscore,verbose); 
				detect(b,a,maxmisperc,orientation,libmaus::lcs::OverlapOrientation::overlap_a_complete_b,libmaus::lcs::OverlapOrientation::overlap_b_covers_a,libmaus::lcs::OverlapOrientation::overlap_a_covers_b,libmaus::lcs::OverlapOrientation::overlap_a_front_dovetail_b_back,false,overhang,maxscore,verbose); 
				detect(ar,b,maxmisperc,orientation,libmaus::lcs::OverlapOrientation::overlap_ar_complete_b,libmaus::lcs::OverlapOrientation::overlap_ar_covers_b,libmaus::lcs::OverlapOrientation::overlap_b_covers_ar,libmaus::lcs::OverlapOrientation::overlap_a_front_dovetail_b_front,true,overhang,maxscore,verbose); 
				detect(b,ar,maxmisperc,orientation,libmaus::lcs::OverlapOrientation::overlap_ar_complete_b,libmaus::lcs::OverlapOrientation::overlap_b_covers_ar,libmaus::lcs::OverlapOrientation::overlap_ar_covers_b,libmaus::lcs::OverlapOrientation::overlap_a_back_dovetail_b_back,false,overhang,maxscore,verbose); 

				detect(br,ar,maxmisperc,orientation,libmaus::lcs::OverlapOrientation::overlap_a_complete_b,libmaus::lcs::OverlapOrientation::overlap_a_covers_b,libmaus::lcs::OverlapOrientation::overlap_b_covers_a,libmaus::lcs::OverlapOrientation::overlap_a_back_dovetail_b_front,false,overhang,maxscore,verbose); 
				detect(ar,br,maxmisperc,orientation,libmaus::lcs::OverlapOrientation::overlap_a_complete_b,libmaus::lcs::OverlapOrientation::overlap_b_covers_a,libmaus::lcs::OverlapOrientation::overlap_a_covers_b,libmaus::lcs::OverlapOrientation::overlap_a_front_dovetail_b_back,true,overhang,maxscore,verbose); 
				detect(br,a,maxmisperc,orientation,libmaus::lcs::OverlapOrientation::overlap_ar_complete_b,libmaus::lcs::OverlapOrientation::overlap_ar_covers_b,libmaus::lcs::OverlapOrientation::overlap_b_covers_ar,libmaus::lcs::OverlapOrientation::overlap_a_front_dovetail_b_front,false,overhang,maxscore,verbose); 
				detect(a,br,maxmisperc,orientation,libmaus::lcs::OverlapOrientation::overlap_ar_complete_b,libmaus::lcs::OverlapOrientation::overlap_b_covers_ar,libmaus::lcs::OverlapOrientation::overlap_ar_covers_b,libmaus::lcs::OverlapOrientation::overlap_a_back_dovetail_b_back,true,overhang,maxscore,verbose); 
			
				if ( verbose )
					printOverlap(std::cerr,a,b,orientation,overhang);
				
				return maxscore != ::std::numeric_limits<int64_t>::min();
			}
			
			
			static void testOverlapCombinations(bool const verbose = false)
			{
				this_type OD;

				libmaus::lcs::OverlapOrientation::overlap_orientation orientation = libmaus::lcs::OverlapOrientation::overlap_cover_complete;
				uint64_t overhang = 0;
				int64_t maxscore;

				std::string const a = "AGATGCTGATGCTGATGCTGATCGATGCTGATGCTGATGCTGACTGATGCATGATTGCATGCA";
				std::string const b = "GCTTTTGCTGATCGATGCTGATGCTGATGCTGACTGATGCATGATTGCATGCAAGTGAC";
				std::string const af = "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTATGCTGATGCTGAC";
				std::string const bf = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAATGTCAGTGGTGAC";
				std::string const ar = "ATGCTGATGCTGACTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT";
				std::string const br = "TGTCAGTGGTGACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

				OD.detect(a,b,10,orientation,overhang,maxscore,verbose);

				if ( maxscore != ::std::numeric_limits<int64_t>::min() )
					std::cerr << orientation << ":" << overhang << std::endl;

				OD.detect(b,a,10,orientation,overhang,maxscore,verbose);
				
				if ( maxscore != ::std::numeric_limits<int64_t>::min() )
					std::cerr << orientation << ":" << overhang << std::endl;

				OD.detect(af,bf,10,orientation,overhang,maxscore,verbose);
				
				if ( maxscore != ::std::numeric_limits<int64_t>::min() )
					std::cerr << orientation << ":" << overhang << std::endl;

				OD.detect(ar,br,10,orientation,overhang,maxscore,verbose);
				
				if ( maxscore != ::std::numeric_limits<int64_t>::min() )
					std::cerr << orientation << ":" << overhang << std::endl;

				OD.detect(a,a,10,orientation,overhang,maxscore,verbose);
				
				if ( maxscore != ::std::numeric_limits<int64_t>::min() )
					std::cerr << orientation << ":" << overhang << std::endl;

				OD.detect(a,libmaus::fastx::reverseComplementUnmapped(a),10,orientation,overhang,maxscore,verbose);
				
				if ( maxscore != ::std::numeric_limits<int64_t>::min() )
					std::cerr << orientation << ":" << overhang << std::endl;
			}
		};
	}
}
#endif
