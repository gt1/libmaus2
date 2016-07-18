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

#include <libmaus2/huffman/LFSupportDecoder.hpp>

#include <libmaus2/huffman/LFRankPosEncoder.hpp>
#include <libmaus2/huffman/LFRankPosDecoder.hpp>

#include <libmaus2/huffman/LFSupportEncoder.hpp>
#include <libmaus2/huffman/LFSupportDecoder.hpp>
#include <libmaus2/sorting/ParallelExternalRadixSort.hpp>

#include <libmaus2/huffman/LFSymRankPosDecoder.hpp>
#include <libmaus2/huffman/LFSymRankPosEncoder.hpp>

#include <libmaus2/sorting/InPlaceParallelSort.hpp>
#include <libmaus2/sorting/ParallelStableSort.hpp>
#include <libmaus2/random/Random.hpp>
#include <libmaus2/sorting/InterleavedRadixSort.hpp>
#include <libmaus2/parallel/NumCpus.hpp>

#if 0
#include <libmaus2/huffman/GenericEncoder.hpp>

struct S
{
	template<typename writer_type>
	void encode(writer_type & /* writer */)
	{

	}
};

void f()
{
	libmaus2::huffman::GenericEncoderTemplate<S,libmaus2::huffman::HuffmanEncoderFileStd> GE("f",4096);
}
#endif

