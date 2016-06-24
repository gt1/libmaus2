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
#if ! defined(LIBMAUS2_BAMBAM_ADAPTERFILTER_HPP)
#define LIBMAUS2_BAMBAM_ADAPTERFILTER_HPP

#include <libmaus2/fastx/QReorder.hpp>
#include <libmaus2/bambam/BamDecoder.hpp>
#include <libmaus2/util/PushBuffer.hpp>
#include <libmaus2/bambam/BamDefaultAdapters.hpp>
#include <libmaus2/bambam/AdapterOffsetStrand.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct AdapterFilter
		{
			typedef AdapterFilter this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			// bits per symbol
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
				bool operator()(libmaus2::bambam::AdapterOffsetStrand const & A, libmaus2::bambam::AdapterOffsetStrand const & B)
				{
					return A.frac > B.frac;
				}
			};

			private:
			struct AdapterInfo
			{
				std::string forward;
				std::string reco;
				std::string name;
				char const * cname;

				AdapterInfo() : cname(0) {}
				AdapterInfo(
					std::string const & rforward,
					std::string const & rreco,
					std::string const & rname
				) : forward(rforward), reco(rreco), name(rname), cname(name.c_str()) {}

				AdapterInfo & operator=(AdapterInfo const & O)
				{
					forward = O.forward;
					reco = O.reco;
					name = O.name;
					cname = name.c_str();
					return *this;
				}
			};

			// data not changed during operation
			unsigned int const seedlength;
			libmaus2::autoarray::AutoArray<uint8_t> const S;
			libmaus2::autoarray::AutoArray<uint8_t> const R;
			uint64_t const wmask;
			std::vector < AdapterInfo > const adapterinfo;
			std::vector<AdapterFragment> const fragments;
			libmaus2::fastx::QReorder4Set<seedk,uint64_t>::unique_ptr_type const kmerfilter;

			// data changed during operation
			libmaus2::fastx::AutoArrayWordPutObject<uint64_t> A;

			std::string const & getForwardAdapter(uint64_t const i) const
			{
				return adapterinfo[i].forward;
			}

			std::string const & getReverseComplementAdapter(uint64_t const i) const
			{
				return adapterinfo[i].reco;
			}

			char const * getAdapterName(uint64_t const i) const
			{
				return adapterinfo[i].cname;
			}

			static void addFragments(
				uint8_t const * const R,
				std::string const & s,
				uint16_t adpid,
				uint16_t adpstr,
				std::vector<AdapterFragment> & fragments,
				uint64_t const seedlength,
				uint64_t const wmask
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

			static libmaus2::autoarray::AutoArray<uint8_t> initS()
			{
				libmaus2::autoarray::AutoArray<uint8_t> S(256);
				std::fill(S.begin(),S.end(),5);
				S['a'] = S['A'] = 0;
				S['c'] = S['C'] = 1;
				S['g'] = S['G'] = 2;
				S['t'] = S['T'] = 3;
				return S;
			}

			static libmaus2::autoarray::AutoArray<uint8_t> initR()
			{
				libmaus2::autoarray::AutoArray<uint8_t> R(256);
				std::fill(R.begin(),R.end(),4);
				R['a'] = R['A'] = 0;
				R['c'] = R['C'] = 1;
				R['g'] = R['G'] = 2;
				R['t'] = R['T'] = 3;
				return R;
			}

			static std::vector<AdapterInfo> loadAdapterInfo(std::istream & in, uint64_t const seedlength)
			{
				assert ( seedlength );

				std::vector<AdapterInfo> adapterinfo;
				libmaus2::bambam::BamDecoder bamdec(in);

				for ( uint64_t r = 0; bamdec.readAlignment(); ++r )
				{
					// forward read: get read and turn all non A,C,G,T,N symbols to N
					std::string const readf = libmaus2::fastx::remapString(libmaus2::fastx::mapString(bamdec.getAlignment().getRead()));
					// reverse complement
					std::string const readr = libmaus2::fastx::reverseComplementUnmapped(readf);
					uint64_t const rl = readf.size();

					if ( rl < seedlength )
					{
						std::cerr << "WARNING: adapter/primer sequence " << bamdec.getAlignment().getName()
							<< " is too short for seed length." << std::endl;
						continue;
					}

					adapterinfo.push_back(AdapterInfo(readf,readr,bamdec.getAlignment().getName()));
				}

				return adapterinfo;
			}

			static std::vector<AdapterInfo> loadAdapterInfo(std::string const & fn, uint64_t const seedlength)
			{
				libmaus2::aio::InputStreamInstance ISI(fn);
				return loadAdapterInfo(ISI,seedlength);
			}

			static std::vector<AdapterFragment> generateFragments(
				uint8_t const * R,
				std::vector<AdapterInfo> const & adapterinfo,
				uint64_t const seedlength,
				uint64_t const wmask
			)
			{
				std::vector<AdapterFragment> fragments;

				for ( uint64_t r = 0; r < adapterinfo.size(); ++r )
				{
					addFragments(R,adapterinfo[r].forward,r,false,fragments,seedlength,wmask);
					addFragments(R,adapterinfo[r].reco,r,true,fragments,seedlength,wmask);
				}

				std::sort(fragments.begin(),fragments.end());

				return fragments;
			}

			static libmaus2::fastx::QReorder4Set<seedk,uint64_t>::unique_ptr_type constructKmerFilter(std::vector<AdapterFragment> const & fragments, uint64_t const seedlength)
			{
				std::vector<uint64_t> W;
				for ( uint64_t i = 0; i < fragments.size(); ++i )
					W.push_back(fragments[i].w);

				libmaus2::fastx::QReorder4Set<seedk,uint64_t>::unique_ptr_type rkmerfilter(
						new libmaus2::fastx::QReorder4Set<seedk,uint64_t>(seedlength,W.begin(),W.end(),16)
					);

				libmaus2::fastx::QReorder4Set<seedk,uint64_t>::unique_ptr_type kmerfilter = UNIQUE_PTR_MOVE(rkmerfilter);

				return UNIQUE_PTR_MOVE(kmerfilter);
			}

			void searchRanks(uint64_t const v, unsigned int const maxmis, libmaus2::fastx::AutoArrayWordPutObject<uint64_t> & TA) const
			{
				kmerfilter->searchRanks(v,maxmis,TA);
			}

			public:
			AdapterFilter(std::string const & adapterfilename, unsigned int const rseedlength = 12)
			: seedlength(rseedlength), S(initS()), R(initR()),
			  wmask(libmaus2::math::lowbits(seedlength * seedk)),
			  adapterinfo(loadAdapterInfo(adapterfilename,seedlength)),
			  fragments(generateFragments(R.begin(),adapterinfo,seedlength,wmask)),
			  kmerfilter(constructKmerFilter(fragments,seedlength))
			{
			}

			AdapterFilter(std::istream & adapterstream, unsigned int const rseedlength = 12)
			: seedlength(rseedlength), S(initS()), R(initR()),
			  wmask(libmaus2::math::lowbits(seedlength * seedk)),
			  adapterinfo(loadAdapterInfo(adapterstream,seedlength)),
			  fragments(generateFragments(R.begin(),adapterinfo,seedlength,wmask)),
			  kmerfilter(constructKmerFilter(fragments,seedlength))
			{
			}

			uint64_t getMatchStart(libmaus2::bambam::AdapterOffsetStrand const & AOSentry) const
			{
				return AOSentry.getMatchStart();
			}

			uint64_t getAdapterMatchLength(uint64_t const n, libmaus2::bambam::AdapterOffsetStrand const & AOSentry) const
			{
				std::string const & adp = AOSentry.adpstr ? getReverseComplementAdapter(AOSentry.adpid) : getForwardAdapter(AOSentry.adpid);

				uint64_t const matchstart = AOSentry.getMatchStart();
				uint64_t const adpstart   = AOSentry.getAdapterStart();

				uint64_t const matchlen = n - matchstart;
				uint64_t const adplen = adp.size() - adpstart;

				uint64_t const comlen = std::min(matchlen,adplen);

				return comlen;
			}

			char const * getAdapterName(libmaus2::bambam::AdapterOffsetStrand const & AOSentry) const
			{
				return getAdapterName(AOSentry.adpid);
			}

			void getAdapterMatchFreqs(
				uint64_t const n,
				libmaus2::bambam::AdapterOffsetStrand const & AOSentry,
				uint64_t & fA,
				uint64_t & fC,
				uint64_t & fG,
				uint64_t & fT
			) const
			{
				std::string const & adp = AOSentry.adpstr ? getReverseComplementAdapter(AOSentry.adpid) : getForwardAdapter(AOSentry.adpid);

				uint64_t const matchstart = AOSentry.getMatchStart();
				uint64_t const adpstart   = AOSentry.getAdapterStart();

				uint64_t const matchlen = n - matchstart;
				uint64_t const adplen = adp.size() - adpstart;

				uint64_t const comlen = std::min(matchlen,adplen);

				fA = fC = fG = fT = 0;

				for ( uint64_t i = 0; i < comlen; ++i )
				{
					char const sym = adp[adpstart+i];

					switch ( sym )
					{
						case 'a': case 'A': fA++; break;
						case 'c': case 'C': fC++; break;
						case 'g': case 'G': fG++; break;
						case 't': case 'T': fT++; break;
						default:
						{
							switch ( libmaus2::random::Random::rand8() % 4 )
							{
								case 0: fA++; break;
								case 1: fC++; break;
								case 2: fG++; break;
								case 3: fT++; break;
							}
						}
					}
				}
			}

			void printAdapterMatch(
				uint8_t const * const ua,
				uint64_t const n,
				libmaus2::bambam::AdapterOffsetStrand const & AOSentry
			) const
			{
				std::string const & adp = AOSentry.adpstr ? getReverseComplementAdapter(AOSentry.adpid) : getForwardAdapter(AOSentry.adpid);

				uint64_t const matchstart = AOSentry.getMatchStart();
				uint64_t const adpstart   = AOSentry.getAdapterStart();

				uint64_t const matchlen = n - matchstart;
				uint64_t const adplen = adp.size() - adpstart;

				uint64_t const comlen = std::min(matchlen,adplen);

				std::cerr
					<< "AdapterOffsetStrand("
						<< "id=" << AOSentry.adpid << "(" << getAdapterName(AOSentry) << ")" << ","
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
				libmaus2::util::PushBuffer<libmaus2::bambam::AdapterOffsetStrand> & AOSPB,
				int64_t const minscore,
				double const minfrac,
				double const minpfrac,
				libmaus2::fastx::AutoArrayWordPutObject<uint64_t> & TA,
				int const verbose = 0
			) const
			{
				AOSPB.reset();

				uint8_t const * u = ua;
				uint8_t const * const ue = u + n;

				if ( ue-u >= seedlength )
				{
					std::vector<libmaus2::bambam::AdapterOffsetStrand> AOS;

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

						searchRanks(w,maxmis,TA);

						if ( TA.p )
						{
							for ( uint64_t i = 0; i < TA.p; ++i )
							{
								uint64_t const rank = TA.A[i];
								AdapterFragment const & frag = fragments[rank];

								AOS.push_back(
									libmaus2::bambam::AdapterOffsetStrand(
										frag.adpid,
										static_cast<int16_t>(frag.adppos)-static_cast<int16_t>(matchpos),
										frag.adpstr)
								);
							}
						}

						matchpos++;
					}

					std::sort(AOS.begin(),AOS.end());
					std::vector<libmaus2::bambam::AdapterOffsetStrand>::iterator it = std::unique(AOS.begin(),AOS.end());
					AOS.resize(it-AOS.begin());

					for ( std::vector<libmaus2::bambam::AdapterOffsetStrand>::iterator itc = AOS.begin(); itc != AOS.end(); ++itc )
					{
						libmaus2::bambam::AdapterOffsetStrand & AOSentry = *itc;

						std::string const & adp = AOSentry.adpstr ? getReverseComplementAdapter(AOSentry.adpid) : getForwardAdapter(AOSentry.adpid);

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
							libmaus2::bambam::AdapterOffsetStrand & AOSentry = AOS[i];

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

			bool searchAdapters(
				uint8_t const * const ua,
				uint64_t const n,
				unsigned int const maxmis,
				libmaus2::util::PushBuffer<libmaus2::bambam::AdapterOffsetStrand> & AOSPB,
				int64_t const minscore,
				double const minfrac,
				double const minpfrac,
				int const verbose = 0
			)
			{
				return searchAdapters(ua,n,maxmis,AOSPB,minscore,minfrac,minpfrac,A,verbose);
			}
		};
	}
}
#endif
