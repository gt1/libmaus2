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

#include <libmaus/huffman/IndexLoaderBase.hpp>
#include <iostream>
#include <sstream>
#include <libmaus/huffman/huffman.hpp>
#include <libmaus/math/isqrt.hpp>
#include <libmaus/wavelet/HuffmanWaveletTree.hpp>
#include <libmaus/huffman/CanonicalEncoder.hpp>
#include <libmaus/huffman/HuffmanTree.hpp>

bool checkNontrivial(std::string const & s)
{
	if ( s.size() < 2 )
		return false;
	for ( uint64_t i = 1; i < s.size(); ++i )
		if ( s[i] != s[0] )
			return true;
	return false;
}

void testRangeQuantile()
{
	std::string s(256,'a');
	
	for ( unsigned int i = 0; i < 10; ++i )
	{
		for ( unsigned int j = 0; j < s.size(); ++j )
			s[j] = 'a' + (rand() % 18);
		
		if ( !checkNontrivial(s) )
			continue;
			
		::libmaus::wavelet::HuffmanWaveletTree hwt(s.c_str(), s.c_str() + s.size());

                std::pair < std::vector < uint64_t >, std::vector < uint64_t > > SOBC = hwt.enctable.symsOrderedByCode();
		
		// hwt.enctable.printSorted();                
		
		for ( unsigned int j = 0; j < hwt.n; ++j )
			assert( hwt[j] == s[j] );
		
		std::cerr << s << std::endl;
		
		for ( unsigned int j = 0; j < hwt.n; ++j )
			std::cerr << 
				SOBC.second[ s[j] - hwt.enctable.minsym ];
		std::cerr << std::endl;
		
		hwt.toString(std::cerr);
		hwt.enctable.printSorted();
		
		if ( 1 )
		{
			std::ostringstream ostr1;
			hwt.root->serialize(ostr1);
			std::cerr << "(1) size " << ostr1.str().size() << std::endl;
			std::istringstream istr1(ostr1.str());
			::libmaus::util::shared_ptr < ::libmaus::huffman::HuffmanTreeNode >::type acopy = ::libmaus::huffman::HuffmanTreeNode::deserialize(istr1);
			std::ostringstream ostr2;
			acopy->serialize(ostr2);
			std::cerr << "(2) size " << ostr2.str().size() << std::endl;
			
			assert ( ostr1.str() == ostr2.str() );

			std::ostringstream ostr3;
			hwt.serialize(ostr3);
			std::cerr << "(3) size " << ostr3.str().size() << std::endl;
			
			std::istringstream istr3(ostr3.str());
			::libmaus::wavelet::HuffmanWaveletTree hwt3(istr3);

			std::ostringstream ostr4;
			hwt3.serialize(ostr4);
			std::cerr << "(4) size " << ostr4.str().size() << std::endl;
			
			for ( unsigned int i = 0; i < hwt.n; ++i )
				assert ( hwt[i] == hwt3[i] );
				
			assert ( ostr3.str() == ostr4.str() );
		}

		for ( unsigned int right = 0; right <= hwt.n; ++right )
			for ( unsigned int left = 0; left <= right; ++left )
			{
				std::vector < std::pair < uint64_t, uint64_t > > V;
			
				for ( uint64_t j = left; j < right; ++j )
				{
					V .push_back(
						std::pair < uint64_t, uint64_t >(
							SOBC.second[s[j]-hwt.enctable.minsym],
							s[j]
							)
						);
				}
				
				std::sort ( V.begin(), V.end() );
				
				for ( unsigned int pos = 0; pos < right-left; ++pos )
				{
					assert ( hwt.rangeQuantile(left,right,pos) == V[pos].second );
				}
			}
	}

	for ( unsigned int i = 0; i < 10; ++i )
	{
		for ( unsigned int j = 0; j < s.size()/2; ++j )
			s[(rand()%s.size())] = 'a';
		for ( unsigned int j = 0; j < s.size()/4; ++j )
			s[(rand()%s.size())] = 'b';
		for ( unsigned int j = 0; j < s.size()/8; ++j )
			s[(rand()%s.size())] = 'c';
		for ( unsigned int j = 0; j < s.size()/16; ++j )
			s[(rand()%s.size())] = 'd';
		for ( unsigned int j = 0; j < s.size()/16; ++j )
			s[(rand()%s.size())] = 'e' + (rand() % 5);

		if ( !checkNontrivial(s) )
			continue;
	
		::libmaus::wavelet::HuffmanWaveletTree hwt(s.c_str(), s.c_str() + s.size());

                std::pair < std::vector < uint64_t >, std::vector < uint64_t > > SOBC = hwt.enctable.symsOrderedByCode();
		
		// hwt.enctable.printSorted();                
		
		for ( unsigned int j = 0; j < hwt.n; ++j )
			assert( hwt[j] == s[j] );
		
		std::cerr << s << std::endl;
		
		for ( unsigned int j = 0; j < hwt.n; ++j )
			std::cerr << 
				SOBC.second[ s[j] - hwt.enctable.minsym ];
		std::cerr << std::endl;
		
		hwt.toString(std::cerr);
		hwt.enctable.printSorted();

		if ( 1 )
		{
			std::ostringstream ostr1;
			hwt.root->serialize(ostr1);
			std::cerr << "(1) size " << ostr1.str().size() << std::endl;
			std::istringstream istr1(ostr1.str());
			::libmaus::util::shared_ptr < ::libmaus::huffman::HuffmanTreeNode >::type acopy = ::libmaus::huffman::HuffmanTreeNode::deserialize(istr1);
			std::ostringstream ostr2;
			acopy->serialize(ostr2);
			std::cerr << "(2) size " << ostr2.str().size() << std::endl;
			
			assert ( ostr1.str() == ostr2.str() );

			std::ostringstream ostr3;
			hwt.serialize(ostr3);
			std::cerr << "(3) size " << ostr3.str().size() << std::endl;
			
			std::istringstream istr3(ostr3.str());
			::libmaus::wavelet::HuffmanWaveletTree hwt3(istr3);

			std::ostringstream ostr4;
			hwt3.serialize(ostr4);
			std::cerr << "(4) size " << ostr4.str().size() << std::endl;
			
			for ( unsigned int i = 0; i < hwt.n; ++i )
				assert ( hwt[i] == hwt3[i] );
				
			assert ( ostr3.str() == ostr4.str() );
		}
		
		for ( unsigned int right = 0; right <= hwt.n; ++right )
			for ( unsigned int left = 0; left <= right; ++left )
			{
				std::vector < std::pair < uint64_t, uint64_t > > V;
			
				for ( uint64_t j = left; j < right; ++j )
				{
					V .push_back(
						std::pair < uint64_t, uint64_t >(
							SOBC.second[s[j]-hwt.enctable.minsym],
							s[j]
							)
						);
				}
				
				std::sort ( V.begin(), V.end() );
				
				for ( unsigned int pos = 0; pos < right-left; ++pos )
				{
					assert ( hwt.rangeQuantile(left,right,pos) == V[pos].second );
				}
			}
	}
}

