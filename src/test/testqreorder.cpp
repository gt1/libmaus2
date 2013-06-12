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
#include <iostream>
#include <cstdlib>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/util/OutputFileNameTools.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/fastx/FastAReader.hpp>
#include <libmaus/aio/CircularWrapper.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>
#include <libmaus/fastx/QReorder.hpp>
#include <libmaus/bambam/BamDecoder.hpp>
#include <libmaus/util/PushBuffer.hpp>

void printBits(uint64_t const i)
{
	for ( uint64_t m = 1ull << 63; m; m >>= 1 )
		if ( i & m )
			std::cerr << '1';
		else
			std::cerr << '0';
	
	std::cerr << std::endl;
}

void testReorder4()
{
	unsigned int const k = 2;
	uint64_t const fraglen = 2;
	
	for ( uint64_t z = 0; z < 16; ++z )
	{
		libmaus::fastx::QReorderTemplate4Base<k> Q(
			fraglen+((z&1)!=0),
			fraglen+((z&2)!=0),
			fraglen+((z&4)!=0),
			fraglen+((z&8)!=0)
		);
	
		for ( uint64_t i = 0; i < (1ull<<((Q.l0+Q.l1+Q.l2+Q.l3)*Q.k)); ++i )
		{			
			assert ( i == Q.front01i(Q.front01(i)) );
			assert ( i == Q.front02i(Q.front02(i)) );
			assert ( i == Q.front03i(Q.front03(i)) );
			assert ( i == Q.front12i(Q.front12(i)) );
			assert ( i == Q.front13i(Q.front13(i)) );
			assert ( i == Q.front23i(Q.front23(i)) );
		}
	}
}

uint64_t slowComp(uint64_t a, uint64_t b, unsigned int k, unsigned int l)
{	
	unsigned int d = 0;
	uint64_t const mask = libmaus::math::lowbits(k);
	
	for ( unsigned int i = 0; i < l; ++i, a >>= k, b >>= k )
		 if ( (a & mask) != (b & mask) )
		 {
		 	d++;
		 }
		 
	return d;
}

std::vector<uint64_t> filter(
	std::vector<uint64_t> const & V, 
	uint64_t const q,
	unsigned int const k,
	unsigned int const l,
	unsigned int const maxmis
)
{
	std::vector<uint64_t> O;
	for ( uint64_t i = 0; i < V.size(); ++i )
		if ( slowComp(V[i],q,k,l) <= maxmis )
			O.push_back(V[i]);
	std::sort(O.begin(),O.end());
	return O;
}

void testQReorder4Set()
{
	srand(5);
	std::vector<uint64_t> V;
	unsigned int const l = 7;
	unsigned int const k = 2;
	uint64_t const bmask = (1ull<<(l*k))-1;
	
	for ( uint64_t i = 0; i < 32; ++i )
		V.push_back( rand() & bmask );

	libmaus::fastx::QReorder4Set<k,uint64_t> QR(l,V.begin(),V.end(),2);
	libmaus::fastx::AutoArrayWordPutObject<uint64_t> AAWPO;
	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		QR.search(V[i],0,AAWPO);
		assert ( AAWPO.p );
		
		// std::cerr << V[i] << "\t" << AAWPO.p << std::endl;
		
		if ( AAWPO.p )
			for ( uint64_t j = 0; j < AAWPO.p; ++j )
				assert ( AAWPO.A[j] == V[i] );

		std::vector<uint64_t> O = filter(V,V[i] /* q */,k,l,0);
		assert ( O == std::vector<uint64_t>(AAWPO.A.begin(),AAWPO.A.begin()+AAWPO.p) );
	}

	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		for ( unsigned int j = 0; j < (l*k); ++j )
		{
			uint64_t const q = V[i] ^ (1ull << j);
			QR.search(q,1,AAWPO);
			assert ( AAWPO.p );

			std::vector<uint64_t> O = filter(V,q,k,l,1);
			assert ( O == std::vector<uint64_t>(AAWPO.A.begin(),AAWPO.A.begin()+AAWPO.p) );
		}
	}

	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		for ( unsigned int j = 0; j < (l*k); ++j )
			for ( unsigned int z = 0; z < (l*k); ++z )
			{
				uint64_t const q = (V[i] ^ (1ull << j)) ^ (1ull << z);
			
				QR.search(q,2,AAWPO);
				assert ( AAWPO.p );

				std::vector<uint64_t> O = filter(V,q,k,l,2);
				assert ( O == std::vector<uint64_t>(AAWPO.A.begin(),AAWPO.A.begin()+AAWPO.p) );
				// std::cerr << "yes, " << O.size() << std::endl;
			}
	}
}

