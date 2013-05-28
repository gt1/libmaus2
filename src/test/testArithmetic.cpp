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

#include <libmaus/arithmetic/ArithEncoder.hpp>
#include <libmaus/arithmetic/ArithDecoder.hpp>
#include <libmaus/arithmetic/RangeEncoder.hpp>
#include <libmaus/arithmetic/RangeDecoder.hpp>
#include <iostream>
#include <cstdlib>

#include <libmaus/bitio/BitIOOutput.hpp>
#include <libmaus/bitio/BitIOInput.hpp>
#include <libmaus/bitio/BitWriter.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/timing/RealTimeClock.hpp>



typedef ::libmaus::arithmetic::Model model_type;

template<typename bw_type>
void encodeFileArithmetic(::libmaus::autoarray::AutoArray<uint8_t> const & data, unsigned int const alph, bw_type & CB)
{
	::libmaus::timing::RealTimeClock rtc; rtc.start();

	::libmaus::arithmetic::ArithmeticEncoder < bw_type > AE(CB);
	model_type ME(alph);

	for ( uint64_t i = 0; i < data.size(); ++i )
		AE.encodeUpdate(ME,data[i]);
	AE.flush(true /* add end marker */);
	
	std::cerr << "Encoded in " << rtc.getElapsedSeconds() << " s, " 
		<< 1./(rtc.getElapsedSeconds()/data.size())
		<< std::endl;	
}

template<typename bw_type>
void encodeFileRange(::libmaus::autoarray::AutoArray<uint8_t> const & data, unsigned int const alph, bw_type & CB)
{
	::libmaus::timing::RealTimeClock rtc; rtc.start();

	::libmaus::arithmetic::RangeEncoder < bw_type > AE(CB);
	model_type ME(alph);

	for ( uint64_t i = 0; i < data.size(); ++i )
		AE.encodeUpdate(ME,data[i]);
	AE.flush(true /* add end marker */);
	
	std::cerr << "Encoded in " << rtc.getElapsedSeconds() << " s, " 
		<< 1./(rtc.getElapsedSeconds()/data.size())
		<< std::endl;	
}

void testArithmetic(std::string const & filename)
{
	::libmaus::autoarray::AutoArray<uint8_t> data = ::libmaus::util::GetFileSize::readFile(filename);
	unsigned int const alph = 8;
	for ( uint64_t i = 0; i < data.size(); ++i )
		data[i] &= (alph-1);

	::libmaus::timing::RealTimeClock rtc; rtc.start();

	typedef ::libmaus::bitio::BitWriterVector8 bw_type;
	std::vector < uint8_t > VV;
	std::back_insert_iterator< std::vector < uint8_t > > it(VV);
	bw_type CB(it);

	unsigned int const loops = 3;
	
	for ( uint64_t z = 0; z < loops; ++z )
		encodeFileArithmetic(data,alph,CB);

	typedef ::libmaus::bitio::IteratorBitInputStream< std::vector<uint8_t>::const_iterator, true > br_type;
	br_type CR(VV.begin(),VV.end());

	for ( uint64_t z = 0; z < loops; ++z )
	{
		rtc.start();	
		::libmaus::arithmetic::ArithmeticDecoder < br_type > AD (CR);
		model_type MD(alph);
	
		::libmaus::autoarray::AutoArray<uint8_t> ddata(data.size(),false);
		for ( uint64_t i = 0; i < data.size(); ++i )
			ddata[i] = AD.decodeUpdate(MD);
		AD.flush(true /* read end marker */);
			
		std::cerr << "Decoded in " << rtc.getElapsedSeconds() << " s, "
			<< 1./(rtc.getElapsedSeconds()/data.size());

		bool ok = true;
		for ( uint64_t i = 0; i < data.size(); ++i )
			ok = ok && (data[i] == ddata[i]);
			
		std::cout << " " << (ok ? "ok" : "failed") << std::endl;
	}

	std::cerr << static_cast<double>(loops*data.size()) / VV.size() << std::endl;

}

void testRange(std::string const & filename)
{
	::libmaus::autoarray::AutoArray<uint8_t> data = ::libmaus::util::GetFileSize::readFile(filename);
	unsigned int const alph = 256;
	for ( uint64_t i = 0; i < data.size(); ++i )
		data[i] &= (alph-1);

	::libmaus::timing::RealTimeClock rtc; rtc.start();
	unsigned int const loops = 3;
	std::ostringstream ostr;
	
	for ( unsigned int l = 0; l < loops; ++l )
	{
		::libmaus::arithmetic::RangeEncoder<std::ostream> RE(ostr);
		model_type MDenc(alph);
		for ( uint64_t i = 0; i < data.size(); ++i )
			RE.encodeUpdate(MDenc,data[i]);
		RE.flush();
	}
	
	double const enctime = rtc.getElapsedSeconds();
	std::cerr << "encoding time " << enctime/loops << " compression " << 
		(
			static_cast<double>(3*data.size()) / ostr.str().size()
		) 
	<< std::endl;
	
	bool ok = true;
	std::istringstream istr(ostr.str());
	rtc.start();
	
	for ( unsigned int l = 0; ok && l < loops; ++l )
	{
		model_type MDdec(alph);
		::libmaus::arithmetic::RangeDecoder<std::istream> RD(istr);
		::libmaus::autoarray::AutoArray<uint8_t> ddata(data.size(),false);
		for ( uint64_t i = 0; i < data.size(); ++i )
		{
			ddata[i] = RD.decodeUpdate(MDdec);
			ok = ok && (data[i] == ddata[i]);
		}
	}
	double const dectime = rtc.getElapsedSeconds();
	std::cerr << "decoding time " << dectime/loops << std::endl;
		
	std::cerr << "range coder " << (ok ? "ok" : "failed") << std::endl;
}

int main(int argc, char * argv[])
{
	if ( argc < 2 )
	{
		std::cerr << "usage: " << argv[0] << " <in>" << std::endl;
		return EXIT_FAILURE;
	}

	std::string const filename = argv[1];	
	testArithmetic(filename);
	testRange(filename);
}
