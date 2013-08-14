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
#include <cassert>
#include <algorithm>
#include <limits>

#if defined(_OPENMP)
#include <omp.h>
#endif

template<typename copy_type>
void blockswap(void * pa, void * pb, uint64_t const s)
{
	static uint64_t const parthres = 4096;

	uint64_t const full = s/sizeof(copy_type);
	uint64_t const frac = s - full * sizeof(copy_type);

	copy_type * ca = reinterpret_cast<copy_type *>(pa);
	copy_type * cb = reinterpret_cast<copy_type *>(pb);
	
	if ( s < parthres )
	{
		copy_type * ce = ca + full;
		
		while ( ca != ce )
			std::swap(*(ca++),*(cb++));
			
		uint8_t * ua = reinterpret_cast<uint8_t *>(ca);
		uint8_t * ue = ua + frac;
		uint8_t * ub = reinterpret_cast<uint8_t *>(cb);
		
		while ( ua != ue )
			std::swap(*(ua++),*(ub++));	
	}
	else
	{
		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( int64_t i = 0; i < static_cast<int64_t>(full); ++i )
			std::swap(ca[i],cb[i]);

		uint8_t * ua = reinterpret_cast<uint8_t *>(ca+full);
		uint8_t * ue = ua + frac;
		uint8_t * ub = reinterpret_cast<uint8_t *>(cb+full);
	
		while ( ua != ue )
			std::swap(*(ua++),*(ub++));	
	}
}

template<typename copy_type>
void blockswap(void * vpa, uint64_t s, uint64_t t)
{
	uint8_t * pa = reinterpret_cast<uint8_t *>(vpa);

	while ( s && t )
	{
		if ( s <= t )
		{
			blockswap<copy_type>(pa,pa+s,s);
			pa += s;
			t -= s;
		}
		else // if ( t < s )
		{
			uint8_t * pe = pa+s+t;
			blockswap<copy_type>(pe-2*t,pe-t,t);
			s -= t;
		}
	}
}

void testBlockSwapDifferent()
{
	uint8_t p[256];
	
	for ( uint64_t s = 0; s <= sizeof(p); ++s )
	{
		uint8_t *pp = &p[0];
		uint64_t t = sizeof(p)/sizeof(p[0])-s;
		for ( uint64_t i = 0; i < s; ++i )
			*(pp++) = 1;
		for ( uint64_t i = s; i < (s+t); ++i )
			*(pp++) = 0;
		
		blockswap<uint64_t>(&p[0],s,t);
	
		for ( uint64_t i = 0; i < t; ++i )
			assert ( p[i] == 0 );
		for ( uint64_t i = 0; i < s; ++i )
			assert ( p[t+i] == 1 );
	}
}

void testBlockSwap()
{
	uint8_t p[2*89];
	uint8_t q[2*89];
	
	for ( uint64_t i = 0; i < 89; ++i )
	{
		p[i] = i;
		p[i+89] = (89-i-1);

		q[i+89] = i;
		q[i   ] = (89-i-1);
	}
		
	blockswap<uint64_t>(&p[0],&p[0]+89,89);
	
	for ( uint64_t i = 0; i < 2*89; ++i )
	{
		//std::cerr << "p[" << i << "]=" << static_cast<int64_t>(p[i]) << std::endl;
		assert ( p[i] == q[i] );
	}
}

struct MergeStepBinSearchResult
{
	uint64_t l0;
	uint64_t l1;
	uint64_t r0;
	uint64_t r1;
	int64_t nbest;
	
	MergeStepBinSearchResult() : l0(0), l1(0), r0(0), r1(0), nbest(std::numeric_limits<int64_t>::max()) {}
	MergeStepBinSearchResult(
		uint64_t const rl0,
		uint64_t const rl1,
		uint64_t const rr0,
		uint64_t const rr1,
		int64_t const rnbest
	) : l0(rl0), l1(rl1), r0(rr0), r1(rr1), nbest(rnbest) {}
	
