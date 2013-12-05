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
#include <vector>
#include <sstream>
#include <cstdlib>
#include <libmaus/timing/RealTimeClock.hpp>
#include <libmaus/gamma/GammaEncoder.hpp>
#include <libmaus/gamma/GammaDecoder.hpp>

#include <libmaus/gamma/SparseGammaGapEncoder.hpp>
#include <libmaus/gamma/SparseGammaGapDecoder.hpp>
#include <libmaus/gamma/SparseGammaGapMerge.hpp>
#include <libmaus/gamma/SparseGammaGapFile.hpp>
#include <libmaus/gamma/SparseGammaGapFileSet.hpp>
#include <libmaus/gamma/SparseGammaGapFileLevelSet.hpp>

template<typename T>
struct VectorPut : public std::vector<T>
{
	VectorPut() {}
	void put(T const v)
	{
		std::vector<T>::push_back(v);
	}
};

struct CountPut
{
	uint64_t cnt;

	CountPut()
	: cnt(0)
	{
	
	}
	
	template<typename T>
	void put(T const)
	{
		++cnt;	
	}
};

template<typename T>
struct VectorGet
{
	typename std::vector<T>::const_iterator it;
	
	VectorGet(typename std::vector<T>::const_iterator rit)
	: it(rit)
	{}
	
	uint64_t get()
	{
		return *(it++);
	}
};

void testRandom(unsigned int const n)
{
	srand(time(0));
	std::vector<uint64_t> V(n);
	for ( uint64_t i = 0; i <n; ++i )
		V[i] = rand();

	::libmaus::timing::RealTimeClock rtc; 
	
	rtc.start();
	CountPut CP;
	::libmaus::gamma::GammaEncoder< CountPut > GCP(CP);	
	for ( uint64_t i = 0; i < n; ++i )
		GCP.encode(V[i]);
	GCP.flush();
	double const cencsecs = rtc.getElapsedSeconds();
	std::cerr << "[V] count encoded " << n << " numbers in time " << cencsecs 
		<< " rate " << (n / cencsecs)/(1000*1000) << " m/s"
		<< " output words " << CP.cnt
		<< std::endl;
	
	VectorPut<uint64_t> VP;
	rtc.start();
	::libmaus::gamma::GammaEncoder< VectorPut<uint64_t> > GE(VP);	
	for ( uint64_t i = 0; i < n; ++i )
		GE.encode(V[i]);
	GE.flush();
	double const encsecs = rtc.getElapsedSeconds();
	std::cerr << "[V] encoded " << n << " numbers to dyn growing vector in time " << encsecs 
		<< " rate " << (n / encsecs)/(1000*1000) << " m/s"
		<< std::endl;

	rtc.start();
	VectorGet<uint64_t> VG(VP.begin());
	::libmaus::gamma::GammaDecoder < VectorGet<uint64_t> > GD(VG);
	bool ok = true;
	for ( uint64_t i = 0; ok && i < n; ++i )
	{
		uint64_t const v = GD.decode();
		ok = ok && (v==V[i]);
		if ( ! ok )
		{
			std::cerr << "expected " << V[i] << " got " << v << std::endl;
		}
	}
	double const decsecs = rtc.getElapsedSeconds();
	std::cerr << "[V] decoded " << n << " numbers in time " << decsecs
		<< " rate " << (n / decsecs)/(1000*1000) << " m/s"
		<< std::endl;
	
	if ( ok )
	{
		std::cout << "Test of gamma coding with " << n << " random numbers ok." << std::endl;
	}
	else
	{
		std::cout << "Test of gamma coding with " << n << " random numbers failed." << std::endl;
	}
}

void testLow()
{
	unsigned int const n = 127;
	VectorPut<uint64_t> VP;
	::libmaus::gamma::GammaEncoder< VectorPut<uint64_t> > GE(VP);	
	for ( uint64_t i = 0; i < n; ++i )
		GE.encode(i);
	GE.flush();
	
	VectorGet<uint64_t> VG(VP.begin());
	::libmaus::gamma::GammaDecoder < VectorGet<uint64_t> > GD(VG);
	bool ok = true;
	for ( uint64_t i = 0; ok && i < n; ++i )
		ok = ok && ( GD.decode() == i );

	if ( ok )
	{
		std::cout << "Test of gamma coding with numbers up to 126 ok." << std::endl;
	}
	else
	{
		std::cout << "Test of gamma coding with numbers up to 126 failed." << std::endl;
	}
}

#include <libmaus/gamma/GammaGapEncoder.hpp>
#include <libmaus/huffman/IndexDecoderDataArray.hpp>
#include <libmaus/gamma/GammaGapDecoder.hpp>
#include <libmaus/util/TempFileRemovalContainer.hpp>

