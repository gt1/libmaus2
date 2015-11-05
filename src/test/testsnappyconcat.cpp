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

#include <libmaus2/lz/SimpleCompressedStreamInterval.hpp>

#include <libmaus2/lz/SnappyCompressorObjectFactory.hpp>
#include <libmaus2/lz/SnappyDecompressorObjectFactory.hpp>

#include <libmaus2/lz/SimpleCompressedInputStream.hpp>
#include <libmaus2/lz/SimpleCompressedConcatInputStream.hpp>
#include <libmaus2/lz/SimpleCompressedOutputStream.hpp>

#include <iostream>

void testSingle(std::string const & s0)
{
	uint64_t const l0 = s0.size();
	libmaus2::autoarray::AutoArray<char> C(s0.size(),false);

	for ( uint64_t bs = 1; bs <= 16; ++bs )
		for ( uint64_t b_0 = 0; b_0 <= l0; ++b_0 )
			for ( uint64_t b_1 = 0; b_1 <= l0-b_0; ++b_1 )
			{
				std::ostringstream o_0;
				libmaus2::lz::SnappyCompressorObjectFactory scompfact;
				libmaus2::lz::SimpleCompressedOutputStream<std::ostream> lzout_0(o_0,scompfact,bs);

				lzout_0.write(s0.c_str(),b_0);
				std::pair<uint64_t,uint64_t> start_0 = lzout_0.getOffset();
				lzout_0.write(s0.c_str()+b_0,b_1);
				// std::pair<uint64_t,uint64_t> end_0 =
					lzout_0.getOffset();
				lzout_0.write(s0.c_str()+b_0+b_1,s0.size()-(b_0+b_1));
				lzout_0.flush();

				std::string const u0 = s0.substr(b_0,b_1);

				std::istringstream i_0(o_0.str());
				libmaus2::lz::SnappyDecompressorObjectFactory decompfact;
				libmaus2::lz::SimpleCompressedInputStream<std::istream> decomp(i_0,decompfact,start_0);

				uint64_t j = 0;
				for ( uint64_t i = 0; i < u0.size(); ++i )
				{
					int const c = decomp.get();
					assert ( c == u0[j++] );
				}

				// uint64_t const r =
					decomp.read(C.begin(),l0-(b_0+b_1));
				assert ( decomp.get() == std::istream::traits_type::eof() );
			}
}

void testConcat(std::string const & s0, std::string const & s1)
{
	uint64_t const l0 = s0.size();
	uint64_t const l1 = s1.size();

	for ( uint64_t bs = 1; bs <= 16; ++bs )
		for ( uint64_t b_0 = 0; b_0 <= l0; ++b_0 )
			for ( uint64_t b_1 = 0; b_1 <= l0-b_0; ++b_1 )
				for ( uint64_t c_0 = 0; c_0 <= l1; ++c_0 )
					for ( uint64_t c_1 = 0; c_1 <= l1-c_0; ++c_1 )
					{
						#if 0
						std::cerr
							<< "b_0=" << b_0 << " b_1=" << b_1 << " "
							<< "c_0=" << c_0 << " c_1=" << c_1 << " "
							<< "bs=" << bs
							<< std::endl;
						#endif

						std::ostringstream o_0, o_1;
						libmaus2::lz::SnappyCompressorObjectFactory scompfact;
						libmaus2::lz::SimpleCompressedOutputStream<std::ostream> lzout_0(o_0,scompfact,bs);
						libmaus2::lz::SimpleCompressedOutputStream<std::ostream> lzout_1(o_1,scompfact,bs);

						// std::cerr << "encoding s_0" << std::endl;
						lzout_0.write(s0.c_str(),b_0);
						std::pair<uint64_t,uint64_t> start_0 = lzout_0.getOffset();
						lzout_0.write(s0.c_str()+b_0,b_1);
						std::pair<uint64_t,uint64_t> end_0 = lzout_0.getOffset();
						lzout_0.write(s0.c_str()+b_0+b_1,s0.size()-(b_0+b_1));
						lzout_0.flush();

						std::string const u0 = s0.substr(b_0,b_1);

						// std::cerr << "encoding s_1" << std::endl;
						lzout_1.write(s1.c_str(),c_0);
						std::pair<uint64_t,uint64_t> start_1 = lzout_1.getOffset();
						lzout_1.write(s1.c_str()+c_0,c_1);
						std::pair<uint64_t,uint64_t> end_1 = lzout_1.getOffset();
						lzout_1.write(s1.c_str()+c_0+c_1,s1.size()-(c_0+c_1));
						lzout_1.flush();

						std::string const u1 = s1.substr(c_0,c_1);
						std::string const u = u0+u1;

						std::istringstream i_0(o_0.str());
						libmaus2::lz::SimpleCompressedConcatInputStreamFragment<std::istream> frag_0(start_0,end_0,&i_0);
						std::istringstream i_1(o_1.str());
						libmaus2::lz::SimpleCompressedConcatInputStreamFragment<std::istream> frag_1(start_1,end_1,&i_1);

						std::vector<libmaus2::lz::SimpleCompressedConcatInputStreamFragment<std::istream> > frags;
						frags.push_back(frag_0);
						frags.push_back(frag_1);

						libmaus2::lz::SnappyDecompressorObjectFactory decompfact;
						libmaus2::lz::SimpleCompressedConcatInputStream<std::istream> conc(frags,decompfact);

						#if 0
						std::cerr << "expecting " << u << std::endl;
						std::cerr << "write     ";
						std::cerr.write(s0.c_str()+b_0,b_1);
						std::cerr.write(s1.c_str()+c_0,c_1);
						std::cerr << std::endl;

						std::cerr << "frags[0]="
							<< frags[0].low.first << ","
							<< frags[0].low.second << ","
							<< frags[0].high.first << ","
							<< frags[0].high.second << std::endl;
						std::cerr << "frags[1]="
							<< frags[1].low.first << ","
							<< frags[1].low.second << ","
							<< frags[1].high.first << ","
							<< frags[1].high.second << std::endl;
						#endif

						int c = -1;
						uint64_t j = 0;
						while ( (c = conc.get()) != -1 )
						{
							// std::cout.put(c);
							assert ( c == u[j++] );
						}
						assert ( j == u.size() );

						// std::cout.put('\n');
					}

}

int main()
{
	try
	{
		testSingle("schloss");

		testConcat("schloss","dagstuhl");
		testConcat("dagstuhl","schloss");
		testConcat("schloss","");
		testConcat("","schloss");
		testConcat("dagstuhl","");
		testConcat("","dagstuhl");
		testConcat("aaaaaaa","aaaaaaaaaa");
		testConcat("","aaaaaaaaaa");
		testConcat("aaaaaaa","");
		testConcat("abcdefghi","jklmnopqrs");
		testConcat("jklmnopqrs","abcdefghi");
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
