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

#include <libmaus/bitio/CompactArray.hpp>
#include <libmaus/bitio/SignedCompactArray.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/suffixsort/divsufsort.hpp>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/lcp/LCP.hpp>
#include <libmaus/lcp/OracleLCP.hpp>

#include <libmaus/fm/SampledSA.hpp>
#include <libmaus/fm/SampledISA.hpp>

static void writePlcpDif(::libmaus::bitio::FastWriteBitWriterBuffer64Sync & FWBW, uint64_t plcpdif)
{
	while ( plcpdif > 63 )
		FWBW.write(0,64);
	assert ( plcpdif+1 <= 64 );
	FWBW.write(1,plcpdif+1);
}

template<typename lf_type, typename isa_type, typename lcp_type>
static void writeSuccinctLCP(
	lf_type & LF,
	isa_type & SISA,
	lcp_type & LCP,
	std::ostream & out
)
{
	uint64_t const n = LF.getN();
	::libmaus::serialize::Serialize<uint64_t>::serialize(out,n);
	::libmaus::serialize::Serialize<uint64_t>::serialize(out,(2*n+63)/64);
	
	::libmaus::aio::SynchronousGenericOutput<uint64_t> SGO(out,8*1024);
	::libmaus::aio::SynchronousGenericOutput<uint64_t>::iterator_type SGOit(SGO);
	::libmaus::bitio::FastWriteBitWriterBuffer64Sync FWBW(SGOit);

	uint64_t const isasamplingrate = SISA.isasamplingrate;
	::libmaus::autoarray::AutoArray<uint64_t> plcpbuf(isasamplingrate+1,false);
	
	uint64_t const rp0 = SISA.SISA[0];
	uint64_t const pdif0 = LCP[rp0] + 1 - LCP[LF(rp0)];
	
	// std::cerr << pdif0 << std::endl;
	writePlcpDif(FWBW,pdif0);
	
	for ( uint64_t i = 1; i < SISA.SISA.size(); ++i )
	{
		uint64_t r = SISA.SISA[i];

		uint64_t * op = plcpbuf.end();
		uint64_t * const opa = plcpbuf.begin()+1;
		
		while ( op != opa )
		{
			*(--op) = LCP[r];
			r = LF(r);				
		}			
		*(--op) = LCP[r];

		op = plcpbuf.end()-1;
		
		while ( op-- != plcpbuf.begin() )
			op[1] = (op[1]+1)-op[0];
			
		// 1 ...			
		
		for ( op = opa; op != plcpbuf.end(); ++op )
		{
			// std::cerr << *op << std::endl;			
			writePlcpDif(FWBW,*op);
		}
	}
	
	uint64_t const rest = 
		LF.getN() - ((LF.getN()/isasamplingrate)*isasamplingrate + 1);
		
	// std::cerr << "rest=" << rest << std::endl;
	
	// LF on rank of position 0
	uint64_t rr = LF(rp0);
	
	for ( uint64_t i = 0; i < rest; ++i )
	{
		plcpbuf[plcpbuf.size()-i-1] = LCP[rr];
		rr = LF(rr);			
	}
	plcpbuf[plcpbuf.size()-rest-1] = LCP[rr];
	
	for ( uint64_t i = 0; i < rest; ++i )
	{
		uint64_t pdif = plcpbuf[plcpbuf.size()-rest+i] + 1 - plcpbuf[plcpbuf.size()-rest+i-1];
		// std::cerr << "pdif=" << pdif << std::endl;
		writePlcpDif(FWBW,pdif);
	}

	FWBW.flush();
	SGO.flush();
	out.flush();
}