void testBlockSwapDifferent()
{
	uint8_t p[256];
	uint64_t const numthreads = libmaus2::parallel::NumCpus::getNumLogicalProcessors();

	for ( uint64_t s = 0; s <= sizeof(p); ++s )
	{
		uint8_t *pp = &p[0];
		uint64_t t = sizeof(p)/sizeof(p[0])-s;
		for ( uint64_t i = 0; i < s; ++i )
			*(pp++) = 1;
		for ( uint64_t i = s; i < (s+t); ++i )
			*(pp++) = 0;

		libmaus2::sorting::InPlaceParallelSort::blockswap<uint64_t>(&p[0],s,t,numthreads);

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

	uint64_t const numthreads = libmaus2::parallel::NumCpus::getNumLogicalProcessors();
	libmaus2::sorting::InPlaceParallelSort::blockswap<uint64_t>(&p[0],&p[0]+89,89,numthreads);

	for ( uint64_t i = 0; i < 2*89; ++i )
	{
		//std::cerr << "p[" << i << "]=" << static_cast<int64_t>(p[i]) << std::endl;
		assert ( p[i] == q[i] );
	}
}

#if 0
void testblockmerge()
{
	uint32_t A[256];

	for ( uint64_t i = 0; i < 128; ++i )
	{
		A[i] = i;
		A[i+128] = 3*i+1;
	}

	// TrivialBaseSort TBS;
	libmaus2::sorting::InPlaceParallelSort::FixedSizeBaseSort TBS(4);
	uint64_t const numthreads = libmaus2::parallel::NumCpus::getNumLogicalProcessors();
	libmaus2::sorting::InPlaceParallelSort::mergestep(&A[0],128,128,TBS,numthreads);

	for ( uint64_t i = 1; i < sizeof(A)/sizeof(A[0]); ++i )
		assert ( A[i-1] <= A[i] );

	for ( uint64_t i = 0; i < 128; ++i )
	{
		A[i] = i;
		A[i+128] = 3*i+1;
	}

	std::reverse(&A[0],&A[128]);
	std::reverse(&A[128],&A[256]);

	libmaus2::sorting::InPlaceParallelSort::mergestep(&A[0],128,128,std::greater<uint32_t>(),TBS,numthreads);

	for ( uint64_t i = 1; i < sizeof(A)/sizeof(A[0]); ++i )
		assert ( A[i-1] >= A[i] );

	#if 0
	for ( uint64_t i = 0; i < 256; ++i )
		std::cerr << "A[" << i << "]=" << A[i] << std::endl;
	#endif
}
#endif

void testinplacesort()
{
	uint64_t const n = 8ull*1024ull*1024ull*1024ull;
	libmaus2::autoarray::AutoArray<uint32_t> A(n,false);

	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( int64_t i = 0; i < static_cast<int64_t>(n); ++i )
		A[i] = n-i-1;

	// libmaus2::sorting::InPlaceParallelSort::ParallelFixedSizeBaseSort<uint32_t *, std::less<uint32_t> > TBS(512*1024, 4096) ;
	// FixedSizeBaseSort TBS(32*1024);
	// TrivialBaseSort TBS;
	// libmaus2::sorting::InPlaceParallelSort::inplacesort(&A[0],&A[n],std::less<uint32_t>(),TBS);
	uint64_t const numthreads = libmaus2::parallel::NumCpus::getNumLogicalProcessors();
	libmaus2::sorting::InPlaceParallelSort::inplacesort(&A[0],&A[n],numthreads);

	#if 0
	for ( uint64_t i = 0; i < n; ++i )
		std::cerr << "A[" << i << "]=" << A[i] << std::endl;
	#endif

	for ( uint64_t i = 1; i < n; ++i )
		assert ( A[i-1] <= A[i] );
}

void testinplacesort2()
{
	uint64_t const n = 8ull*1024ull*1024ull*1024ull;
	libmaus2::autoarray::AutoArray<uint32_t> A(n,false);

	#if defined(_OPENMP)
	#pragma omp parallel for
	#endif
	for ( int64_t i = 0; i < static_cast<int64_t>(n); ++i )
		A[i] = n-i-1;

	// libmaus2::sorting::InPlaceParallelSort::ParallelFixedSizeBaseSort<uint32_t *, std::less<uint32_t> > TBS(512*1024, 4096) ;
	// FixedSizeBaseSort TBS(32*1024);
	// TrivialBaseSort TBS;
	// libmaus2::sorting::InPlaceParallelSort::inplacesort(&A[0],&A[n],std::less<uint32_t>(),TBS);
	uint64_t const numthreads = libmaus2::parallel::NumCpus::getNumLogicalProcessors();
	libmaus2::sorting::InPlaceParallelSort::inplacesort2(&A[0],&A[n],numthreads);

	#if 0
	for ( uint64_t i = 0; i < n; ++i )
		std::cerr << "A[" << i << "]=" << A[i] << std::endl;
	#endif

	for ( uint64_t i = 1; i < n; ++i )
		assert ( A[i-1] <= A[i] );
}


void testMultiMerge()
{
	libmaus2::random::Random::setup();

	uint64_t n = 105812;
	libmaus2::autoarray::AutoArray<uint64_t> V(n,false);
	libmaus2::autoarray::AutoArray<uint64_t> W(n,false);
	for ( uint64_t i = 0; i < n; ++i )
		V[i] = ((i*29)%(31));
		// V[i] = libmaus2::random::Random::rand8() % 2;

	uint64_t const l = n/2;
	uint64_t const r = n-l;
	// uint64_t const l0 = l/2;
	// uint64_t const l1 = l-l0;
	// uint64_t const r0 = r/2;
	// uint64_t const r1 = r-r0;

	std::sort(V.begin(),V.begin()+l);
	std::sort(V.begin()+l,V.begin()+l+r);

	#if 0
	for ( uint64_t i = 0; i < l; ++i )
		std::cerr << V[i] << ";";
	std::cerr << std::endl;
	for ( uint64_t i = 0; i < r; ++i )
		std::cerr << V[l+i] << ";";
	std::cerr << std::endl;
	#endif

	libmaus2::sorting::ParallelStableSort::parallelMerge(V.begin(),V.begin()+l,V.begin()+l,V.begin()+l+r,W.begin(),std::less<uint64_t>());

	std::cerr << std::string(80,'-') << std::endl;
	for ( uint64_t i = 0; i < W.size(); ++i )
	{
		// std::cerr << W[i] << ";";
		assert ( i == 0 || W[i-1] <= W[i] );
	}
	// std::cerr << std::endl;
}

void testMultiSort()
{
	libmaus2::random::Random::setup();

	uint64_t rn = 16384;
	libmaus2::autoarray::AutoArray<uint64_t> V(rn,false);
	libmaus2::autoarray::AutoArray<uint64_t> W(rn,false);
	for ( uint64_t i = 0; i < V.size(); ++i )
		V[i] = ((i*29)%(31));
		// V[i] = libmaus2::random::Random::rand8() % 2;
		// V[i] = libmaus2::random::Random::rand8();

	std::less<uint64_t> order;
	uint64_t const * in = libmaus2::sorting::ParallelStableSort::parallelSort(V.begin(),V.end(),W.begin(),W.end(),order);

	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		// std::cerr << in[i] << ";";
		assert ( i == 0 || in[i-1] <= in[i] );
	}
	// std::cerr << std::endl;
}