void testSmaller()
{
	std::string s(256,'a');

	for ( unsigned int i = 0; i < 10; ++i )
	{
		for ( unsigned int j = 0; j < s.size(); ++j )
			s[j] = 'a' + (rand() % 18);

		if ( !checkNontrivial(s) )
			continue;
			
		::libmaus::wavelet::HuffmanWaveletTree hwt(s.c_str(), s.c_str() + s.size());

                std::pair < std::vector < uint64_t >, std::vector < uint64_t > > SOBC = hwt.enctable.symsOrderedByCode();
		
		// hwt.enctable.printSorted();                
		
		for ( unsigned int j = 0; j < hwt.n; ++j )
			assert( hwt[j] == s[j] );
		
		std::cerr << s << std::endl;
		
		for ( unsigned int j = 0; j < hwt.n; ++j )
			std::cerr << 
				SOBC.second[ s[j] - hwt.enctable.minsym ];
		std::cerr << std::endl;
		
		hwt.toString(std::cerr);
		hwt.enctable.printSorted();
		
		for ( unsigned int right = 0; right <= hwt.n; ++right )
			for ( unsigned int left = 0; left <= right; ++left )
				for ( int sym = hwt.enctable.minsym; sym <= hwt.enctable.maxsym; ++sym )
					if ( hwt.enctable.codeused[ sym - hwt.enctable.minsym ] )
					{
						uint64_t smaller = 0;
						for ( uint64_t j = left; j < right; ++j )
							if ( SOBC.second[s[j]-hwt.enctable.minsym] < SOBC.second[sym-hwt.enctable.minsym] )
							{
								#if 0
								std::cerr << "symbol " << s[j] << " rank " << SOBC.second[s[j]-hwt.enctable.minsym]
									<< " smaller than symbol " << sym << " rank " << SOBC.second[sym-hwt.enctable.minsym] << std::endl;
								#endif
								smaller++;
							}

						// std::cerr << "*** new range ***" << " l=" << left << " r=" << right << " sym=" << static_cast<char>(sym) << " rank=" << SOBC.second[sym-hwt.enctable.minsym] << std::endl;

						uint64_t wtsmaller = hwt.smaller ( sym, left, right );
					
						// std::cerr << "left=" << left << " right=" << right << " sym=" << sym << " smaller=" << smaller << " wtsmaller=" << wtsmaller << std::endl;
						
						assert ( wtsmaller == smaller );
					}
	}

	for ( unsigned int i = 0; i < 10; ++i )
	{
		for ( unsigned int j = 0; j < s.size()/2; ++j )
			s[(rand()%s.size())] = 'a';
		for ( unsigned int j = 0; j < s.size()/4; ++j )
			s[(rand()%s.size())] = 'b';
		for ( unsigned int j = 0; j < s.size()/8; ++j )
			s[(rand()%s.size())] = 'c';
		for ( unsigned int j = 0; j < s.size()/16; ++j )
			s[(rand()%s.size())] = 'd';
		for ( unsigned int j = 0; j < s.size()/16; ++j )
			s[(rand()%s.size())] = 'e' + (rand() % 5);

		if ( !checkNontrivial(s) )
			continue;
	
		::libmaus::wavelet::HuffmanWaveletTree hwt(s.c_str(), s.c_str() + s.size());

                std::pair < std::vector < uint64_t >, std::vector < uint64_t > > SOBC = hwt.enctable.symsOrderedByCode();
		
		// hwt.enctable.printSorted();                
		
		for ( unsigned int j = 0; j < hwt.n; ++j )
			assert( hwt[j] == s[j] );
		
		std::cerr << s << std::endl;
		
		for ( unsigned int j = 0; j < hwt.n; ++j )
			std::cerr << 
				SOBC.second[ s[j] - hwt.enctable.minsym ];
		std::cerr << std::endl;
		
		hwt.toString(std::cerr);
		hwt.enctable.printSorted();
		
		for ( unsigned int right = 0; right <= hwt.n; ++right )
			for ( unsigned int left = 0; left <= right; ++left )
				for ( int sym = hwt.enctable.minsym; sym <= hwt.enctable.maxsym; ++sym )
					if ( hwt.enctable.codeused[ sym - hwt.enctable.minsym ] )
					{
						uint64_t smaller = 0;
						for ( uint64_t j = left; j < right; ++j )
							if ( SOBC.second[s[j]-hwt.enctable.minsym] < SOBC.second[sym-hwt.enctable.minsym] )
							{
								#if 0
								std::cerr << "symbol " << s[j] << " rank " << SOBC.second[s[j]-hwt.enctable.minsym]
									<< " smaller than symbol " << sym << " rank " << SOBC.second[sym-hwt.enctable.minsym] << std::endl;
								#endif
								smaller++;
							}

						// std::cerr << "*** new range ***" << " l=" << left << " r=" << right << " sym=" << static_cast<char>(sym) << " rank=" << SOBC.second[sym-hwt.enctable.minsym] << std::endl;

						uint64_t wtsmaller = hwt.smaller ( sym, left, right );
					
						// std::cerr << "left=" << left << " right=" << right << " sym=" << sym << " smaller=" << smaller << " wtsmaller=" << wtsmaller << std::endl;
						
						assert ( wtsmaller == smaller );
					}
	}
}