uint8_t const adapters_bam[1134] =
{
	31,139,8,4,0,0,0,0,0,255,6,0,66,67,2,0,81,4,181,152,93,111,219,54,20,134,53,160,55,238,205,38,
	78,105,189,174,197,34,129,67,185,97,138,107,181,41,6,3,193,66,69,67,105,108,115,141,56,237,190,10,20,114,172,56,
	222,108,217,147,221,54,217,85,77,104,128,183,139,97,87,187,218,143,220,126,65,70,55,180,195,80,148,165,168,9,1,3,
	134,12,188,207,57,228,57,239,161,236,226,111,222,233,106,154,182,77,188,210,211,70,173,186,241,160,212,122,92,251,229,69,
	16,29,135,254,32,184,190,221,124,84,170,123,181,182,63,24,15,163,73,169,217,88,126,221,249,186,86,25,15,15,38,175,
	252,40,168,28,142,252,110,165,221,11,43,252,215,245,214,227,173,165,200,92,248,222,198,189,141,135,159,95,103,36,237,25,
	251,156,240,181,54,127,160,93,211,110,9,207,230,79,188,81,88,175,219,143,130,35,27,119,252,209,36,136,170,118,85,195,
	148,64,140,136,161,35,132,1,74,93,5,245,29,13,24,132,26,198,12,1,131,194,21,250,223,41,244,111,102,233,59,44,
	126,211,48,177,1,60,115,10,63,250,55,69,251,137,66,123,45,91,219,209,40,41,187,144,206,144,59,219,86,10,63,21,
	68,110,100,10,55,119,118,237,102,212,27,176,125,17,162,126,79,169,124,164,80,254,44,135,178,163,233,113,25,120,16,152,
	22,54,189,153,137,9,157,154,196,194,233,27,159,92,63,11,156,15,57,123,61,149,221,10,88,73,134,251,189,176,123,26,
	129,6,207,157,56,166,89,184,31,5,233,119,57,238,142,132,107,4,71,147,131,62,207,180,42,228,136,89,122,235,43,229,
	91,10,249,242,42,121,71,56,28,163,156,38,251,143,32,113,135,203,126,145,38,219,246,163,253,97,39,64,157,160,27,132,
	65,228,79,130,79,88,223,121,58,118,1,32,172,237,168,73,204,202,201,201,127,52,171,224,242,175,60,165,217,232,251,245,
	164,33,80,15,196,83,22,83,172,171,55,182,173,80,78,156,151,66,89,178,130,242,234,12,191,87,80,100,63,72,82,114,
	26,66,177,205,201,227,8,223,10,42,55,179,149,47,98,9,199,10,105,217,18,148,210,151,225,9,125,1,116,59,197,19,
	4,120,194,20,206,147,205,56,139,247,187,160,13,56,175,38,241,154,95,10,73,142,171,9,95,64,104,102,33,0,48,68,
	136,194,212,70,206,177,254,84,4,179,149,17,204,121,23,129,4,65,183,76,145,27,235,150,75,103,22,166,175,223,34,30,
	132,158,43,76,77,62,14,22,145,127,90,184,227,211,57,63,247,27,118,8,174,129,89,13,27,89,8,95,129,48,87,33,
	88,127,3,58,163,214,212,132,32,111,142,93,65,112,45,29,50,150,202,169,90,0,53,80,160,62,206,70,57,90,209,179,
	251,74,16,191,198,129,242,141,105,23,111,106,57,135,229,80,33,231,200,114,77,185,7,96,177,118,63,93,127,43,28,71,
	158,113,123,209,11,214,236,11,139,180,235,97,135,181,191,147,152,110,22,142,167,151,56,221,196,59,195,226,138,34,219,44,
	143,172,53,240,251,125,123,183,129,237,251,119,121,177,106,49,46,83,234,17,215,52,212,129,180,21,242,242,209,37,228,55,
	151,242,57,79,244,47,65,241,131,20,143,227,148,39,97,239,37,107,50,191,191,216,232,171,178,186,67,129,95,226,49,125,
	42,197,244,38,205,33,107,66,136,13,3,178,100,77,79,143,231,113,188,49,152,252,172,95,21,172,135,106,150,163,121,52,
	102,185,234,172,122,60,224,198,204,81,203,22,160,136,97,93,189,88,166,175,21,116,121,247,57,253,254,85,237,246,111,138,
	24,228,193,194,99,120,112,165,243,228,64,128,26,41,62,204,174,174,195,65,111,223,99,165,206,11,125,49,84,46,210,215,
	197,72,69,102,203,31,130,234,251,41,7,44,144,70,124,178,92,209,81,135,2,249,118,118,222,242,24,42,144,255,129,34,
	127,43,149,184,53,226,227,78,40,179,188,57,143,4,213,91,156,4,37,210,225,113,59,234,117,236,94,200,78,51,100,62,
	118,102,198,152,189,11,120,102,60,245,32,6,172,168,9,204,124,99,44,202,60,115,104,214,56,83,139,128,178,229,162,216,
	133,83,118,39,202,195,124,169,96,34,137,217,31,134,93,70,28,7,209,196,118,196,116,53,19,25,46,133,166,5,216,40,
	52,1,177,128,158,143,250,150,228,205,187,44,219,25,241,0,97,88,74,188,24,98,23,25,153,23,240,197,218,83,84,209,
	13,137,60,158,207,192,243,19,246,204,18,136,90,247,7,133,174,60,98,151,186,23,30,173,121,254,241,89,170,55,119,90,
	203,238,207,122,9,123,165,16,150,95,194,20,194,151,241,10,214,82,236,88,106,78,187,123,156,156,153,209,79,138,210,90,
	79,147,77,216,210,69,255,236,249,31,119,227,102,68,26,21,0,0,31,139,8,4,0,0,0,0,0,255,6,0,66,67,
	2,0,27,0,3,0,0,0,0,0,0,0,0,0
};