	MergeStepBinSearchResult sideswap() const
	{
		return MergeStepBinSearchResult(r0,r1,l0,l1,nbest);
	}
};

template<typename iterator, typename order_type>
MergeStepBinSearchResult mergestepbinsearch(
	iterator const aa, 
	iterator const ae, 
	iterator const ba, 
	iterator const be, 
	order_type order)
{
	typedef typename ::std::iterator_traits<iterator>::value_type value_type;		

	uint64_t const s = ae-aa;
	uint64_t const t = be-ba;

	uint64_t l = 0;
	uint64_t r = s;
	
	while ( r-l > 2 )
	{
		uint64_t const m = (l+r) >> 1;
		value_type const & v = aa[m];

		iterator bm = std::lower_bound(ba,be,v,order);

		int64_t n = static_cast<int64_t>((bm-ba) + m) - ((s+t)/2);
		
		if ( 
			n < 0 && (bm != be) && 
			(!(order(*bm,v))) &&
			(!(order(v,*bm)))
		)
		{
			std::pair<iterator,iterator> const eqr = ::std::equal_range(ba,be,v,order);
			n += std::min(-n,static_cast<int64_t>(eqr.second-eqr.first));
		}
					
		if ( n < 0 )
			l = m+1; // l excluded
		else // n >= 0
			r = m+1; // r included
	}
	
	uint64_t lbest = l;
	int64_t nbest = std::numeric_limits<int64_t>::max();
	iterator bmbest = ba;
	
	for ( uint64_t m = (l ? (l-1):l); m < r; ++m )
	{
		value_type const v = aa[m];
	
		iterator bm = std::lower_bound(ba,be,v,order);

		int64_t n = static_cast<int64_t>((bm-ba) + m) - ((s+t)/2);
		
		if ( n < 0 && bm != be && (!(order(*bm,v))) && (!(order(v,*bm))) )
		{
			std::pair<iterator,iterator> const eqr = ::std::equal_range(ba,be,v,order);
			uint64_t const add = std::min(-n,static_cast<int64_t>(eqr.second-eqr.first));
			n += add;
			bm += add;
		}
		
		if ( std::abs(n) < std::abs(nbest) )
		{
			lbest = m;
			nbest = n;
			bmbest = bm;
		}
	}

	uint64_t const l0 = lbest;
	uint64_t const l1 = s-l0;
		
	uint64_t const r0 = bmbest-ba;
	uint64_t const r1 = t-r0;

	return MergeStepBinSearchResult(l0,l1,r0,r1,nbest);
}

template<typename iterator, typename order_type, typename base_sort>
void mergestepRec(
	iterator p, 
	uint64_t const s, 
	uint64_t const t, 
	order_type order,
	base_sort & basesort
)
{
	if ( (!s) || (!t) )
	{
	
	}
	else if ( basesort(p,s,t,order) )
	{
	
	}
	else
	{
		iterator const aa = p;
		iterator const ae = aa + s;
		iterator const ba = ae;
		iterator const be = ba + t;

		MergeStepBinSearchResult const msbsr_l = mergestepbinsearch(aa,ae,ba,be,order);
		MergeStepBinSearchResult const msbsr_r = mergestepbinsearch(ba,be,aa,ae,order).sideswap();
		MergeStepBinSearchResult const msbsr = (std::abs(msbsr_l.nbest) <= std::abs(msbsr_r.nbest)) ? msbsr_l : msbsr_r;
		
		uint64_t const l0 = msbsr.l0;
		uint64_t const l1 = msbsr.l1;
		
		uint64_t const r0 = msbsr.r0;
		uint64_t const r1 = msbsr.r1;

		// std::cerr << "l=" << l0 << " n=" << (l0+r0) << " (s+t)/2=" << (s+t)/2 << std::endl;
		
		if ( (s+t)/2 != (l0+r0) )
		{
			std::cerr << "split uneven." << std::endl;
		
			#if 0
			std::cerr << "l0=";
			for ( uint64_t i = 0; i < l0; ++i )
				std::cerr << p[i] << ";";
			std::cerr << std::endl;

			std::cerr << "l1=";
			for ( uint64_t i = 0; i < l1; ++i )
				std::cerr << p[l0+i] << ";";
			std::cerr << std::endl;

			std::cerr << "r0=";
			for ( uint64_t i = 0; i < r0; ++i )
				std::cerr << p[l0+l1+i] << ";";
			std::cerr << std::endl;

			std::cerr << "r1=";
			for ( uint64_t i = 0; i < r1; ++i )
				std::cerr << p[l0+l1+r0+i] << ";";
			std::cerr << std::endl;
			#endif
		}
		
		// l0 l1 r0 r1 -> l0 r0 l1 r1
		typedef typename ::std::iterator_traits<iterator>::value_type value_type;		
		blockswap<uint64_t>(p+l0,l1*sizeof(value_type),r0*sizeof(value_type));
		
		// std::cerr << "l1=" << l1 << " r0=" << r0 << std::endl;
		
		mergestepRec(p,l0,r0,order,basesort);
		mergestepRec(p+l0+r0,l1,r1,order,basesort);
	}
}