void runTests()
{
	std::string text = "aabbbacabbaba";
        ::libmaus::util::shared_ptr < ::libmaus::huffman::HuffmanTreeNode >::type root = ::libmaus::huffman::HuffmanBase::createTree( text.begin(), text.end() );
        ::libmaus::huffman::EncodeTable<1> enctable(root.get());
        
        std::cerr << std::hex;
        enctable.print();
        std::cerr << std::dec;

        std::cerr << "---" << std::endl;
        
        root->square(8);        
        
        ::libmaus::huffman::EncodeTable<1> enctablesq(root.get());

        std::cerr << std::hex;
        enctablesq.print();
        std::cerr << std::dec;
        
        // return 0;
	
	::libmaus::wavelet::HuffmanWaveletTree::testTree();

	testRangeQuantile();
	testSmaller();

	::libmaus::huffman::HuffmanSorting::test();

	::libmaus::wavelet::HuffmanWaveletTree::testTree();
                              
	std::string a = "aaba";
	::libmaus::wavelet::HuffmanWaveletTree H0(a.begin(),a.end());
                               
	for ( unsigned int i = 0; i < a.size(); ++i )
		assert ( H0[i] == a[i] );                                     
}

struct FlushCallback
{
	virtual ~FlushCallback() {}
	virtual void flush(uint64_t const * const A, uint64_t words) = 0;
};