#include <libmaus2/parallel/NumCpus.hpp>

void testParallelSortState(uint64_t const rn = (1ull << 14) )
{
	uint64_t const numcpus = libmaus2::parallel::NumCpus::getNumLogicalProcessors();

	libmaus2::autoarray::AutoArray<uint64_t> V(rn,false);
	libmaus2::autoarray::AutoArray<uint64_t> W(rn,false);
	for ( uint64_t i = 0; i < V.size(); ++i )
		V[i] = ((i*29)%(31));

	typedef uint64_t * iterator;
	typedef std::less<uint64_t> order_type;
	order_type order;
	libmaus2::sorting::ParallelStableSort::ParallelSortControl<iterator, order_type> PSC(
		V.begin(),V.end(),W.begin(),W.end(),order,numcpus,true /* copy back */
	);
	libmaus2::sorting::ParallelStableSort::ParallelSortControlState<iterator,order_type> sortstate = PSC.getSortState();

	while ( ! sortstate.serialStep() )
	{

	}

	for ( uint64_t i = 1; i < V.size(); ++i )
		assert ( V[i-1] <= V[i] );
}

#include <libmaus2/timing/RealTimeClock.hpp>
#include <libmaus2/huffman/LFSetBitEncoder.hpp>
#include <libmaus2/huffman/LFSetBitDecoder.hpp>
#include <libmaus2/huffman/LFValueEncoder.hpp>
#include <libmaus2/huffman/LFValueDecoder.hpp>
#include <libmaus2/huffman/LFPhiPairEncoder.hpp>
#include <libmaus2/huffman/LFPhiPairDecoder.hpp>
#include <libmaus2/huffman/LFPhiPairLCPDecoder.hpp>
#include <libmaus2/huffman/LFPhiPairLCPEncoder.hpp>
#include <libmaus2/huffman/LFRankLCPDecoder.hpp>
#include <libmaus2/huffman/LFRankLCPEncoder.hpp>

void testlfsupport()
{
	std::string fn = "mem://tmp_";

	#if 0
	{
		libmaus2::aio::OutputStreamInstance OSI(fn);
		libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO(OSI,4096);

		::libmaus2::bitio::FastWriteBitWriterStream64Std writer(SGO);
		for ( uint64_t i = 0; i < 1024; ++i )
		{
			// writer.write(i,14);
			writer.writeElias2(i);
		}

		writer.flush();
		SGO.flush();
	}

	{
		libmaus2::aio::InputStreamInstance ISI(fn);
		libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(ISI,4096);
		libmaus2::huffman::LFSSupportBitDecoder	dec(SGI);
		for ( uint64_t i = 0; i < 1024; ++i )
		{
			#if 0
			uint64_t const v = dec.peek(14);
			dec.erase(14);
			#endif
			// uint64_t const v = dec.read(14);
			uint64_t const v = libmaus2::bitio::readElias2(dec);
			std::cerr << v << std::endl;
		}
	}
	#endif

	uint64_t n = 128*1024;
	std::vector < libmaus2::huffman::LFInfo > V(n);
	libmaus2::autoarray::AutoArray < uint64_t > Vcat;
	libmaus2::autoarray::AutoArray<uint64_t> VO;
	libmaus2::autoarray::AutoArray<uint64_t> Poff;

	{
		libmaus2::huffman::LFSupportEncoder enc(fn,1024);
		uint64_t vcato = 0;
		uint64_t poffo = 0;
		for ( uint64_t i = 0; i < n; ++i )
		{
			// uint64_t const p = (1ull<<60) + n-i;
			uint64_t const p = std::numeric_limits<uint64_t>::max() - 1;

			V[i] = libmaus2::huffman::LFInfo(libmaus2::random::Random::rand8() % 4 /* sym */, p-19581*i /* p */,0 /* n */, 0 /* rv */, i%2 /* active */);

			uint64_t vo = 0;
			Poff.push(poffo,vcato);

			uint64_t const c = i % 8;
			for ( uint64_t j = 0; j < c; ++j )
			{
				uint64_t const vv = (1ull << 61)+j;
				VO.push(vo,vv);
				Vcat.push(vcato,vv);
			}

			V[i].v = VO.begin();
			V[i].n = c;

			enc.encode(V[i]);
		}
		for ( uint64_t i = 0; i < n; ++i )
		{
			V[i].v = Vcat.begin() + Poff[i];
		}
	}

	std::cerr << "encoding done." << std::endl;

	for ( uint64_t j = 0; j <= n; ++j )
	{
		//std::cerr << "j=" << j << std::endl;
		libmaus2::huffman::LFSupportDecoder dec(std::vector<std::string>(1,fn),j);
		libmaus2::huffman::LFInfo info;
		for ( uint64_t i = j; i < n; ++i )
		{
			dec.decode(info);
			// std::cerr << info << std::endl;
			assert ( info == V[i] );
		}
	}
}

