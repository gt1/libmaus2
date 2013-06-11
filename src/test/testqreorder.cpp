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

struct AdapterFilter
{
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
	
	enum { seedk = 3 };

	unsigned int const seedlength;
	std::vector<AdapterFragment> fragments;
	std::vector<std::string> adapters;
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
		
		uint16_t adppos = adpstr ? s.size()-seedlength : 0;
		while ( u != ue )
		{
			w <<= seedk;
			w |= R[*(u++)];
			w &= wmask;
			
			fragments.push_back(AdapterFragment(w,adpid,adppos,adpstr));
		
			adppos = adpstr ? (adppos-1) : (adppos+1);
		}
	}

	AdapterFilter(std::string const & adapterfilename, unsigned int const rseedlength = 12)
	: seedlength(rseedlength), S(256),
	  wmask(libmaus::math::lowbits(seedlength * seedk))
	{
		assert ( seedlength );
		libmaus::bambam::BamDecoder bamdec(adapterfilename);

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
			
			adapters.push_back(readf);
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
	
	void searchAdapters(std::string const & s, unsigned int const maxmis)
	{
		uint8_t const * u = reinterpret_cast<uint8_t const *>(s.c_str());
		uint8_t const * const ue = u + s.size();
		
		// std::cerr << "-----" << std::endl;

		if ( ue-u >= seedlength )
		{
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
						
						#if 1
						std::cerr 
							<< "matchpos=" << matchpos << " adapter "
							<< frag.adpid
							<< " position " 
							<< frag.adppos
							<< " strand "
							<< frag.adpstr
							<< std::endl;
						
						assert ( slowComp(frag.w,w,seedk,seedlength) <= maxmis );	
						
						std::cerr << s.substr(matchpos,seedlength) << std::endl;
						
						std::string const adpfrag = adapters[frag.adpid].substr(frag.adppos,seedlength);
						
						if ( frag.adpstr )						
							std::cerr << libmaus::fastx::reverseComplementUnmapped(adpfrag) << std::endl;
						else
							std::cerr << adpfrag << std::endl;
						#endif
					}
				}
				
				matchpos++;
			}
		}
	}
};

int main()
{
	try
	{
		// testQReorder4Set();
		AdapterFilter AF("adapters.bam",20);
		
		libmaus::bambam::BamDecoder bamdec(std::cin);
		uint64_t c = 0;
		while ( bamdec.readAlignment() )
		{
			AF.searchAdapters(bamdec.getAlignment().getRead(), 2);		
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

