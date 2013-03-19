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
#include <libmaus/suffixsort/CircularBwt.hpp>
#include <libmaus/lf/LF.hpp>

void mergeTwoTest(std::string const s)
{
	uint64_t const m = (s.size()+1)/2;
	
	std::string const front = s.substr(0,m);
	std::string const back  = s.substr(m);

	#if 0	
	if ( (! front.size()) || front == std::string(front.size(),front[0]) )
		return;
	#endif
	
	#if 0
	if ( (! back.size()) || back == std::string(back.size(),back[0]) )
		return;
	#endif
	
	// std::cerr << s << std::endl;
	
	uint64_t zrank = 0;
	int64_t const term = '#';
	std::pair< std::string,std::vector<bool> > const frontblock = ::libmaus::suffixsort::CircularBwt::circularBwt(s,0,m,term,&zrank);
	std::string const & frontbwt = frontblock.first;
	std::pair< std::string,std::vector<bool> > const backblock = ::libmaus::suffixsort::CircularBwt::circularBwt(s,m,s.size()-m,term);
	std::string const backstr = s.substr(m);
	std::string const & backbwt = backblock.first;
	
	#if defined(DEBUG)
	std::cerr << "front: " << s.substr(0,m) << std::endl;
	std::cerr << "back:  " << s.substr(m) << std::endl;
	#endif
	
	std::vector < std::string > frontsuffixes;
	std::vector < std::string > backsuffixes;
	std::string const s2 = s+s;
	for ( uint64_t i = 0; i < m; ++i )
		frontsuffixes.push_back(s2.substr(i));
	std::sort(frontsuffixes.begin(),frontsuffixes.end());
	for ( uint64_t i = 0; i < s.size()-m; ++i )
		backsuffixes.push_back(s2.substr(m+i));
	std::sort(frontsuffixes.begin(),frontsuffixes.end());
	std::sort(backsuffixes.begin(),backsuffixes.end());

	std::pair< std::string,std::vector<bool> > const fullpair = ::libmaus::suffixsort::CircularBwt::circularBwt(s,0,s.size(),s[s.size()-1]);

	typedef ::libmaus::lf::LF lf_type;
	typedef lf_type::wt_type wt_type;
	typedef lf_type::wt_ptr_type wt_ptr_type;
	
	wt_ptr_type WT(new wt_type(frontbwt.c_str(),frontbwt.size()));
	lf_type LF(WT);
	::libmaus::autoarray::AutoArray<uint64_t> GAP(frontbwt.size()+1);
	
	// recompute D array, the pseudo bwt is missing one symbol
	for ( uint64_t i = 0; i < LF.D.size(); ++i )
		LF.D[i] = 0;
	for ( uint64_t i = 0; i < m; ++i )
		LF.D[ s[i] ] ++;
	LF.D.prefixSums();
	
	// rank we start with
	uint64_t r = zrank;
	#if 0
	for ( uint64_t i = 0; i < s.size(); ++i )
		r = LF.step(s[s.size()-i-1],r);
	#endif
	
	uint64_t const firstblocklast = s[m-1];
	std::vector<bool> const & gt = backblock.second;
	for ( uint64_t ii = 0; ii < backbwt.size(); ++ii )
	{
		uint64_t const i = backbwt.size()-ii-1;
		uint64_t const sym = backstr[i];
		#if defined(DEBUG)
		uint64_t const prer = r;
		#endif
		uint64_t const step = LF.step(sym,r);
		bool const gtf = gt[i+1];
		
		if ( sym == firstblocklast )
		{
			r = step + gtf;
		}
		else
		{
			r = step;
		}
		
		GAP[r]++;
		
		std::string const ins = s2.substr(s.size()-ii-1);
		uint64_t k = 0;
		while ( k < frontsuffixes.size() && ins > frontsuffixes[k] )
			++k;
	
		#if defined(DEBUG)	
		std::cerr << "---\n\n";
		std::cerr << "Inserted " << s2.substr(s.size()-ii-1) << " rank " << r << " sym=" << static_cast<char>(sym) 
			<< " step=" << step  
			<< " gt=" << gtf
			<< " k=" << k
			<< std::endl;
		
		for ( uint64_t j = 0; j < frontsuffixes.size(); ++j )
			std::cerr 
				<< ((j==r) ? ('*') : (' '))
				<< ((j==prer) ? ('p') : (' '))
				<< ((j==k) ? ('e') : (' '))
				<< " "
				<< frontbwt[j]
				<< " "
				<< "[" << std::setw(2) << std::setfill('0') << j << std::setw(0) << "] = " << frontsuffixes[j] << std::endl;
		#endif
	}
	
	std::vector<uint8_t> voutbwt;
	uint64_t inp = 0;
	for ( uint64_t i = 0; i < GAP.size(); ++i )
	{
		for ( uint64_t j = 0; j < GAP[i]; ++j, ++inp )
		{
			int64_t const c = backbwt[inp];
			
			if ( c == term )
				voutbwt.push_back(s[m-1]);
			else
				voutbwt.push_back(c);				
		}
		if ( i < frontbwt.size() )
		{
			int64_t const c = frontbwt[i];

			if ( c == term )
				voutbwt.push_back(s[s.size()-1]);
			else
				voutbwt.push_back(c);						
		}
	}
	assert ( inp == backbwt.size() );
	std::string outbwt(voutbwt.size(),' ');
	std::copy(voutbwt.begin(),voutbwt.end(),outbwt.begin());

	#if defined(DEBUG)
	for ( uint64_t i = 0; i < voutbwt.size(); ++i )
		std::cerr << "(" << voutbwt[i] << "," << fullpair.first[i] << ")";
	std::cerr << std::endl;
	#endif
	
	for ( uint64_t i = 0; i < voutbwt.size(); ++i )
		assert ( voutbwt[i] == fullpair.first[i] );	
}

int testMergeTwo(unsigned int const k = 16)
{
	#if defined(_OPENMP)
	unsigned int const numthreads = omp_get_max_threads();
	#else
	unsigned int const numthreads = 1;
	#endif
		
	std::vector<std::string> strs(numthreads,std::string(k /* +1 */,'#'));
	
	#if 1
	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( int64_t i = 0; i < static_cast<int64_t>(1ull << k); ++i )
	{
		#if defined(_OPENMP)
		uint64_t const tid = omp_get_thread_num();
		#else
		uint64_t const tid = 0;
		#endif
	
		std::string & s = strs[tid];
		for ( unsigned int j = 0; j < k; ++j )
			s[j] = (i & (1ull << j)) ? 'b' : 'a';
		
		// std::cerr << s << std::endl;
		mergeTwoTest(s);
	}
	// mergeTwoTest("alabar_a_la_alabarda");
	#endif
		
	// mergeTwoTest("babaaaaaaaaaaaaa");
	return 0;
}

int main()
{
	return testMergeTwo();
}