void testrankpos()
{
	std::string fn = "mem://tmp_";

	#if 0
	{
		libmaus2::aio::OutputStreamInstance OSI(fn);
		libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO(OSI,4096);

		::libmaus2::bitio::FastWriteBitWriterStream64Std writer(SGO);
		for ( uint64_t i = 0; i < 1024; ++i )
		{
			// writer.write(i,14);
			writer.writeElias2(i);
		}

		writer.flush();
		SGO.flush();
	}

	{
		libmaus2::aio::InputStreamInstance ISI(fn);
		libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(ISI,4096);
		libmaus2::huffman::LFSRankPosBitDecoder	dec(SGI);
		for ( uint64_t i = 0; i < 1024; ++i )
		{
			#if 0
			uint64_t const v = dec.peek(14);
			dec.erase(14);
			#endif
			// uint64_t const v = dec.read(14);
			uint64_t const v = libmaus2::bitio::readElias2(dec);
			std::cerr << v << std::endl;
		}
	}
	#endif

	// uint64_t n = 128*1024;
	uint64_t n = 3*1024;
	std::vector < libmaus2::huffman::LFRankPos > V(n);
	libmaus2::autoarray::AutoArray < uint64_t > Vcat;
	libmaus2::autoarray::AutoArray<uint64_t> VO;
	libmaus2::autoarray::AutoArray<uint64_t> Poff;

	{
		libmaus2::huffman::LFRankPosEncoder enc(fn,1024);
		uint64_t vcato = 0;
		uint64_t poffo = 0;
		uint64_t r = 0;
		for ( uint64_t i = 0; i < n; ++i )
		{
			// uint64_t const p = (1ull<<60) + n-i;
			uint64_t const p = std::numeric_limits<uint64_t>::max() - 1;

			r += (libmaus2::random::Random::rand8() % 4);
			V[i] = libmaus2::huffman::LFRankPos(r /* rank */, p-19581*i /* p */,0 /* n */, 0 /* rv */, i%2 /* active */);

			uint64_t vo = 0;
			Poff.push(poffo,vcato);

			uint64_t const c = i % 8;
			for ( uint64_t j = 0; j < c; ++j )
			{
				uint64_t const vv = (1ull << 61)+j;
				VO.push(vo,vv);
				Vcat.push(vcato,vv);
			}

			V[i].v = VO.begin();
			V[i].n = c;

			enc.encode(V[i]);
		}
		for ( uint64_t i = 0; i < n; ++i )
		{
			V[i].v = Vcat.begin() + Poff[i];
		}
	}

	std::cerr << "encoding done." << std::endl;

	for ( uint64_t j = 0; j <= n; ++j )
	{
		if ( j % 1024 == 0 )
			std::cerr << "j=" << j << std::endl;
		libmaus2::huffman::LFRankPosDecoder dec(std::vector<std::string>(1,fn),j);
		libmaus2::huffman::LFRankPos info;
		for ( uint64_t i = j; i < n; ++i )
		{
			bool const ok = dec.decode(info);
			assert ( ok );
			// std::cerr << info << std::endl;
			assert ( info == V[i] );
		}
		assert ( ! dec.decode(info) );
	}

	uint64_t j = 0;
	while ( j < n )
	{
		uint64_t const r = V[j].r;
		uint64_t h = j+1;
		while ( h < n && V[h].r == r )
			++h;

		uint64_t rinit = r;
		if ( j )
		{
			assert ( V[j-1].r < r );
			rinit = V[j-1].r+1;
		}

		libmaus2::huffman::LFRankPosDecoder dec(std::vector<std::string>(1,fn),rinit,libmaus2::huffman::LFRankPosDecoder::init_type_rank);
		libmaus2::huffman::LFRankPos info;
		for ( uint64_t i = j; i < n; ++i )
		{
			bool const ok = dec.decode(info);
			assert ( ok );
			// std::cerr << info << std::endl;
			assert ( info == V[i] );
		}
		assert ( ! dec.decode(info) );

		j = h;
	}

	std::cerr << "[V] testrankpos done" << std::endl;
}