template<typename iterator, typename order_type, typename base_sort>
void mergestep(
	iterator p, 
	uint64_t const s, 
	uint64_t const t, 
	order_type order,
	base_sort & basesort
)
{
	mergestepRec(p,s,t,order,basesort);
	basesort.flush();
}

template<typename iterator, typename base_sort>
void mergestep(iterator p, uint64_t const s, uint64_t const t, base_sort & basesort)
{
	typedef typename ::std::iterator_traits<iterator>::value_type value_type;		
	typedef std::less<value_type> order_type;
	mergestepRec<iterator,order_type>(p,s,t,order_type(),basesort);
	basesort.flush();
}

struct TrivialBaseSort
{
	TrivialBaseSort() {}
	
	template<typename iterator, typename order_type>
	bool operator()(iterator, uint64_t const, uint64_t const, order_type)
	{
		return false;
	}
	
	void flush()
	{
	
	}
};

struct FixedSizeBaseSort
{
	uint64_t const thres;

	FixedSizeBaseSort(uint64_t const rthres) : thres(rthres) {}
	
	template<typename iterator, typename order_type>
	bool operator()(iterator p, uint64_t const s, uint64_t const t, order_type order)
	{
		if ( s+t <= thres )		
		{
			std::inplace_merge(p,p+s,p+s+t,order);
			return true;
		}
		else
		{
			return false;
		}
	}
	
	void flush()
	{
	
	}
};

#include <libmaus/autoarray/AutoArray.hpp>

template<typename iterator, typename order_type>
struct ParallelFixedSizeBaseSort
{
	struct MergePackage
	{
		iterator p;
		uint64_t s;
		uint64_t t;
		order_type order;
		
		MergePackage() 
		: p(), s(0), t(0), order() {}
		MergePackage(
			iterator rp,
			uint64_t const rs,
			uint64_t const rt,
			order_type const rorder
		) : p(rp), s(rs), t(rt), order(rorder) {}
	};

	uint64_t const thres;
	libmaus::autoarray::AutoArray<MergePackage> Q;
	MergePackage * const qa;
	MergePackage *       qc;
	MergePackage * const qe;

	ParallelFixedSizeBaseSort(uint64_t const rthres, uint64_t const rqsize) 
	: thres(rthres), Q(rqsize,false), qa(Q.begin()), qc(qa), qe(Q.end()) {}
	
	bool operator()(iterator p, uint64_t const s, uint64_t const t, order_type order)
	{
		if ( s+t <= thres )		
		{
			*(qc++) = MergePackage(p,s,t,order);
			
			if ( qc == qe )
				flush();

			return true;
		}
		else
		{
			return false;
		}
	}
	
	void flush()
	{
		int64_t const f = qc-qa;
		
		#if defined(_OPENMP)
		#pragma omp parallel for schedule(dynamic,1)
		#endif
		for ( int64_t i = 0; i < f; ++i )
		{
			MergePackage const & qp = qa[i];
			std::inplace_merge(qp.p,qp.p+qp.s,qp.p+qp.s+qp.t,qp.order);			
		}

		qc = qa;
	}
};

