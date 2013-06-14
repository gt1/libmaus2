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
#if ! defined(LIBMAUS_BAMBAM_ADAPTERFILTER_HPP)
#define LIBMAUS_BAMBAM_ADAPTERFILTER_HPP

#include <libmaus/fastx/QReorder.hpp>
#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/util/PushBuffer.hpp>
#include <libmaus/bambam/BamDefaultAdapters.hpp>
#include <libmaus/bambam/AdapterOffsetStrand.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct AdapterFilter
		{
			private:
			enum { seedk = 3 };

			public:
			struct AdapterFragment
			{
				uint64_t w;
				uint16_t adpid; // adapter id
				uint16_t adppos; // position on adapter
				uint16_t adpstr; // strand
				
				AdapterFragment() : w(0), adpid(0), adppos(0), adpstr(0) {}
				AdapterFragment(
					uint64_t const rw,
					uint16_t const radpid,
					uint16_t const radppos,
					uint16_t const radpstr)
				: w(rw), adpid(radpid), adppos(radppos), adpstr(radpstr) {}
				
				bool operator<(AdapterFragment const & o) const
				{
					if ( w != o.w )
						return w < o.w;
					else if ( adpid != o.adpid )
						return adpid < o.adpid;
					else if ( adppos != o.adppos )
						return adppos < o.adppos;
					else
						return adpstr < o.adpstr;
				}
			};
			
			struct AdapterOffsetStrandFracComparator
			{
				bool operator()(libmaus::bambam::AdapterOffsetStrand const & A, libmaus::bambam::AdapterOffsetStrand const & B)
				{
					return A.frac > B.frac;
				}
			};
			
			private:
			unsigned int const seedlength;
			std::vector<AdapterFragment> fragments;
			std::vector<std::string> adaptersf;
			std::vector<std::string> adaptersr;
			std::vector<libmaus::bambam::BamAlignment::shared_ptr_type> badapters;
			libmaus::fastx::QReorder4Set<seedk,uint64_t>::unique_ptr_type kmerfilter;
			libmaus::fastx::AutoArrayWordPutObject<uint64_t> A;
			libmaus::autoarray::AutoArray<uint8_t> S;
			uint64_t const wmask;
			
			void addFragments(
				uint8_t const * const R,
				std::string const & s, 
				uint16_t adpid, uint16_t adpstr
			)
			{
				uint64_t w = 0;
				uint8_t const * u = reinterpret_cast<uint8_t const *>(s.c_str());
				uint8_t const * ue = u + s.size();
				for ( uint64_t i = 0; i < seedlength-1; ++i )
				{
					w <<= seedk;
					w |= R[*(u++)];
				}
				
				// uint16_t adppos = adpstr ? s.size()-seedlength : 0;
				uint16_t adppos = 0;
				while ( u != ue )
				{
					w <<= seedk;
					w |= R[*(u++)];
					w &= wmask;
					
					fragments.push_back(AdapterFragment(w,adpid,adppos,adpstr));
				
					// adppos = adpstr ? (adppos-1) : (adppos+1);
					adppos++;
				}
			}
			
			void init(std::istream & in)
			{
				assert ( seedlength );
				libmaus::bambam::BamDecoder bamdec(in);

				libmaus::autoarray::AutoArray<uint8_t> R(256);
				std::fill(R.begin(),R.end(),4);
				R['a'] = R['A'] = 0;
				R['c'] = R['C'] = 1;
				R['g'] = R['G'] = 2;
				R['t'] = R['T'] = 3;

				std::fill(S.begin(),S.end(),5);
				S['a'] = S['A'] = 0;
				S['c'] = S['C'] = 1;
				S['g'] = S['G'] = 2;
				S['t'] = S['T'] = 3;
				
				uint64_t r = 0;
				while ( bamdec.readAlignment() )
				{
					std::string const readf = bamdec.getAlignment().getRead();
					std::string const readr = bamdec.getAlignment().getReadRC();
					uint64_t const rl = readf.size();
					
					if ( rl < seedlength )
					{
						std::cerr << "WARNING: adapter/primer sequence " << bamdec.getAlignment().getName()
							<< " is too short for seed length." << std::endl;
						continue;
					}
					
					addFragments(R.begin(),readf,r,false);
					addFragments(R.begin(),readr,r,true);
					
					adaptersf.push_back(readf);
					adaptersr.push_back(libmaus::fastx::reverseComplementUnmapped(readf));
					badapters.push_back(bamdec.getAlignment().sclone());
					
					++r;
				}
				
				std::sort(fragments.begin(),fragments.end());

				std::vector<uint64_t> W;
				for ( uint64_t i = 0; i < fragments.size(); ++i )
					W.push_back(fragments[i].w);
					
				libmaus::fastx::QReorder4Set<seedk,uint64_t>::unique_ptr_type rkmerfilter(
						new libmaus::fastx::QReorder4Set<seedk,uint64_t>(seedlength,W.begin(),W.end(),16)
					);
					
				kmerfilter = UNIQUE_PTR_MOVE(rkmerfilter);
			
			}

			libmaus::fastx::AutoArrayWordPutObject<uint64_t> const & searchRanks(uint64_t const v, unsigned int const maxmis)
			{
				kmerfilter->searchRanks(v,maxmis,A);
				return A;
			}

			public:
			AdapterFilter(std::string const & adapterfilename, unsigned int const rseedlength = 12)
			: seedlength(rseedlength), S(256),
			  wmask(libmaus::math::lowbits(seedlength * seedk))
			{
				libmaus::aio::CheckedInputStream CIS(adapterfilename);
				init(CIS);
			}

			AdapterFilter(std::istream & adapterstream, unsigned int const rseedlength = 12)
			: seedlength(rseedlength), S(256),
			  wmask(libmaus::math::lowbits(seedlength * seedk))
			{
				init(adapterstream);
			}
			
			uint64_t getMatchStart(libmaus::bambam::AdapterOffsetStrand const & AOSentry) const
			{
				return AOSentry.getMatchStart();
			}
			
			uint64_t getAdapterMatchLength(uint64_t const n, libmaus::bambam::AdapterOffsetStrand const & AOSentry) const
			{
				std::string const & adp = AOSentry.adpstr ? adaptersr[AOSentry.adpid] : adaptersf[AOSentry.adpid];
				
				uint64_t const matchstart = AOSentry.getMatchStart();
				uint64_t const adpstart   = AOSentry.getAdapterStart();
				
				uint64_t const matchlen = n - matchstart;
				uint64_t const adplen = adp.size() - adpstart;
				
				uint64_t const comlen = std::min(matchlen,adplen);
			
				return comlen;
			}
			
			char const * getAdapterName(uint64_t const id) const
			{
				return badapters[id]->getName();
			}

			char const * getAdapterName(libmaus::bambam::AdapterOffsetStrand const & AOSentry) const
			{
				return getAdapterName(AOSentry.adpid);
			}
			
			void printAdapterMatch(
				uint8_t const * const ua,
				uint64_t const n,
				libmaus::bambam::AdapterOffsetStrand const & AOSentry
			) const
			{
				std::string const & adp = AOSentry.adpstr ? adaptersr[AOSentry.adpid] : adaptersf[AOSentry.adpid];

				uint64_t const matchstart = AOSentry.getMatchStart();
				uint64_t const adpstart   = AOSentry.getAdapterStart();
				
				uint64_t const matchlen = n - matchstart;
				uint64_t const adplen = adp.size() - adpstart;
				
				uint64_t const comlen = std::min(matchlen,adplen);
			
				std::cerr 
					<< "AdapterOffsetStrand(" 
						<< "id=" << AOSentry.adpid << "(" << badapters[AOSentry.adpid]->getName() << ")" << "," 
						<< "off=" << AOSentry.adpoff << "," 
						<< "str=" << AOSentry.adpstr << ","
						<< "sco=" << AOSentry.score << ","
						<< "sta=" << AOSentry.maxstart << ","
						<< "end=" << AOSentry.maxend << ","
						<< "len=" << AOSentry.maxend-AOSentry.maxstart << ","
						<< "fra=" << AOSentry.frac << ","
						<< "pfr=" << AOSentry.pfrac << ","
						<< "mat=" << matchstart + AOSentry.maxstart
						<< ")" << std::endl;

				for ( uint64_t i = 0; i < comlen; ++i )
					std::cerr << adp[adpstart+i];
				std::cerr << std::endl;

				for ( uint64_t i = 0; i < comlen; ++i )
					std::cerr << ua[matchstart+i];
				std::cerr << std::endl;

				for ( uint64_t i = 0; i < comlen; ++i )
					if ( adp[adpstart+i] == ua[matchstart+i] )
						std::cerr << "+";
					else
						std::cerr << "-";
				std::cerr << std::endl;

				for ( uint64_t i = 0; i < comlen; ++i )
					if ( i >= AOSentry.maxstart && i < AOSentry.maxend )
						std::cerr << "*";
					else
						std::cerr << " ";
				std::cerr << std::endl;
			
			}
			
			bool searchAdapters(
				uint8_t const * const ua,
				uint64_t const n,
				unsigned int const maxmis,
				libmaus::util::PushBuffer<libmaus::bambam::AdapterOffsetStrand> & AOSPB,
				int64_t const minscore,
				double const minfrac,
				double const minpfrac,
				int const verbose = 0
			)
			{
				AOSPB.reset();
				
				uint8_t const * u = ua;
				uint8_t const * const ue = u + n;
				
				if ( ue-u >= seedlength )
				{
					std::vector<libmaus::bambam::AdapterOffsetStrand> AOS;
				
					uint64_t w = 0;
					for ( uint64_t i = 0; i < seedlength-1; ++i )
					{
						w <<= seedk;
						w |= S[ *(u++) ];
					}
					
					uint64_t matchpos = 0;
					while ( u != ue )
					{
						w <<= seedk;
						w |= S[ *(u++) ];
						w &= wmask;
						
						searchRanks(w,maxmis);
						
						if ( A.p )
						{
							for ( uint64_t i = 0; i < A.p; ++i )
							{
								uint64_t const rank = A.A[i];
								AdapterFragment const & frag = fragments[rank];
								
								AOS.push_back(
									libmaus::bambam::AdapterOffsetStrand(
										frag.adpid,
										static_cast<int16_t>(frag.adppos)-static_cast<int16_t>(matchpos),
										frag.adpstr)
								);
							}
						}
						
						matchpos++;
					}
					
					std::sort(AOS.begin(),AOS.end());
					std::vector<libmaus::bambam::AdapterOffsetStrand>::iterator it = std::unique(AOS.begin(),AOS.end());
					AOS.resize(it-AOS.begin());
					
					for ( std::vector<libmaus::bambam::AdapterOffsetStrand>::iterator itc = AOS.begin(); itc != AOS.end(); ++itc )
					{
						libmaus::bambam::AdapterOffsetStrand & AOSentry = *itc;
					
						std::string const & adp = AOSentry.adpstr ? adaptersr[AOSentry.adpid] : adaptersf[AOSentry.adpid];

						uint64_t const matchstart = (AOSentry.adpoff >= 0) ? 0 : -AOSentry.adpoff;
						uint64_t const adpstart   = (AOSentry.adpoff >= 0) ? AOSentry.adpoff : 0;
						
						uint64_t const matchlen = n - matchstart;
						uint64_t const adplen = adp.size() - adpstart;
						
						uint64_t const comlen = std::min(matchlen,adplen);
						
						uint64_t curstart = 0;
						int64_t cursum = 0;
						int64_t curmax = -1;
						uint64_t maxstart = 0;
						uint64_t maxend = 0;
						
						int64_t const SCORE_MATCH = 1;
						int64_t const PEN_MISMATCH = -2;

						for ( uint64_t i = 0; i < comlen; ++i )
						{
							if ( adp[adpstart+i] == ua[matchstart+i] )
								cursum += SCORE_MATCH;
							else
								cursum += PEN_MISMATCH;
								
							if ( cursum < 0 )
							{
								cursum = 0;
								curstart = i+1;						
							}
							
							if ( cursum > curmax )
							{
								curmax = cursum;
								maxstart = curstart;
								maxend = i+1;
							}
						}
						
						AOSentry.score = curmax;
						AOSentry.frac = (maxend-maxstart) / static_cast<double>(adp.size());
						AOSentry.pfrac = (maxend-maxstart) / static_cast<double>(comlen);
						AOSentry.maxstart = maxstart;
						AOSentry.maxend = maxend;

						if ( verbose > 2 )
							printAdapterMatch(ua,n,AOSentry);
					}
					
					std::sort(AOS.begin(),AOS.end(),AdapterOffsetStrandFracComparator());
					
					for ( uint64_t i = 0; i < AOS.size(); ++i )
					{
						if ( AOS[i].score >= minscore && (AOS[i].frac) >= minfrac && (AOS[i].pfrac) >= minpfrac )
						{
							libmaus::bambam::AdapterOffsetStrand & AOSentry = AOS[i];

							if ( verbose > 1 )
								printAdapterMatch(ua,n,AOSentry);
						
							AOSPB.push(AOSentry);
						}
					}
					
					return ((AOSPB.end()-AOSPB.begin()) != 0);
				}
				
				// no overlap found
				return false;
			}
		};
	}
}
#endif