void testsymrankpos()
{
	std::string fn = "mem://tmp_";

	#if 0
	{
		libmaus2::aio::OutputStreamInstance OSI(fn);
		libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO(OSI,4096);

		::libmaus2::bitio::FastWriteBitWriterStream64Std writer(SGO);
		for ( uint64_t i = 0; i < 1024; ++i )
		{
			// writer.write(i,14);
			writer.writeElias2(i);
		}

		writer.flush();
		SGO.flush();
	}

	{
		libmaus2::aio::InputStreamInstance ISI(fn);
		libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(ISI,4096);
		libmaus2::huffman::LFSSymRankPosBitDecoder	dec(SGI);
		for ( uint64_t i = 0; i < 1024; ++i )
		{
			#if 0
			uint64_t const v = dec.peek(14);
			dec.erase(14);
			#endif
			// uint64_t const v = dec.read(14);
			uint64_t const v = libmaus2::bitio::readElias2(dec);
			std::cerr << v << std::endl;
		}
	}
	#endif

	uint64_t n = 128*1024;
	std::vector < libmaus2::huffman::LFSymRankPos > V(n);
	libmaus2::autoarray::AutoArray < uint64_t > Vcat;
	libmaus2::autoarray::AutoArray<uint64_t> VO;
	libmaus2::autoarray::AutoArray<uint64_t> Poff;

	{
		libmaus2::huffman::LFSymRankPosEncoder enc(fn,1024);
		uint64_t vcato = 0;
		uint64_t poffo = 0;
		for ( uint64_t i = 0; i < n; ++i )
		{
			// uint64_t const p = (1ull<<60) + n-i;
			uint64_t const p = std::numeric_limits<uint64_t>::max() - 1;

			V[i] = libmaus2::huffman::LFSymRankPos(i%13 /* sym*/,libmaus2::random::Random::rand8() % 4 /* rank */, p-19581*i /* p */,0 /* n */, 0 /* rv */, i%2 /* active */);

			uint64_t vo = 0;
			Poff.push(poffo,vcato);

			uint64_t const c = i % 8;
			for ( uint64_t j = 0; j < c; ++j )
			{
				uint64_t const vv = (1ull << 61)+j;
				VO.push(vo,vv);
				Vcat.push(vcato,vv);
			}

			V[i].v = VO.begin();
			V[i].n = c;

			enc.encode(V[i]);
		}
		for ( uint64_t i = 0; i < n; ++i )
		{
			V[i].v = Vcat.begin() + Poff[i];
		}
	}

	std::cerr << "encoding done." << std::endl;

	for ( uint64_t j = 0; j <= n; ++j )
	{
		// std::cerr << "j=" << j << std::endl;
		libmaus2::huffman::LFSymRankPosDecoder dec(std::vector<std::string>(1,fn),j);
		libmaus2::huffman::LFSymRankPos info;
		for ( uint64_t i = j; i < n; ++i )
		{
			dec.decode(info);
			// std::cerr << info << std::endl;
			assert ( info == V[i] );
		}
	}
}

struct PSC
{
	bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
	{
		return A.second < B.second;
	}
};

#include <libmaus2/sorting/PairFileSorting.hpp>