struct AdapterOffsetStrand
{
	uint16_t adpid;
	int16_t adpoff;
	uint16_t adpstr;
	int16_t score;
	double frac;
	uint16_t maxstart;
	uint16_t maxend;
	
	AdapterOffsetStrand() : adpid(0), adpoff(0), adpstr(0), score(std::numeric_limits<int16_t>::min()), frac(0) {}
	AdapterOffsetStrand(
		uint16_t const radpid,
		int16_t const radpoff,
		uint16_t const radpstr
	) : adpid(radpid), adpoff(radpoff), adpstr(radpstr), score(std::numeric_limits<int16_t>::min()), frac(0) {}
	
	bool operator<(AdapterOffsetStrand const & o) const
	{
		if ( adpid != o.adpid )
			return adpid < o.adpid;
		else if ( adpoff != o.adpoff )
			return adpoff < o.adpoff;
		else
			return adpstr < o.adpstr;
	}
	
	bool operator==(AdapterOffsetStrand const & o) const
	{
		return
			adpid == o.adpid &&
			adpoff == o.adpoff &&
			adpstr == o.adpstr;
	}
};

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
		bool operator()(AdapterOffsetStrand const & A, AdapterOffsetStrand const & B)
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
	
	uint64_t getMatchStart(AdapterOffsetStrand const & AOSentry) const
	{
		return (AOSentry.adpoff >= 0) ? 0 : -AOSentry.adpoff;
	}
	
	uint64_t getAdapterMatchLength(uint64_t const n, AdapterOffsetStrand const & AOSentry) const
	{
		std::string const & adp = AOSentry.adpstr ? adaptersr[AOSentry.adpid] : adaptersf[AOSentry.adpid];
		
		uint64_t const matchstart = getMatchStart(AOSentry);
		uint64_t const adpstart   = (AOSentry.adpoff >= 0) ? AOSentry.adpoff : 0;
		
		uint64_t const matchlen = n - matchstart;
		uint64_t const adplen = adp.size() - adpstart;
		
		uint64_t const comlen = std::min(matchlen,adplen);
	
		return comlen;
	}
	
	void printAdapterMatch(
		uint8_t const * const ua,
		uint64_t const n,
		AdapterOffsetStrand const & AOSentry
	)
	{
		std::string const & adp = AOSentry.adpstr ? adaptersr[AOSentry.adpid] : adaptersf[AOSentry.adpid];

		uint64_t const matchstart = getMatchStart(AOSentry);
		uint64_t const adpstart   = (AOSentry.adpoff >= 0) ? AOSentry.adpoff : 0;
		
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
		libmaus::util::PushBuffer<AdapterOffsetStrand> & AOSPB,
		int64_t const minscore,
		double const minfrac,
		int const verbose = 0
	)
	{
		AOSPB.reset();
		
		uint8_t const * u = ua;
		uint8_t const * const ue = u + n;
		
		if ( ue-u >= seedlength )
		{
			std::vector<AdapterOffsetStrand> AOS;
		
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
							AdapterOffsetStrand(
								frag.adpid,
								static_cast<int16_t>(frag.adppos)-static_cast<int16_t>(matchpos),
								frag.adpstr)
						);
					}
				}
				
				matchpos++;
			}
			
			std::sort(AOS.begin(),AOS.end());
			std::vector<AdapterOffsetStrand>::iterator it = std::unique(AOS.begin(),AOS.end());
			AOS.resize(it-AOS.begin());
			
			for ( std::vector<AdapterOffsetStrand>::iterator itc = AOS.begin(); itc != AOS.end(); ++itc )
			{
				AdapterOffsetStrand & AOSentry = *itc;
			
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
				AOSentry.maxstart = maxstart;
				AOSentry.maxend = maxend;

				if ( verbose > 2 )
					printAdapterMatch(ua,n,AOSentry);
			}
			
			std::sort(AOS.begin(),AOS.end(),AdapterOffsetStrandFracComparator());
			
			for ( uint64_t i = 0; i < AOS.size(); ++i )
			{
				if ( AOS[i].score >= minscore && (AOS[i].frac) >= minfrac )
				{
					AdapterOffsetStrand & AOSentry = AOS[i];

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

int main()
{
	try
	{
		std::string builtinAdapters(
			reinterpret_cast<char const *>(&adapters_bam[0]),
			reinterpret_cast<char const *>(&adapters_bam[sizeof(adapters_bam)])
		);
		std::istringstream builtinAdaptersStr(builtinAdapters);
		
		// testQReorder4Set();
		AdapterFilter AF(builtinAdaptersStr,20);
		
		libmaus::bambam::BamDecoder bamdec(std::cin);
		libmaus::autoarray::AutoArray<char> Aread;
		uint64_t c = 0;
		// AdapterOffsetStrand AOSret;
		libmaus::util::PushBuffer<AdapterOffsetStrand> AOSPB;
		
		while ( bamdec.readAlignment() )
		{
			uint64_t const len = bamdec.getAlignment().decodeRead(Aread);
			uint8_t const * const ua = reinterpret_cast<uint8_t const *>(Aread.begin());
			
			bool const matched = AF.searchAdapters(ua, len, 2 /* max mismatches */, AOSPB,0,0.5);
			
			if ( matched )
			{
				std::cerr << "\n" << std::string(80,'-') << "\n\n";
				for ( AdapterOffsetStrand const * it = AOSPB.begin(); it != AOSPB.end(); ++it )
					AF.printAdapterMatch(ua,len,*it);
			}
				
			if ( (++c & (1024*16-1)) == 0 )
				std::cerr << c << std::endl;
		}
				
		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}