void testgammagap()
{
	unsigned int n = 512*1024+199481101;
	std::vector<uint64_t> V (n);
	std::vector<uint64_t> V2(n);
	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		V[i] = i & 0xFFull;
		V2[i] = rand() % 0xFFull;
	}

	std::string const fn("tmpfile");
	std::string const fn2("tmpfile2");
	std::string const fnm("tmpfile.merged");

	::libmaus::util::TempFileRemovalContainer::setup();
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn);
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn2);
	::libmaus::util::TempFileRemovalContainer::addTempFile(fnm);

	::libmaus::gamma::GammaGapEncoder GGE(fn);
	GGE.encode(V.begin(),V.end());
	::libmaus::gamma::GammaGapEncoder GGE2(fn2);
	GGE2.encode(V2.begin(),V2.end());
	
	::libmaus::huffman::IndexDecoderData IDD(fn);
	
	::libmaus::gamma::GammaGapDecoder GGD(std::vector<std::string>(1,fn));
	
	bool ok = true;
	for ( uint64_t i = 0; i < n; ++i )
	{
		uint64_t const v = GGD.decode();
		ok = ok && (v == V[i]);
	}
	std::cout << "decoding " << (ok ? "ok" : "fail") << std::endl;

	std::vector < std::vector<std::string> > merin;
	merin.push_back(std::vector<std::string>(1,fn));
	merin.push_back(std::vector<std::string>(1,fn2));
	::libmaus::gamma::GammaGapEncoder::merge(merin,fnm);
}

#include <libmaus/gamma/GammaRLEncoder.hpp>
#include <libmaus/gamma/GammaRLDecoder.hpp>

void testgammarl()
{
	srand(time(0));
	unsigned int n = 128*1024*1024;
	unsigned int n2 = 64*1024*1024;
	std::vector<uint64_t> V (n);
	std::vector<uint64_t> V2 (n2);
	std::vector<uint64_t> Vcat;
	unsigned int const albits = 3;
	uint64_t const almask = (1ull << albits)-1;
	
	for ( uint64_t i = 0; i < V.size(); ++i )
	{
		V[i]  = rand() & almask;
		Vcat.push_back(V[i]);
	}
	for ( uint64_t i = 0; i < V2.size(); ++i )
	{
		V2[i] = rand() & almask;
		Vcat.push_back(V2[i]);
	}

	std::string const fn("tmpfile");
	std::string const fn2("tmpfile2");
	std::string const fn3("tmpfile3");
	::libmaus::util::TempFileRemovalContainer::setup();
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn);
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn2);
	::libmaus::util::TempFileRemovalContainer::addTempFile(fn3);

	::libmaus::gamma::GammaRLEncoder GE(fn,albits,V.size(),256*1024);	
	for ( uint64_t i = 0; i < V.size(); ++i )
		GE.encode(V[i]);
	GE.flush();

	::libmaus::gamma::GammaRLEncoder GE2(fn2,albits,V2.size(),256*1024);
	for ( uint64_t i = 0; i < V2.size(); ++i )
		GE2.encode(V2[i]);
	GE2.flush();

	#if 0
	::libmaus::huffman::IndexDecoderData IDD(fn);
	for ( uint64_t i = 0; i < IDD.numentries+1; ++i )
		std::cerr << IDD.readEntry(i) << std::endl;	
	#endif

	::libmaus::gamma::GammaRLDecoder GD(std::vector<std::string>(1,fn));
	assert ( GD.getN() == n );

	for ( uint64_t i = 0; i < n; ++i )
		assert ( GD.decode() == static_cast<int64_t>(V[i]) );
	
	uint64_t const off = n / 2 + 1031;
	::libmaus::gamma::GammaRLDecoder GD2(std::vector<std::string>(1,fn),off);
	for ( uint64_t i = off; i < n; ++i )
		assert ( GD2.decode() == static_cast<int64_t>(V[i]) );
		
	std::vector<std::string> fnv;
	fnv.push_back(fn);
	fnv.push_back(fn2);
	::libmaus::gamma::GammaRLEncoder::concatenate(fnv,fn3);
	
	for ( uint64_t off = 0; off < Vcat.size(); off += 18521 )
	{
		::libmaus::gamma::GammaRLDecoder GD3(std::vector<std::string>(1,fn3),off);	
		assert ( GD3.getN() == Vcat.size() );
		
		for ( uint64_t i = 0; i < std::min(static_cast<uint64_t>(1024ull),Vcat.size()-off); ++i )
			assert ( GD3.decode() == static_cast<int64_t>(Vcat[off+i]) );
	}

	remove ( fn.c_str() );
	remove ( fn2.c_str() );
	remove ( fn3.c_str() );	
}