void testpairfilesorting(uint64_t const n)
{
	std::vector< std::pair<uint64_t,uint64_t> > C(n);
	for ( uint64_t i = 0; i < n; ++i )
	{
		C[i].first = i;
		C[i].second = libmaus2::random::Random::rand64();
	}

	std::sort(C.begin(),C.end(),PSC());

	std::string const fn = "mem://tmpfile";

	{
		libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO(fn,16*1024);
		for ( uint64_t i = 0; i < n; ++i )
		{
			SGO.put(C[i].first);
			SGO.put(C[i].second);
		}
		SGO.flush();
	}

	std::string const outfn = fn + ".out";

	libmaus2::sorting::PairFileSorting::sortPairFile(
		std::vector<std::string>(1,fn),
		fn + "_tmp",
		false /* second */,
		true /* keep first */,
		true /* keep second */,
		outfn,
		n / 32 * 2 * sizeof(uint64_t) /* bufsize */,
		32 /* num threads */,
		false /* delete input */,
		4 /* fan in */,
		&(std::cerr)
	);

	std::sort(C.begin(),C.end());

	{
		libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(outfn,16*1024);

		for ( uint64_t i = 0; i < C.size(); ++i )
		{
			uint64_t a, b;
			bool const a_ok = SGI.getNext(a);
			assert ( a_ok );
			bool const b_ok = SGI.getNext(b);
			assert ( b_ok );

			assert ( a == C[i].first );
			assert ( b == C[i].second );
		}
	}
}

void testpairfilesortingSecond(uint64_t const n)
{
	std::vector< std::pair<uint64_t,uint64_t> > C(n);
	for ( uint64_t i = 0; i < n; ++i )
	{
		C[i].second = i;
		C[i].first = libmaus2::random::Random::rand64();
	}

	std::sort(C.begin(),C.end());

	std::string const fn = "mem://tmpfile";

	{
		libmaus2::aio::SynchronousGenericOutput<uint64_t> SGO(fn,16*1024);
		for ( uint64_t i = 0; i < n; ++i )
		{
			SGO.put(C[i].first);
			SGO.put(C[i].second);
		}
		SGO.flush();
	}

	std::string const outfn = fn + ".out";

	libmaus2::sorting::PairFileSorting::sortPairFile(
		std::vector<std::string>(1,fn),
		fn + "_tmp",
		true /* second */,
		true /* keep first */,
		true /* keep second */,
		outfn,
		n / 32 * 2 * sizeof(uint64_t) /* bufsize */,
		32 /* num threads */,
		false /* delete input */,
		4 /* fan in */,
		&(std::cerr)
	);

	std::sort(C.begin(),C.end(),PSC());

	{
		libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(outfn,16*1024);

		for ( uint64_t i = 0; i < C.size(); ++i )
		{
			uint64_t a, b;
			bool const a_ok = SGI.getNext(a);
			assert ( a_ok );
			bool const b_ok = SGI.getNext(b);
			assert ( b_ok );

			assert ( a == C[i].first );
			assert ( b == C[i].second );
		}
	}
}