void testBin()
{
	unsigned int const n = 13;
	typedef ::libmaus::suffixsort::DivSufSort<32,uint8_t *,uint8_t const *,int64_t *,int64_t const *,256,false /* parallel */> sort_type;
	::libmaus::autoarray::AutoArray<int64_t> SA(n+1,false);	
	::libmaus::autoarray::AutoArray<int64_t> ISA(n+1,false);	
	::libmaus::autoarray::AutoArray<uint8_t> S(n+1,false);
	::libmaus::autoarray::AutoArray<uint8_t> BWT(n+1,false);
	S[n] = '$';
	
	for ( uint64_t i = 0; i < (1ull<<n); ++i )
	{
		for ( uint64_t j = 0; j < n; ++j )
			S[j] = ((i & ( 1ull << j )) != 0) ? 'b' : 'a';

		sort_type::divsufsort ( S.begin(), SA.get() , n+1 );
		::libmaus::autoarray::AutoArray<int64_t> PhiLCP = ::libmaus::lcp::computeLcp(S.get(),n+1,SA.get());
		
		for ( uint64_t j = 0; j < SA.size(); ++j )
			ISA[SA[j]] = j;
			
		for ( uint64_t j = 0; j < SA.size(); ++j )
			BWT[j] = S[ (SA[j] + S.size() -1) % S.size() ];
			
		::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, uint64_t >::unique_ptr_type
			PW(new ::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, uint64_t >(BWT.begin(),BWT.size()));
		for ( uint64_t j = 0; j < S.size(); ++j )
			assert ( (*PW)[j] == BWT[j] );
			
		::libmaus::lf::LF LF(PW);
		::libmaus::fm::SimpleSampledSA< ::libmaus::lf::LF > SSA(&LF,8);
		::libmaus::fm::SampledISA< ::libmaus::lf::LF > SISA(&LF,8);
		
		::libmaus::lcp::SuccinctLCP<
			::libmaus::lf::LF,
			::libmaus::fm::SimpleSampledSA< ::libmaus::lf::LF >,
			::libmaus::fm::SampledISA< ::libmaus::lf::LF >
		> SLCP(LF,SSA,SISA);

		std::ostringstream oout;
		writeSuccinctLCP(LF,SISA,PhiLCP,oout);
		
		std::istringstream iin(oout.str());
		::libmaus::lcp::SuccinctLCP<
			::libmaus::lf::LF,
			::libmaus::fm::SimpleSampledSA< ::libmaus::lf::LF >,
			::libmaus::fm::SampledISA< ::libmaus::lf::LF >
		> defSLCP(iin,SSA);
		
		for ( uint64_t j = 0; j < SA.size(); ++j )
		{
			// std::cerr << "***\t" << SLCP[j] << "\t" << defSLCP[j] << std::endl;
			assert ( SLCP[j] == defSLCP[j] );
		}

		#if 0		
		for ( uint64_t i = 0; i < 2*SA.size(); ++i )
			std::cerr << ::libmaus::bitio::getBit(SLCP.LCP,i);
		std::cerr << std::endl;
		
		for ( uint64_t i = 0; i < SA.size(); ++i )
			std::cerr << "PLCP[" << i << "]=" << PhiLCP[ISA[i]] 
				<< ":" << SLCP[ISA[i]]
				<< std::endl;
		#endif
		
		#if 0
		bool thecase = false;
		for ( uint64_t j = 1; j < n; ++j )
			if ( PhiLCP[ISA[j-1]] == PhiLCP[ISA[j]] )
				thecase = true;
		
		if ( thecase )
		{
			std::cerr << "------" << std::endl;
			
			for ( uint64_t j = 0; j < n; ++j )
				std::cerr << "[" << j << "]" << std::string(S.begin()+SA[j],S.end()) << std::endl;
				
			for ( uint64_t j = 0; j < n; ++j )
				std::cerr << "PLCP["<< j << "]=" << "LCP[" << ISA[j] << "]=" << PhiLCP[ISA[j]] << std::endl;
		}
		#endif
	}
}

void testLCP(::libmaus::autoarray::AutoArray<uint8_t> const & data, ::libmaus::autoarray::AutoArray<int64_t> const & SAdiv0)
{
	::libmaus::timing::RealTimeClock drtc; drtc.start();
	
	uint64_t const n = data.size();
	
	std::cerr << "Computing LCP by Phi...";
	drtc.start();	
	::libmaus::autoarray::AutoArray<int64_t> PhiLCP = ::libmaus::lcp::computeLcp(data.get(),n,SAdiv0.get());
	std::cerr << "done, time " << drtc.getElapsedSeconds() << std::endl;
	
	::libmaus::autoarray::AutoArray<uint32_t> USA(n);
	std::copy(SAdiv0.begin(),SAdiv0.end(),USA.get());
	std::cerr << "Computing LCP by oracle...";
	drtc.start();	
	::libmaus::lcp::OracleLCP<
		uint8_t const *, 
		::libmaus::rmq::FischerSystematicSuccinctRMQ,
		::libmaus::lcp::DefaultComparator<uint8_t>
	>::dc_lcp_compute(data.get(), USA.get(), n, 8);
	std::cerr << "done, time " << drtc.getElapsedSeconds() << std::endl;

	::libmaus::autoarray::AutoArray<uint32_t> KLCP(n,false);	
	std::cerr << "Computing LCP via Kasai et al...";
	drtc.start();
	::libmaus::lcp::computeLCPKasai(data.get(), n, SAdiv0.get(), KLCP.get());
	std::cerr << "done, time " << drtc.getElapsedSeconds() << std::endl;

	std::cerr << "Comparing Kasai to Phi...";	
	for ( uint64_t i = 0; i < n; ++i )
		assert ( KLCP[i] == PhiLCP[i] );
	std::cerr << "done." << std::endl;

	std::cerr << "Comparing Kasai to oracle...";	
	for ( uint64_t i = 0; i < n; ++i )
	{
		if ( KLCP[i] != USA[i] )
			std::cerr << "Failure for i=" << i <<
				" KLCP[i]=" << KLCP[i] << " oracle[i]=" << USA[i] << std::endl;
		assert ( KLCP[i] == USA[i] );
	}
	std::cerr << "done." << std::endl;
}