void testblockmerge()
{
	uint32_t A[256];
	
	for ( uint64_t i = 0; i < 128; ++i )
	{
		A[i] = i;
		A[i+128] = 3*i+1;
	}
	
	// TrivialBaseSort TBS;
	FixedSizeBaseSort TBS(4);

	mergestep(&A[0],128,128,TBS);
		
	for ( uint64_t i = 1; i < sizeof(A)/sizeof(A[0]); ++i )
		assert ( A[i-1] <= A[i] );

	for ( uint64_t i = 0; i < 128; ++i )
	{
		A[i] = i;
		A[i+128] = 3*i+1;
	}
	
	std::reverse(&A[0],&A[128]);
	std::reverse(&A[128],&A[256]);

	mergestep(&A[0],128,128,std::greater<uint32_t>(),TBS);

	for ( uint64_t i = 1; i < sizeof(A)/sizeof(A[0]); ++i )
		assert ( A[i-1] >= A[i] );
	
	#if 0	
	for ( uint64_t i = 0; i < 256; ++i )
		std::cerr << "A[" << i << "]=" << A[i] << std::endl;
	#endif
}


template<typename iterator, typename order_type, typename base_sort>
void inplacesort(
	iterator a, 
	iterator e,
	order_type order,
	base_sort & basesort
)
{
	uint64_t const n = e-a;
	#if defined(_OPENMP)
	uint64_t const t = omp_get_max_threads();
	#else
	uint64_t const t = 1;
	#endif
	uint64_t const s0 = (n+t-1)/t;
	uint64_t const b = (n+s0-1)/s0;
	
	#if defined(_OPENMP)
	#pragma omp parallel for schedule(dynamic,1)
	#endif
	for ( int64_t i = 0; i < static_cast<int64_t>(b); ++i )
	{
		uint64_t const low = i*s0;
		uint64_t const high = std::min(low+s0,n);
		std::sort(a+low,a+high,order);
	}
	
	std::cerr << "blocks sorted." << std::endl;
	
	for ( uint64_t s = s0; s < n; s <<= 1 )
	{
		std::cerr << "s=" << s << std::endl;
	
		uint64_t const inblocks = (n+s-1)/s;
		uint64_t const outblocks = (inblocks+1)>>1;
		
		for ( uint64_t o = 0; o < outblocks; ++o )
		{
			uint64_t const low0 = 2*o*s;
			uint64_t const high0 = std::min(low0+s,n);
			uint64_t const low1 = high0;
			uint64_t const high1 = std::min(low1+s,n);
			
			std::cerr << "low0=" << low0 << " high0=" << high0 << " low1=" << low1 << " high1=" << high1 << std::endl;
			
			mergestep(a+low0,high0-low0,high1-low1,order,basesort);
		}
	}
}

#include <libmaus/autoarray/AutoArray.hpp>

void testinplacesort()
{
	uint64_t const n = 8ull*1024ull*1024ull*1024ull;
	libmaus::autoarray::AutoArray<uint32_t> A(n,false);
	
	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( int64_t i = 0; i < static_cast<int64_t>(n); ++i )
		A[i] = n-i-1;

	ParallelFixedSizeBaseSort<uint32_t *, std::less<uint32_t> > TBS(512*1024, 4096) ;
	// FixedSizeBaseSort TBS(32*1024);
	// TrivialBaseSort TBS;
	inplacesort(&A[0],&A[n],std::less<uint32_t>(),TBS);
	
	#if 0
	for ( uint64_t i = 0; i < n; ++i )
		std::cerr << "A[" << i << "]=" << A[i] << std::endl;
	#endif

	for ( uint64_t i = 1; i < n; ++i )
		assert ( A[i-1] <= A[i] );
}

int main(int argc, char * argv[])
{
	testBlockSwapDifferent();
	testBlockSwap();
	testblockmerge();
	testinplacesort();
}