int main()
{
	{
		testpairfilesortingSecond(16*1024*1024);
		testpairfilesorting(16*1024*1024);

		testrankpos();

		testinplacesort2();

		// exit(0);

		testsymrankpos();
		testlfsupport();

		typedef uint64_t value_type;
		unsigned int keybytes[sizeof(value_type)];

		for ( unsigned int i = 0; i < sizeof(value_type); ++i )
			keybytes[i] = i;
		unsigned int const rounds = sizeof(value_type);

		std::cerr << "[V] generating random number array...";
		libmaus2::autoarray::AutoArray<value_type> Vrand(1ull<<30,false);
		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( uint64_t i = 0; i < Vrand.size(); ++i )
			// Vrand[i] = libmaus2::random::Random::rand64();
			Vrand[i] = i*(i % 37);
		std::cerr << "done." << std::endl;

		libmaus2::timing::RealTimeClock rtc;
		double t[2] = {0,0};
		uint64_t const runs = 10;
		uint64_t const numthreads = 32;

		for ( uint64_t j = 0; j < 10; ++j )
		{
			for ( uint64_t i = 0; i < 2; ++i )
			{

				if ( i == 0 )
				{
					static const libmaus2::autoarray::alloc_type atype = libmaus2::autoarray::alloc_type_cxx;

					libmaus2::autoarray::AutoArray<value_type,atype> V0(Vrand.size(),false);
					libmaus2::autoarray::AutoArray<value_type,atype> V1(Vrand.size(),false);

					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( uint64_t i = 0; i < Vrand.size(); ++i )
						V0[i] = Vrand[i];

					rtc.start();
					// radixsort_uint64_t(V0.begin(),V1.begin(),V0.size(),numthreads,rounds,&keybytes[0],0 /* interleave */);
					libmaus2::sorting::InterleavedRadixSort::byteradixsortTemplate<uint64_t,uint64_t>(V0.begin(),V0.end(),V1.begin(),V1.end(),numthreads);
				}
				else if ( i == 1 )
				{
					static const libmaus2::autoarray::alloc_type atype = libmaus2::autoarray::alloc_type_hugepages;

					libmaus2::autoarray::AutoArray<value_type,atype> V0(Vrand.size(),false);
					libmaus2::autoarray::AutoArray<value_type,atype> V1(Vrand.size(),false);

					#if defined(_OPENMP)
					#pragma omp parallel for
					#endif
					for ( uint64_t i = 0; i < Vrand.size(); ++i )
						V0[i] = Vrand[i];

					rtc.start();

					//radixsort_uint64_t(V0.begin(),V1.begin(),V0.size(),numthreads,rounds,&keybytes[0],0 /* interleave */);
					libmaus2::sorting::InterleavedRadixSort::byteradixsortTemplate<uint64_t,uint64_t>(V0.begin(),V0.end(),V1.begin(),V1.end(),numthreads);
				}

				t[i] += rtc.getElapsedSeconds();
			}

			std::cerr << t[0] << " " << t[1] << std::endl;
		}

		std::cerr << "4k " << t[0] / runs << std::endl;
		std::cerr << "1g " << t[1] / runs << std::endl;
	}

	return 0;

	std::cerr << "[V] generating random number array...";
	libmaus2::autoarray::AutoArray<uint64_t> Vrand(1ull<<30,false);
	for ( uint64_t i = 0; i < Vrand.size(); ++i )
		Vrand[i] = libmaus2::random::Random::rand64();
	std::cerr << "done." << std::endl;

	for ( unsigned int i = 0; i < 10; ++i )
	{
		{
			libmaus2::autoarray::AutoArray<uint64_t> Vin(Vrand.size(),false);
			#if defined(_OPENMP)
			#pragma omp parallel for
			#endif
			for ( uint64_t i = 0; i < Vrand.size(); ++i )
				Vin[i] = Vrand[i];

			libmaus2::autoarray::AutoArray<uint64_t> Vtmp(Vin.size(),false);

			std::cerr << "sorting byte...";
			libmaus2::timing::RealTimeClock rtc; rtc.start();
			libmaus2::sorting::InterleavedRadixSort::byteradixsortTemplate<uint64_t,uint64_t>(Vin.begin(),Vin.end(),Vtmp.begin(),Vtmp.end(),32);
			std::cerr << "done, " << (Vin.size() / rtc.getElapsedSeconds()) << std::endl;

			for ( uint64_t i = 1; i < Vin.size(); ++i )
				assert ( Vin[i-1] <= Vin[i] );
		}

		{
			libmaus2::autoarray::AutoArray<uint64_t> Vin(Vrand.size(),false);
			#if defined(_OPENMP)
			#pragma omp parallel for
			#endif
			for ( uint64_t i = 0; i < Vrand.size(); ++i )
				Vin[i] = Vrand[i];

			libmaus2::autoarray::AutoArray<uint64_t> Vtmp(Vin.size(),false);

			std::cerr << "sorting word...";
			libmaus2::timing::RealTimeClock rtc; rtc.start();
			libmaus2::sorting::InterleavedRadixSort::radixsort(Vin.begin(),Vin.end(),Vtmp.begin(),Vtmp.end(),32);
			std::cerr << "done, " << (Vin.size() / rtc.getElapsedSeconds()) << std::endl;

			for ( uint64_t i = 1; i < Vin.size(); ++i )
				assert ( Vin[i-1] <= Vin[i] );
		}
	}

	testParallelSortState(1u<<14);
	testParallelSortState(1u<<15);
	testParallelSortState(1u<<16);
	testParallelSortState(195911);

	testMultiMerge();
	testMultiSort();
	testBlockSwapDifferent();
	testBlockSwap();
	//testblockmerge();
	//testinplacesort();
	testinplacesort2();
}