void testgammasparse()
{
	std::ostringstream o0;
	libmaus::gamma::SparseGammaGapEncoder SE0(o0);
	std::ostringstream o1;
	libmaus::gamma::SparseGammaGapEncoder SE1(o1);
	
	SE0.encode(4, 7);
	SE0.encode(6, 3);
	SE0.term();
	
	SE1.encode(0, 1);
	SE1.encode(2, 5);
	SE1.encode(6, 2);
	SE1.encode(8, 7);
	SE1.term();
	
	std::cerr << "o0.size()=" << o0.str().size() << std::endl;
	std::cerr << "o1.size()=" << o1.str().size() << std::endl;
	
	std::istringstream i0(o0.str());
	libmaus::gamma::SparseGammaGapDecoder SD0(i0);
	std::istringstream i1(o1.str());
	libmaus::gamma::SparseGammaGapDecoder SD1(i1);
	
	for ( uint64_t i = 0; i < 10; ++i )
		std::cerr << SD0.decode() << ";";
	std::cerr << std::endl;
	for ( uint64_t i = 0; i < 10; ++i )
		std::cerr << SD1.decode() << ";";
	std::cerr << std::endl;

	std::istringstream mi0(o0.str());
	std::istringstream mi1(o1.str());
	std::ostringstream mo;
	
	libmaus::gamma::SparseGammaGapMerge::merge(mi0,mi1,mo);
	
	std::istringstream mi(mo.str());
	libmaus::gamma::SparseGammaGapDecoder SDM(mi);

	for ( uint64_t i = 0; i < 10; ++i )
		std::cerr << SDM.decode() << ";";
	std::cerr << std::endl;
}

void testsparsegammamerge()
{
	libmaus::util::TempFileNameGenerator tmpgen("tmp",3);
	libmaus::gamma::SparseGammaGapFileSet SGGF(tmpgen);
	std::map<uint64_t,uint64_t> refM;
	
	for ( uint64_t i = 0; i < 25;  ++i )
	{
		std::string const fn = tmpgen.getFileName();
		libmaus::aio::CheckedOutputStream COS(fn);
		libmaus::gamma::SparseGammaGapEncoder SGE(COS);
		
		SGE.encode(2*i,i+1);   refM[2*i]   += (i+1);
		SGE.encode(2*i+2,i+1); refM[2*i+2] += (i+1);
		SGE.encode(2*i+4,i+1); refM[2*i+4] += (i+1);
		SGE.term();
		
		SGGF.addFile(fn);
	}
	
	std::string const ffn = tmpgen.getFileName();
	SGGF.merge(ffn);
	
	libmaus::aio::CheckedInputStream CIS(ffn);
	libmaus::gamma::SparseGammaGapDecoder SGGD(CIS);
	for ( uint64_t i = 0; i < 60; ++i )
	{
		uint64_t dv = SGGD.decode();
		
		std::cerr << dv;
		if ( refM.find(i) != refM.end() )
		{
			std::cerr << "(" << refM.find(i)->second << ")";
			assert ( refM.find(i)->second == dv );
		}
		else
		{
			std::cerr << "(0)";
			assert ( dv == 0 );
		}
		std::cerr << ";";
	}
	std::cerr << std::endl;
	
	remove(ffn.c_str());
}

void testsparsegammalevelmerge()
{
	libmaus::util::TempFileNameGenerator tmpgen("tmp",3);
	libmaus::gamma::SparseGammaGapFileLevelSet SGGF(tmpgen);
	std::map<uint64_t,uint64_t> refM;
	
	for ( uint64_t i = 0; i < 25;  ++i )
	{
		std::string const fn = tmpgen.getFileName();
		libmaus::aio::CheckedOutputStream COS(fn);
		libmaus::gamma::SparseGammaGapEncoder SGE(COS);
		
		SGE.encode(2*i,i+1);   refM[2*i]   += (i+1);
		SGE.encode(2*i+2,i+1); refM[2*i+2] += (i+1);
		SGE.encode(2*i+4,i+1); refM[2*i+4] += (i+1);
		SGE.term();
		
		SGGF.addFile(fn);
	}
	
	std::string const ffn = tmpgen.getFileName();
	SGGF.merge(ffn);
	
	libmaus::aio::CheckedInputStream CIS(ffn);
	libmaus::gamma::SparseGammaGapDecoder SGGD(CIS);
	for ( uint64_t i = 0; i < 60; ++i )
	{
		uint64_t dv = SGGD.decode();
		
		std::cerr << dv;
		if ( refM.find(i) != refM.end() )
		{
			std::cerr << "(" << refM.find(i)->second << ")";
			assert ( refM.find(i)->second == dv );
		}
		else
		{
			std::cerr << "(0)";
			assert ( dv == 0 );
		}
		std::cerr << ";";
	}
	std::cerr << std::endl;
	
	remove(ffn.c_str());
}


