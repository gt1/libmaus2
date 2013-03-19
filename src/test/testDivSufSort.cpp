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
