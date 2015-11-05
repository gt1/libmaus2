/*
    libmaus2
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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

#include <libmaus2/lcs/MetaEditDistance.hpp>
#include <libmaus2/lcs/EditDistance.hpp>
#include <libmaus2/lcs/BandedEditDistance.hpp>
#include <libmaus2/random/Random.hpp>

static int64_t iabs(int64_t const v)
{
	if ( v < 0 )
		return -v;
	else
		return v;
}

static inline uint64_t dif(uint64_t const a, uint64_t const b)
{
	return static_cast<uint64_t>(iabs(static_cast<int64_t>(a)-static_cast<int64_t>(b)));
}

template<typename edit_a, typename edit_b>
static void testEdit(
	std::string const & a, std::string const & b, uint64_t const k,
	edit_a & E,
	edit_b & BE,
	bool const verbose = false
)
{
	typename edit_a::result_type const EDR = E.process(a.begin(),a.size(),b.begin(),b.size(),k);
	typename edit_b::result_type const EDRB = BE.process(a.begin(),a.size(),b.begin(),b.size(),k);

	bool const EDRBvalid = EDRB.nummis + EDRB.numins + EDRB.numdel <= k;
	bool const EDRcheck  = EDR.nummis  + EDR.numins  + EDR.numdel  <= k;

	assert ( EDRcheck == EDRBvalid );

	if ( verbose )
	{
		std::cout << a << std::endl;
		std::cout << b << std::endl;

		std::cout << EDR << std::endl;
		E.printAlignmentLines(std::cout,a,b,80);
		std::cout << EDRB << std::endl;
		BE.printAlignmentLines(std::cout,a,b,80);
	}

	#if 0
	if ( EDRBvalid && E.traceToString() != BE.traceToString() )
	{
		std::cout << a << std::endl;
		std::cout << b << std::endl;

		std::cout << EDR << std::endl;
		E.printAlignmentLines(std::cout,a,b,80);
		std::cout << EDRB << std::endl;
		BE.printAlignmentLines(std::cout,a,b,80);
	}
	#endif

	if ( EDRcheck )
	{
		assert ( E.getTrace() == BE.getTrace() );
	}
}

template<typename edit_a, typename edit_b>
static void testEdit(std::string const & a, std::string const & b, edit_a & E, edit_b & BE, bool const verbose = false)
{
	testEdit(a,b,dif(a.size(),b.size()),E,BE,verbose);
}

template<typename edit_a, typename edit_b>
static void enumerate(uint64_t const n, uint64_t const k, edit_a & E, edit_b & BE)
{
	uint64_t z = 1;
	for ( uint64_t i = 0; i < n; ++i )
		z *= k;


	std::string a(n+3,'a');
	for ( uint64_t i = 0; i < a.size(); ++i )
		a[i] = 'a' + (libmaus2::random::Random::rand8() % k);

	std::string b(n,' ');
	for ( uint64_t i = 0; i < z; ++i )
	{
		uint64_t x = i;
		for ( uint64_t j = 0; j < n; ++j )
		{
			b[j] = 'a' + (x % k);
			x /= k;
		}

		uint64_t const kmin = dif(a.size(),b.size());
		testEdit(a,b,kmin,E,BE);
		testEdit(a,b,kmin+1,E,BE);
		testEdit(a,b,kmin+2,E,BE);
	}
}

template<typename edit_a, typename edit_b>
void runtest(edit_a & E, edit_b & BE)
{
	libmaus2::random::Random::setup(5);

	testEdit("lichesxein","lichtensteijn",E,BE);
	testEdit("lichtensteijn","lichesxein",E,BE);

	testEdit("schokolade lecker","iss schokolade",E,BE);
	testEdit("iss schokolade","schokolade lecker",E,BE);

	testEdit("aaaaaab","aaaaba",E,BE);
	testEdit("aaaaba","aaaaaab",E,BE);

	std::cerr << "16,2" << std::endl;
	enumerate(16,2,E,BE);
	std::cerr << "12,3" << std::endl;
	enumerate(12,3,E,BE);
	std::cerr << "12,4" << std::endl;
	enumerate(12,4,E,BE);
}

template<libmaus2::lcs::edit_distance_priority_type edit_distance_priority>
void runtestconcrete()
{
	libmaus2::lcs::EditDistance<edit_distance_priority> E;
	libmaus2::lcs::BandedEditDistance<edit_distance_priority> BE;
	runtest(E,BE);
}

template<libmaus2::lcs::edit_distance_priority_type edit_distance_priority>
void runtestmeta()
{
	libmaus2::lcs::EditDistance<edit_distance_priority> E;
	libmaus2::lcs::MetaEditDistance<edit_distance_priority> BE;
	runtest(E,BE);
}

template<libmaus2::lcs::edit_distance_priority_type edit_distance_priority>
void runtestshort()
{
	libmaus2::lcs::EditDistance<edit_distance_priority> E;
	libmaus2::lcs::BandedEditDistance<edit_distance_priority> BE;
	testEdit("bbbbbba","bbbbb",E,BE,true);
	testEdit("bbbbb","bbbbbba",E,BE,true);
	testEdit("bbababb","bbaabbaab",E,BE,true);
}

#include <libmaus2/lcs/LocalEditDistance.hpp>
#include <libmaus2/lcs/LocalAlignmentPrint.hpp>
#include <libmaus2/lcs/BandedLocalEditDistance.hpp>
#include <libmaus2/lcs/MetaLocalEditDistance.hpp>

#include <libmaus2/lcs/BandedAlignerFactory.hpp>

int main()
{
	try
	{
		{
			std::set < libmaus2::lcs::BandedAlignerFactory::aligner_type > sup =libmaus2::lcs::BandedAlignerFactory::getSupportedAligners();

			std::string const a = "CTCCCCGGTTGCAGAATCGCGCAGAACACAGGATTCCCTAAGCAACCTTTCCACTAGAATCGCCG";
			std::string const b =   "NCCCGGTTGCAGAATCGCGCATGAACACAGGATTCCCTAAGCAACCTTTCCACTAGAATCGN";

			uint8_t const * ua = reinterpret_cast<uint8_t const *>(a.c_str());
			size_t const la = a.size();
			uint8_t const * ub = reinterpret_cast<uint8_t const *>(b.c_str());
			size_t const lb = b.size();

			for ( std::set < libmaus2::lcs::BandedAlignerFactory::aligner_type >::const_iterator ita = sup.begin(); ita != sup.end(); ++ita )
			{
				std::cerr << *ita << std::endl;
				libmaus2::lcs::BandedAligner::unique_ptr_type P (libmaus2::lcs::BandedAlignerFactory::construct(*ita) );
				P->align(ua,la,ub,lb,2*(std::max(a.size(),b.size())-std::min(a.size(),b.size())));
				libmaus2::lcs::AlignmentTraceContainer const & trace = P->getTraceContainer();
				libmaus2::lcs::AlignmentPrint::printAlignmentLines(std::cerr,a,b,80,trace.ta,trace.te);
			}
		}

		{
			libmaus2::lcs::BandedLocalEditDistance< ::libmaus2::lcs::diag_del_ins > LED;
			std::string const a = "CTCCCCGGTTGCAGAATCGCGCAGAACACAGGATTCCCTAAGCAACCTTTCCACTAGAATCGCCG";
			std::string const b =   "NCCCGGTTGCAGAATCGCGCATGAACACAGGATTCCCTAAGCAACCTTTCCACTAGAATCGN";
			libmaus2::lcs::LocalEditDistanceResult LEDR = LED.process(
				a.begin(),a.size(),
				b.begin(),b.size(),
				// a.size()
				2*(std::max(a.size(),b.size())-std::min(a.size(),b.size()))
			);

			std::cerr << LEDR << std::endl;
			libmaus2::lcs::LocalAlignmentPrint::printAlignmentLines(std::cerr,a,b,80,LED.ta,LED.te,LEDR);
		}

		{
			libmaus2::lcs::LocalEditDistance< ::libmaus2::lcs::diag_del_ins > LED;
			std::string const a = "XXXXXAABAAYYYY";
			std::string const b = "ZAAAAZZ";
			libmaus2::lcs::LocalEditDistanceResult LEDR = LED.process(a.begin(),a.size(),b.begin(),b.size());

			std::cerr << LEDR << std::endl;
			libmaus2::lcs::LocalAlignmentPrint::printAlignmentLines(std::cerr,a,b,80,LED.ta,LED.te,LEDR);
		}
		{
			libmaus2::lcs::MetaLocalEditDistance< ::libmaus2::lcs::diag_del_ins > LED;
			std::string const a = "XXXXXAABAAYYYY";
			std::string const b = "ZAAAAZZ";
			libmaus2::lcs::LocalEditDistanceResult LEDR = LED.process(a.begin(),a.size(),b.begin(),b.size(),a.size()-b.size());

			std::cerr << LEDR << std::endl;
			libmaus2::lcs::LocalAlignmentPrint::printAlignmentLines(std::cerr,a,b,80,LED.ta,LED.te,LEDR);
		}
		{
			libmaus2::lcs::BandedLocalEditDistance< ::libmaus2::lcs::diag_del_ins > LED;
			std::string const a = "XXAABAAY";
			std::string const b = "ZAAAAZZ";
			libmaus2::lcs::LocalEditDistanceResult LEDR = LED.process(
				a.begin(),a.size(),
				b.begin(),b.size(),
				// a.size()
				2*(std::max(a.size(),b.size())-std::min(a.size(),b.size()))
			);

			std::cerr << LEDR << std::endl;
			libmaus2::lcs::LocalAlignmentPrint::printAlignmentLines(std::cerr,a,b,80,LED.ta,LED.te,LEDR);
		}
		{
			libmaus2::lcs::MetaLocalEditDistance< ::libmaus2::lcs::diag_del_ins > LED;
			std::string const a = "XXAABAAY";
			std::string const b = "ZAAAAZZ";
			libmaus2::lcs::LocalEditDistanceResult LEDR = LED.process(
				a.begin(),a.size(),
				b.begin(),b.size(),
				// a.size()
				2*(std::max(a.size(),b.size())-std::min(a.size(),b.size()))
			);

			std::cerr << LEDR << std::endl;
			libmaus2::lcs::LocalAlignmentPrint::printAlignmentLines(std::cerr,a,b,80,LED.ta,LED.te,LEDR);
		}

		runtestshort< ::libmaus2::lcs::diag_del_ins >();
		runtestshort< ::libmaus2::lcs::del_ins_diag >();

		runtestmeta< ::libmaus2::lcs::diag_del_ins >();
		runtestconcrete< ::libmaus2::lcs::diag_del_ins >();

		runtestmeta< ::libmaus2::lcs::del_ins_diag >();
		runtestconcrete< ::libmaus2::lcs::del_ins_diag >();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