struct RestBitsBlock
{
	uint64_t bits;
	uint64_t words;
	::libmaus::autoarray::AutoArray<uint64_t> block;
	::libmaus::bitio::FastWriteBitWriter8 writer;
	FlushCallback & flush;
	uint64_t bitsused;
	uint64_t bitsleft;
	
	RestBitsBlock(uint64_t rbits, FlushCallback & rflush)
	: bits(rbits), words(bits/64), block(words), writer(block.get()), flush(rflush), bitsused(0), bitsleft(bits)
	{
			
	}
	
	void writeBits(uint64_t const w, unsigned int const b)
	{
		if ( b <= bitsleft )
		{
			writer.write(w,b);
			bitsleft -= b;
			bitsused += b;
		}
		else
		{
			std::cerr << "Calling flush." << std::endl;
		
			unsigned int const w0 = bitsleft;
			unsigned int const w1 = b-w0;
			
			uint64_t const highbits = (w >> w1);
			uint64_t const lowbits = w & ((1ull << w1)-1ull);
			
			assert ( w == ( (highbits << w1) | (lowbits) ) );

			if ( w0 )
			{
				writer.write ( highbits, w0 );
				bitsused += w0;
				bitsleft -= w0;
			}
			
			writer.flush();
			flush.flush(block.get(), words);

			bitsused = 0;
			bitsleft = bits;
			
			writer. write ( lowbits , w1 );
			
			bitsused += w1;
			bitsleft -= w1;
		}
	}
};

struct ArrayFlusher : public FlushCallback
{
	std::vector<uint64_t> data;

	void flush(uint64_t const * const A, uint64_t words)
	{
		for ( uint64_t i = 0; i < words; ++i )
			data.push_back ( A[i] );
	}
};

uint64_t fib(uint64_t i)
{
	if ( i <= 2 )
	{
		return 1;
	}
	else
	{
		return fib(i-1) + fib(i-2);
	}
}

void testFib(uint64_t a)
{
	std::map<int64_t,uint64_t> F;
	uint64_t n = 0;
	for ( uint64_t i = 0; i < a; ++i )
	{
		F[i] = fib(i);
		n += F[i];
	}

	::libmaus::util::shared_ptr < ::libmaus::huffman::HuffmanTreeNode >::type aroot = ::libmaus::huffman::HuffmanBase::createTree(F);

	::libmaus::huffman::EncodeTable<2> enctable(aroot.get());
	
	uint64_t t = 0;
	for ( uint64_t i = 0; i < a; ++i )
		t += F.find(i)->second * enctable[i].second;
	
	std::cerr << "a=" << a << " total bits " << t << " avg " << static_cast<double>(t)/n << std::endl;
}

