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
#include <libmaus2/lcs/SimdX86GlobalAlignmentX128_8.hpp>

#include <libmaus2/lcs/HashContainer2.hpp>
#include <libmaus2/fastx/FastAReader.hpp>
#include <libmaus2/util/ArgInfo.hpp>

#include <algorithm>

#include <libmaus2/lcs/NP.hpp>

static std::string loadFirstPattern(std::string const & filename)
{
	::libmaus2::fastx::FastAReader fa(filename);
	::libmaus2::fastx::FastAReader::pattern_type pattern;
	if ( fa.getNextPatternUnlocked(pattern) )
	{
		return pattern.spattern;
	}
	else
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "Failed to read first pattern from file " << filename << std::endl;
		se.finish();
		throw se;
	}
}

#include <libmaus2/lcs/BitVector.hpp>

struct BitVectorResultPrintCallback : public ::libmaus2::lcs::BitVectorResultCallbackInterface
{
	std::ostream & out;
	
	BitVectorResultPrintCallback(std::ostream & rout) : out(rout)
	{
	}
	
	void operator()(size_t const p, uint64_t const distance)
	{
		out << "BitVector::match: end position " << p << " distance " << distance << std::endl;
	}				
};

#include <libmaus2/lcs/AlignerFactory.hpp>
#include <libmaus2/lcs/BandedAlignerFactory.hpp>
#include <libmaus2/lcs/AlignmentPrint.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>

#include <csignal>
volatile bool timerexpired = false;
void sigalrm(int)
{
	timerexpired = true;
}

#include <libmaus2/math/Rational.hpp>
#include <libmaus2/math/BernoulliNumber.hpp>
#include <libmaus2/math/Faulhaber.hpp>

#include <libmaus2/lcs/EditDistance.hpp>