#include <libmaus/gamma/SparseGammaGapBlockEncoder.hpp>
#include <libmaus/gamma/SparseGammaGapConcatDecoder.hpp>

void testSparseGammaConcat()
{
	for ( uint64_t An = 1; An <= 512; ++An )
		for ( uint64_t z = 0; z < 4; ++z )
		{
			// set up array with random numbers
			uint64_t const Amod = (An+1)/2 + (rand() % An);
			libmaus::autoarray::AutoArray<uint64_t> A(An,false);
			for ( uint64_t i = 0; i < A.size(); ++i )
				A[i] = rand() % Amod;

			// file name prefix
			std::string const fnpref = "tmp_g";
			
			// encode array
			std::vector<std::string> concfn = libmaus::gamma::SparseGammaGapBlockEncoder::encodeArray(
				&A[0], &A[An], fnpref, 7 /* parts */, 5 /* block size */);
				
			// store data in map for reference
			std::map<uint64_t,uint64_t> M;
			for ( uint64_t i = 0; i < An; ++i )
				M[A[i]]++;
		
			std::vector<uint64_t> const splitkeys = libmaus::gamma::SparseGammaGapFileIndexMultiDecoder::getSplitKeys(concfn,concfn,Amod,8);
			
			std::cerr << "splitkeys: ";
			for ( uint64_t i = 0; i < splitkeys.size(); ++i )
				std::cerr << splitkeys[i] << ";";
			std::cerr << std::endl;
		
			// test reading back starting from beginning	
			libmaus::gamma::SparseGammaGapConcatDecoder SGGCD(concfn);
			for ( uint64_t i = 0; i < Amod; ++i )
			{
				uint64_t const v = SGGCD.decode();
				// std::cerr << i << "\t" << v << "\t" << M[i] << std::endl;
				assert ( v == M[i] );
			}
			
			// test reading back from given starting position
			for ( uint64_t j = 0; j < Amod; ++j )
			{
				// std::cerr << "*** j=" << j << " ***" << std::endl;
				libmaus::gamma::SparseGammaGapConcatDecoder SGGCD(concfn,j);
			
				for ( uint64_t i = j; i < Amod; ++i )
				{
					uint64_t const v = SGGCD.decode();
					if ( v != M[i] )
						std::cerr << j << "\t" << i << "\t" << v << "\t" << M[i] << std::endl;
					assert ( v == M[i] );
				}
			}
			
			// remove files
			for ( uint64_t i = 0; i < concfn.size(); ++i )
				remove(concfn[i].c_str());
			
			std::cerr << An << "\t" << z+1 << std::endl;
		}
}

void testSparseGammaIndexing()
{
	std::stringstream datastr;
	std::stringstream indexstr;
	libmaus::gamma::SparseGammaGapBlockEncoder SGGBE(datastr,indexstr,-1 /* prevkey */,2);
	SGGBE.encode(2,3);
	SGGBE.encode(7,2);
	SGGBE.encode(11,1);
	SGGBE.encode(12,5);
	SGGBE.encode(13,4);
	SGGBE.term();
	
	std::istringstream istr(datastr.str());
	libmaus::gamma::SparseGammaGapFileIndexDecoder SGGFID(istr);
	
	for ( libmaus::gamma::SparseGammaGapFileIndexDecoder::const_iterator ita = SGGFID.begin(); ita != SGGFID.end(); ++ita )
	{
		std::cerr << *ita << std::endl;
	}
	
	std::cerr << std::string(80,'-') << std::endl;
			
	// std::cerr << SGGFID.getBlockIndex(1) << std::endl;
	std::cerr << SGGFID.getBlockIndex(2) << std::endl;
	std::cerr << SGGFID.getBlockIndex(7) << std::endl;
	std::cerr << SGGFID.getBlockIndex(11) << std::endl;
	std::cerr << SGGFID.getBlockIndex(12) << std::endl;
	std::cerr << SGGFID.getBlockIndex(13) << std::endl;
	std::cerr << SGGFID.getBlockIndex(512) << std::endl;
}

int main()
{
	try
	{
		// srand(time(0));
		srand(5);
		
		testSparseGammaConcat();
		testSparseGammaIndexing();

		testsparsegammalevelmerge();
		
		return 0;
		
		testsparsegammamerge();
		testgammasparse();
		testgammarl();
		testLow();
		testRandom(256*1024*1024);
		testgammagap();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