int testHwtSimpleSerialisation()
{
	try
	{
		std::string text = "abracadabra";
		::libmaus::wavelet::HuffmanWaveletTree  HWT(text.begin(),text.end());
		
		{
		std::ostringstream ostr0;
		HWT.root->lineSerialise(ostr0);
		std::istringstream istr0(ostr0.str());
		::libmaus::util::shared_ptr< ::libmaus::huffman::HuffmanTreeNode >::type sroot0 = ::libmaus::huffman::HuffmanTreeNode::simpleDeserialise(istr0);
	
		std::ostringstream ostr1;
		sroot0->lineSerialise(ostr1);
	
		std::cerr << ostr0.str();
		std::cerr << "---" << std::endl;
		std::cerr << ostr1.str();
		
		assert ( ostr0.str() == ostr1.str() );
		}

		{
		std::ostringstream ostr0;
		HWT.navroot->lineSerialise(ostr0);
		std::istringstream istr0(ostr0.str());
		::libmaus::wavelet::HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode::unique_ptr_type uroot0 = ::libmaus::wavelet::HuffmanWaveletTree::HuffmanWaveletTreeNavigationNode::deserialiseSimple(istr0);
		
		std::ostringstream ostr1;
		uroot0->lineSerialise(ostr1);

		std::cerr << ostr0.str();
		std::cerr << "---" << std::endl;
		std::cerr << ostr1.str();
		
		assert ( ostr0.str() == ostr1.str() );
		}		

		{
		std::ostringstream ostr0;
		HWT.simpleSerialise(ostr0);
		std::istringstream istr0(ostr0.str());
		::libmaus::wavelet::HuffmanWaveletTree::unique_ptr_type uroot0 = ::libmaus::wavelet::HuffmanWaveletTree::simpleDeserialise(istr0);
		
		for ( uint64_t i = 0; i < HWT.n; ++i )
			assert ( HWT[i] == (*uroot0)[i] );
		
		}
		
		return EXIT_SUCCESS;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}

void testFibonacciTables()
{
	for ( uint64_t a = 2; a < 40; ++a )
		testFib(a);

	
}

#include <libmaus/aio/GenericInput.hpp>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/huffman/RLEncoder.hpp>
#include <libmaus/huffman/RLDecoder.hpp>

#include <libmaus/huffman/GapEncoder.hpp>
#include <libmaus/huffman/GapDecoder.hpp>


uint64_t fibo(uint64_t i)
{
	if ( i < 2 )
		return 1;
	
	uint64_t f_0 = 1, f_1 = 1;
	for ( uint64_t j = 2; j <= i; ++j )
	{
		f_1 = f_0+f_1;
		std::swap(f_0,f_1);
	}
	
	return f_0;
}

void testHuffmanTree()
{
	// std::cerr << sizeof(HuffmanNode) << std::endl;
	
	std::map<int64_t,uint64_t> M;
	#if 1
	for ( uint64_t i = 0; i <= 20; ++i )
		M[i] = fibo(i);
	#else
	for ( uint64_t i = 0; i < 15; ++i )
		M[i]  = 1;
	#endif
	libmaus::huffman::HuffmanTree H(M.begin(),M.size(),true,true);
	// std::cerr << H;
	// H.printLeafCodes(std::cerr);
	
	std::string const ser = H.serialise();
	std::istringstream istr(ser);
	libmaus::huffman::HuffmanTree H2(istr);
	assert ( H == H2 );
	#if 0
	H2.testComputeSubTreeCounts();
	H2.assignDfsIds();
	std::cerr << "start dfs" << std::endl;
	std::cerr << H2;
	H2.testAssignDfsIds();
	#endif
	
	libmaus::huffman::HuffmanTree H3(M.begin(),M.size(),true /* sort leafs by depth */,true /* assign codes */,true /* order by inner nodes by dfs */);
	std::cerr << "start reordered: " << H3.root()-H3.leafs() << std::endl;
	std::cerr << H3;
		
	std::ostringstream ostrout;
	libmaus::bitio::BitWriterStream8 BWS8(ostrout);
	libmaus::huffman::HuffmanTree::EncodeTable E(H);
	for ( uint64_t i = 0; i < 16; ++i )
	{
		int64_t const sym = i % 15;
		BWS8.write(E.getCode(sym),E.getCodeLength(sym));
	}
	BWS8.flush();
	
	std::istringstream bitistr(ostrout.str());
	libmaus::bitio::StreamBitInputStream BIN(bitistr);
	for ( uint64_t i = 0; i < 16; ++i )
	{
		// std::cerr << H.decodeSlow(BIN) << std::endl;
		assert ( H.decodeSlow(BIN) == i % 15 );
	}
}

int main()
{
	testHuffmanTree();	
	
	return 0;

	::libmaus::util::Histogram hist;
	unsigned int seq[] = { 0,1,1,0,1,2,4,1,3,1,5,1,1,1 };
	uint64_t const seqn = sizeof(seq)/sizeof(seq[0]);
	for ( uint64_t i = 0; i < seqn; ++i )
		hist(seq[i]);
	std::string const fn = "debug";
	::libmaus::huffman::GapEncoder GE(fn,hist,seqn);
	GE.encode(&seq[0],&seq[seqn]);
	
	::libmaus::huffman::GapDecoder GD(std::vector<std::string>(1,fn));

	#if 1
	{
		std::string const s = "fischersfritzefischtfrischefischeveronikaderlenzistda";
		::libmaus::huffman::CanonicalEncoder CE(s.begin(),s.end());
		::libmaus::huffman::HuffmanTreeNode::unique_ptr_type root = CE.codeToTree();
		root->lineSerialise(std::cerr);
		
		CE.print(std::cerr);
		
		::libmaus::huffman::EncodeTable<2> E(root.get());
		E.printChar();
		
		for ( uint64_t i = 0; i < s.size(); ++i )
		{
			uint8_t c = s[i];
			assert ( CE.getCode(c).second == E[c].second );
			assert ( CE.getCode(c).first == E[c].first.A[0] );
		}
		
		// return 0;
	}

	#if 0
	{
		std::string const testfilename = "testfile";
		::libmaus::huffman::RLEncoder::unique_ptr_type RLE(new ::libmaus::huffman::RLEncoder(testfilename,1ull << 50,64*1024));
		RLE->encode(5);
		RLE->flush();
		RLE.reset();
		
		::libmaus::huffman::RLDecoder rldec(testfilename);	
	}
	#endif

	srand(time(0));
	
	::libmaus::autoarray::AutoArray<uint8_t> A = ::libmaus::aio::GenericInput<uint8_t>::readArray("mis");
	std::map<int64_t,uint64_t> F;
	for ( uint64_t i = 0; i < A.size(); ++i )
		F [ A[i] ] ++;
	
	::libmaus::huffman::CanonicalEncoder cane(F,12);
	cane.print();
	
	std::ostringstream ostr;
	typedef libmaus::bitio::FastWriteBitWriterStream8 writer_type;
	writer_type writer(ostr);
	
	::libmaus::timing::RealTimeClock rtc; rtc.start();
	for ( uint64_t i = 0; i < A.size(); ++i )
		cane.encode ( writer, A[i] );
	writer.flush();
	for ( uint64_t i = 0; i < A.size(); ++i )
		cane.encode ( writer, A[i] );
	writer.flush();
	std::cerr << "Encode time " << rtc.getElapsedSeconds() << std::endl;
	
	typedef libmaus::bitio::StreamBitInputStream reader_type;
	std::istringstream istr(ostr.str());
	reader_type reader(istr);

	rtc.start();	
	for ( uint64_t i = 0; i < A.size(); ++i )
		assert ( cane.slowDecode(reader) == A[i] );
	reader.flush();
	for ( uint64_t i = 0; i < A.size(); ++i )
		assert ( cane.slowDecode(reader) == A[i] );
	reader.flush();
	std::cerr << "Slow " << rtc.getElapsedSeconds() << std::endl;

	// std::cerr << "Here." << std::endl;

	std::istringstream bibistr(ostr.str());
	::libmaus::huffman::FileStreamBaseType::unique_ptr_type FSBT(new ::libmaus::huffman::FileStreamBaseType(bibistr));
	::libmaus::huffman::BitInputBuffer4 BIB(FSBT,32);

	rtc.start();
	for ( uint64_t i = 0; i < A.size(); ++i )
	{
		uint8_t sym = cane.fastDecode(BIB);
		// std::cout << sym;
		assert ( sym == A[i] );
	}
	BIB.flush();
	for ( uint64_t i = 0; i < A.size(); ++i )
	{
		uint8_t sym = cane.slowDecode(BIB);
		// std::cout << sym;
		assert ( sym == A[i] );
	}
	BIB.flush();
	std::cerr << "Fast " << rtc.getElapsedSeconds() << std::endl;
	// std::cout << std::endl;
	
	#if 0
	testHwtSimpleSerialisation();
	testFibonacciTables();
	runTests();

	#if 1
	std::string text = "abracadabra";
	std::string::const_iterator ita = text.begin(), itb = text.end();
	::libmaus::util::shared_ptr < ::libmaus::huffman::HuffmanTreeNode >::type aroot = ::libmaus::huffman::HuffmanBase::createTree(ita,itb);
	uint64_t const n = text.size();
	::libmaus::autoarray::AutoArray < uint64_t > code = hufEncodeString (ita,itb, aroot.get());

	uint64_t const declookup = 8;
	::libmaus::huffman::DecodeTable dectable(aroot.get(), declookup);
	::libmaus::huffman::EncodeTable<2> enctable(aroot.get());
	
	enctable.printChar();
	
	uint64_t const codelen = enctable.getCodeLength(ita,itb);
	
	std::cerr << "Code length " << codelen << std::endl;
	uint64_t const abps = (codelen + (n-1)) / n;
	uint64_t const rawbitblocksize = ::libmaus::math::isqrt(n * abps);
	uint64_t const bitblocksize = ((rawbitblocksize+63)/64)*64;
	
	std::cerr << "Average code length " << abps << std::endl;
	std::cerr << "Raw bit block size " << rawbitblocksize << std::endl;
	std::cerr << "Bit block size " << bitblocksize << std::endl;
	
	uint64_t boff = 0;
	unsigned int tableid = 0;
	uint64_t decsyms = 0;
	
	ArrayFlusher AF;
	RestBitsBlock DBB(bitblocksize, AF);
	RestBitsBlock RBB(bitblocksize, AF);
	
	while ( decsyms < text.size() )
	{
		uint8_t const bits = ::libmaus::bitio::getBits(code.get(), boff, declookup);
		boff += declookup;
		::libmaus::huffman::DecodeTableEntry const & decentry = dectable(tableid,bits);
		std::vector<int> const & symbols = decentry.symbols;
		
		for ( unsigned int i = 0; i < std::min(static_cast<uint64_t>(symbols.size()), text.size()-decsyms); ++i )
		{
			std::pair< ::libmaus::uint::UInt<2> , unsigned int > L = enctable.getNonMsbBits(symbols[i]);
			bool const H = enctable.getBitFromTop(symbols[i],0);
			
			std::cerr << "(" << "[" << H << "]" << static_cast<char>(symbols[i]) << ")"
				<< " ";
			print(std::cerr, L.first, L.second);
			std::cerr <<std::endl;
			
			
			DBB.writeBits( H, 1 );
			RBB.writeBits( L.first, L.second );
		}

		decsyms += symbols.size();
		tableid = decentry.nexttable;
	}
	std::cerr << std::endl;
	#endif
	#endif
	#endif
}