int main(int argc, char * argv[])
{
	{
		#if 0
		std::string const a = "hamstetr";
		std::string const b = "hramster";
		#endif

		std::string a =  "GCAGNGTGGAAAGCACCGCAAATCACATTTACGAAAAAGCTCTGTTAACCCCGATTTAGGTGGCGACATTCCCCTTGACATAATAAAGTCTGTACCAAGAG";
		std::string b = "TGCAGNCTGGAAGCACCGCAAAAATCAAAATTTACGAAAAAGTCGTCTGTTAACCCGATGTTAGGTGCCGGAAACTTTCCCCTTGACTAATAAAGTCTGTACAGAG";

		#if 0
		{
		libmaus2::lcs::EditDistance<> ED;
		ED.process(a.begin(),a.size(),b.begin(),b.size(),0,0,1,1,1);
		libmaus2::lcs::AlignmentPrint::printAlignmentLines(std::cout,a.begin(),a.size(),b.begin(),b.size(),80,ED.ta,ED.te);
		}
		#endif
		
		libmaus2::lcs::NP np;
		
		std::cerr << np.np(a.begin(),a.end(),b.begin(),b.end()) << std::endl;
		libmaus2::lcs::AlignmentPrint::printAlignmentLines(std::cout,a.begin(),a.size(),b.begin(),b.size(),80,np.ta,np.te);
		
		#if 0
		std::string::const_iterator aa = a.begin();
		std::string::const_iterator ae = a.end();
		std::string::const_iterator ba = b.begin();
		std::string::const_iterator be = b.end();
		#endif
		
		char const * aa = a.c_str();
		char const * ae = aa + a.size();
		char const * ba = b.c_str();
		char const * be = ba + b.size();
	
		libmaus2::timing::RealTimeClock rtc; rtc.start();
		int d = 0;	
		uint64_t n = 5*1024*1024;
		for ( size_t i = 0; i < n; ++i )
			d += np.np(aa,ae,ba,be);
			
		double const ela = rtc.getElapsedSeconds();
		
		std::cerr << "d=" << d << " " <<  n/ela << std::endl;
		
		libmaus2::lcs::EditDistance<> ED;
		ED.process(a.begin(),a.size(),b.begin(),b.size(),0,0,1,1,1);
		std::cerr << ED.getAlignmentStatistics() << std::endl;
	}

	try
	{
		libmaus2::math::Rational<> R1(1);
		libmaus2::math::Rational<> R2(2);
		
		std::cerr << "R1=" << R1 << std::endl;
		std::cerr << "R2=" << R2 << std::endl;
		std::cerr << "R1+R2=" << R1+R2 << std::endl;
		std::cerr << "R1-R2=" << R1-R2 << std::endl;
		std::cerr << "R1*R2=" << R1*R2 << std::endl;
		std::cerr << "R1/R2=" << R1/R2 << std::endl;
		
		R1 *= R2;
		std::cerr << "R1 after R1*=R2 = " << R1 << std::endl;
		R1 /= R2;
		std::cerr << "R1 after R1/=R2 = " << R1 << std::endl;
		R1 /= R2;
		std::cerr << "R1 after R1/=R2 = " << R1 << std::endl;

		libmaus2::math::Rational<> R3(2,3);
		libmaus2::math::Rational<> R4(5,4);
		
		std::cerr << static_cast<double>(R3) << std::endl;
		std::cerr << static_cast<double>(R4) << std::endl;
		std::cerr << static_cast<double>(R3)+static_cast<double>(R4) << " " << static_cast<double>(R3+R4) << std::endl;
		std::cerr << static_cast<double>(R4)+static_cast<double>(R3) << " " << static_cast<double>(R4+R3) << std::endl;
		std::cerr << static_cast<double>(R3)-static_cast<double>(R4) << " " << static_cast<double>(R3-R4) << std::endl;
		std::cerr << static_cast<double>(R4)-static_cast<double>(R3) << " " << static_cast<double>(R4-R3) << std::endl;
		std::cerr << static_cast<double>(R3)*static_cast<double>(R4) << " " << static_cast<double>(R3*R4) << std::endl;
		std::cerr << static_cast<double>(R4)*static_cast<double>(R3) << " " << static_cast<double>(R4*R3) << std::endl;
		std::cerr << static_cast<double>(R3)/static_cast<double>(R4) << " " << static_cast<double>(R3/R4) << std::endl;
		std::cerr << static_cast<double>(R4)/static_cast<double>(R3) << " " << static_cast<double>(R4/R3) << std::endl;
		
		
		for ( uint64_t i = 0; i <= 24; ++i )
		{
			std::cerr << "B(" << i << ")=" << libmaus2::math::BernoulliNumber::B(i) << std::endl;
		}
		
		for ( uint64_t i = 0; i < 20; ++i )
		{
			std::vector < libmaus2::math::Rational<> > const P = libmaus2::math::Faulhaber::polynomial(i);
			for (uint64_t j = 0; j<P.size(); ++j )
				std::cerr << P[j] << ";";
			std::cerr << std::endl;
		}
		
		/*
		 * T(d,0) = 1
		 * T(d,i>0) = T(d-1,i) + 2 * \sum_{j=1}{i-1} T(d-1,j) + 2
		 */
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		{
			std::set<libmaus2::lcs::AlignerFactory::aligner_type> const sup = libmaus2::lcs::AlignerFactory::getSupportedAligners();
			std::string text =  "GCAGNGTGGAAAGCACCGCAAATCACATTTACGAAAAAGCTCTGTTAACCCCGATTTAGGTGGCGACATTCCCCTTGACATAATAAAGTCTGTACCAAGAG";
			uint8_t const * t = reinterpret_cast<uint8_t const *>(text.c_str());
			size_t const tn = text.size();
			std::string query = "TGCAGNCTGGAAGCACCGCAAAAATCAAAATTTACGAAAAAGTCGTCTGTTAACCCGATGTTAGGTGCCGGAAACTTTCCCCTTGACTAATAAAGTCTGTACAGAG";
			uint8_t const * q = reinterpret_cast<uint8_t const *>(query.c_str());
			size_t const qn = query.size();
			libmaus2::timing::RealTimeClock rtc;
			std::map < libmaus2::lcs::AlignerFactory::aligner_type, double > R;
			
			for ( std::set<libmaus2::lcs::AlignerFactory::aligner_type>::const_iterator ita = sup.begin(); ita != sup.end(); ++ita )
			{
				libmaus2::lcs::AlignerFactory::aligner_type const type = *ita;
				std::cerr << "\naligner type " << type << "\n" << std::endl;
				libmaus2::lcs::Aligner::unique_ptr_type Tptr(
					libmaus2::lcs::AlignerFactory::construct(type)
				);
				Tptr->align(t,tn,q,qn);
	
				libmaus2::lcs::AlignmentTraceContainer const & trace = Tptr->getTraceContainer();
				libmaus2::lcs::AlignmentPrint::printAlignmentLines(std::cout,text.begin(),text.size(),query.begin(),query.size(),80,trace.ta,trace.te);
				std::cerr << trace.getAlignmentStatistics() << std::endl;
				
				uint64_t const b = 1024;
				uint64_t n = 0;
				timerexpired = false;
				void (*oldhandler)(int) = signal(SIGALRM,sigalrm);
				alarm(5);
				rtc.start();
				while ( ! timerexpired )
				{
					for ( uint64_t i = 0; i < b; ++i )
						Tptr->align(t,tn,q,qn);			
					n += b;
				}
				double const sec = rtc.getElapsedSeconds();
				signal(SIGALRM,oldhandler);
				double const rate = n /sec;
				R[type] = rate;
				std::cerr << rate << " alns/s" << std::endl;
			}
						
			for ( std::map < libmaus2::lcs::AlignerFactory::aligner_type, double >::const_iterator ita = R.begin();
				ita != R.end(); ++ita )
			{
				std::cerr << ita->first << "\t" << ita->second << std::endl;
			}
		}

		{
			std::set<libmaus2::lcs::BandedAlignerFactory::aligner_type> const sup = libmaus2::lcs::BandedAlignerFactory::getSupportedAligners();
			std::string text =  "GCAGNGTGGAAAGCACCGCAAATCACATTTACGAAAAAGCTCTGTTAACCCCGATTTAGGTGGCGACATTCCCCTTGACATAATAAAGTCTGTACCAAGAG";
			uint8_t const * t = reinterpret_cast<uint8_t const *>(text.c_str());
			size_t const tn = text.size();
			std::string query = "TGCAGNCTGGAAGCACCGCAAAAATCAAAATTTACGAAAAAGTCGTCTGTTAACCCGATGTTAGGTGCCGGAAACTTTCCCCTTGACTAATAAAGTCTGTACAGAG";
			uint8_t const * q = reinterpret_cast<uint8_t const *>(query.c_str());
			size_t const qn = query.size();
			libmaus2::timing::RealTimeClock rtc;
			std::map < libmaus2::lcs::BandedAlignerFactory::aligner_type, double > R;
			
			for ( std::set<libmaus2::lcs::BandedAlignerFactory::aligner_type>::const_iterator ita = sup.begin(); ita != sup.end(); ++ita )
			{
				libmaus2::lcs::BandedAlignerFactory::aligner_type const type = *ita;
				std::cerr << "\naligner type " << type << "\n" << std::endl;
				libmaus2::lcs::BandedAligner::unique_ptr_type Tptr(
					libmaus2::lcs::BandedAlignerFactory::construct(type)
				);
				Tptr->align(t,tn,q,qn,8);
	
				libmaus2::lcs::AlignmentTraceContainer const & trace = Tptr->getTraceContainer();
				libmaus2::lcs::AlignmentPrint::printAlignmentLines(std::cout,text.begin(),text.size(),query.begin(),query.size(),80,trace.ta,trace.te);
				std::cerr << trace.getAlignmentStatistics() << std::endl;
				
				uint64_t const b = 1024;
				uint64_t n = 0;
				timerexpired = false;
				void (*oldhandler)(int) = signal(SIGALRM,sigalrm);
				alarm(5);
				rtc.start();
				while ( ! timerexpired )
				{
					for ( uint64_t i = 0; i < b; ++i )
						Tptr->align(t,tn,q,qn,8);
					n += b;
				}
				double const sec = rtc.getElapsedSeconds();
				signal(SIGALRM,oldhandler);
				double const rate = n /sec;
				R[type] = rate;
				std::cerr << rate << " alns/s" << std::endl;
			}
			
			for ( std::map < libmaus2::lcs::BandedAlignerFactory::aligner_type, double >::const_iterator ita = R.begin();
				ita != R.end(); ++ita )
			{
				std::cerr << ita->first << "\t" << ita->second << std::endl;
			}
		}
	
		{
			std::string const text = "remachine";
			std::string const query = "match";
			
			BitVectorResultPrintCallback CB(std::cerr);
			
			libmaus2::lcs::BitVector<uint64_t>::bitvector(
				text.begin(),text.size(),
				query.begin(),query.size(),
				CB,
				3
			);
		}
	
		{
			int A[] = { 1,2,2,3,0,1,2,3 };
			int const n = sizeof(A)/sizeof(A[0]);
			libmaus2::util::LongestIncreasingSubsequenceExtendedResult::unique_ptr_type const M = libmaus2::util::LongestIncreasingSubsequence<int const *>::longestIncreasingSubsequenceExtended(&A[0],n);
			std::cerr << *M << std::endl;
		}

		::libmaus2::util::ArgInfo const arginfo(argc,argv);
		std::string const a = loadFirstPattern(arginfo.getRestArg<std::string>(0));
		std::string const b = loadFirstPattern(arginfo.getRestArg<std::string>(1));
		bool const inva = arginfo.getValue<bool>("inva",false);
		bool const invb = arginfo.getValue<bool>("invb",false);
		#if 0
		double const maxindelfrac = arginfo.getValue<bool>("maxindelfrac",0.05);
		double const maxsubstfrac = arginfo.getValue<bool>("maxsubstfrac",0.05);
		#endif

		std::string const A = inva ? ::libmaus2::fastx::reverseComplementUnmapped(a) : a;
		std::string const B = invb ? ::libmaus2::fastx::reverseComplementUnmapped(b) : b;

		libmaus2::lcs::HashContainer2 HC(4 /* kmer size */,A.size());
		HC.process(A.begin());

		::libmaus2::lcs::SuffixPrefixResult SPR = HC.match(B.begin(),B.size(),A.begin());

		HC.printAlignmentLines(
			std::cerr,
			A.begin(),
			B.begin(),
			B.size(),
			SPR,
			80
		);
		
		std::cerr << SPR << std::endl;

	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