int main(int argc, char * argv[])
{
	testBin();

	if ( argc < 2 )
	{
		std::cerr << "usage: " << argv[0] << " <filename>" << std::endl;
		return EXIT_FAILURE;
	}
	
	::libmaus::autoarray::AutoArray<uint8_t> data = ::libmaus::util::GetFileSize::readFile(argv[1]);
	std::cerr << "Got file of size " << data.size() << std::endl;

	unsigned int const bitwidth = 64;

	typedef ::libmaus::bitio::CompactArray::const_iterator compact_const_it;
	typedef ::libmaus::bitio::CompactArray::iterator compact_it;
	::libmaus::bitio::CompactArray C(data.getN(), 8);
	for ( uint64_t i = 0; i < data.getN(); ++i )
		C.set ( i, data[i] );

	uint64_t const n = data.getN();
		
	typedef ::libmaus::suffixsort::DivSufSort<bitwidth,uint8_t *,uint8_t const *,int64_t *,int64_t const *> sort_type;
	typedef sort_type::saidx_t saidx_t;

	::libmaus::autoarray::AutoArray<saidx_t> SAdiv0(n,false);	

	std::cerr << "Running divsufsort...";
	::libmaus::timing::RealTimeClock drtc; drtc.start();
	sort_type::divsufsort ( data.get() , SAdiv0.get() , n );
	std::cerr << "done, time " << drtc.getElapsedSeconds() << std::endl;

	// xxx
	data.release();
	
	#if defined(LIBMAUS_HAVE_SYNC_OPS)
	typedef ::libmaus::bitio::SynchronousSignedCompactArray signed_compact_array_type;
	signed_compact_array_type CSA(n, ::libmaus::math::bitsPerNum(n) + 1 );
	typedef signed_compact_array_type::const_iterator compact_index_const_it;
	typedef signed_compact_array_type::iterator compact_index_it;
	typedef ::libmaus::suffixsort::DivSufSort<bitwidth,compact_it,compact_const_it,compact_index_it,compact_index_const_it,256,true /* parallel */> compact_index_sort_type;
	std::cerr << "Running divsufsort on double compact representation using " << CSA.getB() << " bits per position...";
	::libmaus::timing::RealTimeClock rtc; rtc.start();
	compact_index_sort_type::divsufsort(C.begin(), CSA.begin(), n);
	std::cerr << "done, time " << rtc.getElapsedSeconds() << std::endl;
	#else
	typedef ::libmaus::bitio::SignedCompactArray signed_compact_array_type;
	signed_compact_array_type CSA(n, ::libmaus::math::bitsPerNum(n) + 1 );
	typedef signed_compact_array_type::const_iterator compact_index_const_it;
	typedef signed_compact_array_type::iterator compact_index_it;
	typedef ::libmaus::suffixsort::DivSufSort<bitwidth,compact_it,compact_const_it,compact_index_it,compact_index_const_it,256,false /* parallel */> compact_index_sort_type;
	std::cerr << "Running divsufsort on double compact representation using " << CSA.getB() << " bits per position...";
	::libmaus::timing::RealTimeClock rtc; rtc.start();
	compact_index_sort_type::divsufsort(C.begin(), CSA.begin(), n);
	std::cerr << "done, time " << rtc.getElapsedSeconds() << std::endl;	
	#endif

	::libmaus::suffixsort::DivSufSortUtils<
		bitwidth,compact_it,compact_const_it,compact_index_it,compact_index_const_it>::sufcheck(C.begin(),CSA.begin(),n,1);
	
	for ( uint64_t i = 0; i < SAdiv0.getN(); ++i )
		assert ( CSA.get(i) == SAdiv0[i] );
}
