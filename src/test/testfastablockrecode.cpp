/*
    libmaus
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

#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/fastx/StreamFastAReader.hpp>

#include <libmaus/fastx/FastADefinedBasesTable.hpp>
#include <libmaus/util/CountPutObject.hpp>

struct FastAInfo
{
	std::string sid;
	uint64_t len;
	
	FastAInfo()
	{

	}
	FastAInfo(
		std::string const & rsid, 
		uint64_t const rlen
	) : sid(rsid), len(rlen) 
	{
	
	}
	FastAInfo(std::istream & in)
	{
		len = libmaus::util::NumberSerialisation::deserialiseNumber(in);
		uint64_t const sidlen = libmaus::util::UTF8::decodeUTF8(in);
		libmaus::autoarray::AutoArray<char> B(sidlen,false);
		in.read(B.begin(),sidlen);
		if ( in.gcount() != static_cast<int64_t>(sidlen) )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastAInfo(std::istream &): unexpected EOF/IO failure" << std::endl;
			lme.finish();
			throw lme;		
		}
		sid = std::string(B.begin(),B.end());
	}
	
	template<typename stream_type>
	void serialiseInternal(stream_type & out) const
	{
		libmaus::util::NumberSerialisation::serialiseNumber(out,len);
		libmaus::util::UTF8::encodeUTF8(sid.size(),out);
		out.write(sid.c_str(),sid.size());
	}

	template<typename stream_type>	
	uint64_t serialise(stream_type & out) const
	{
		libmaus::util::CountPutObject CPO;
		serialiseInternal(CPO);
		serialiseInternal(out);
		return CPO.c;
	}
};

#include <libmaus/util/TempFileRemovalContainer.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/bambam/BamSeqEncodeTable.hpp>

void countFastA(libmaus::util::ArgInfo const & arginfo, std::istream & fain, std::ostream & finalout)
{
	std::string const tmpprefix = arginfo.getUnparsedValue("T",arginfo.getDefaultTmpFileName());

	std::string const blockptrstmpfilename = tmpprefix + "_blockptrs";	
	libmaus::util::TempFileRemovalContainer::addTempFile(blockptrstmpfilename);
	std::string const datatmpfilename = tmpprefix + "_data";	
	libmaus::util::TempFileRemovalContainer::addTempFile(datatmpfilename);
	
	libmaus::aio::CheckedOutputStream blockptrCOS(blockptrstmpfilename);
	libmaus::aio::CheckedOutputStream dataCOS(datatmpfilename);

	libmaus::fastx::StreamFastAReaderWrapper S(fain);
	libmaus::fastx::StreamFastAReaderWrapper::pattern_type pat;
	uint64_t const bs = 64*1024;
	uint64_t ncnt = 0;
	uint64_t rcnt = 0;
	uint64_t rncnt = 0;
	
	libmaus::autoarray::AutoArray<uint8_t> toup(256,false);
	for ( uint64_t i = 0; i < 256; ++i )
		if ( isalpha(i) )
			toup[i] = toupper(i);
		else
			toup[i] = i;

	libmaus::fastx::FastADefinedBasesTable deftab;
	libmaus::autoarray::AutoArray<uint64_t> H(256,false);
	libmaus::bambam::BamSeqEncodeTable const bamencodetab;
	
	uint64_t totaldatasize = 0;
	uint64_t totalseqlen = 0;

	std::vector<FastAInfo> infovec;		
	std::vector<uint64_t> seqsizes;
	// FastAInfo
	// blockptrs
	// blocks
	while ( S.getNextPatternUnlocked(pat) )
	{
		std::string & s = pat.spattern;		
		char const * cpat = s.c_str();
		uint8_t const * upat = reinterpret_cast<uint8_t const *>(cpat);
		
		for ( uint64_t i = 0; i < s.size(); ++i )
			s[i] = toup[static_cast<uint8_t>(s[i])];

		std::vector<uint64_t> blocksizes;
		FastAInfo const fainfo(pat.sid,s.size());
		infovec.push_back(fainfo);
		
		libmaus::util::CountPutObject FAICPO;
		fainfo.serialise(FAICPO);
		uint64_t const numblocks = (s.size() + bs - 1)/bs;
		
		uint64_t low = 0;
		while ( low != s.size() )
		{
			uint64_t const high = std::min(low+bs,static_cast<uint64_t>(s.size()));
			std::fill(H.begin(),H.end(),0ull);
			
			for ( uint64_t i = low; i != high; ++i )
				H[toup[upat[i]]]++;
			
			uint64_t const regcnt = H['A'] + H['C'] + H['G'] + H['T'];
			uint64_t const regncnt = regcnt + H['N'];
			
			uint64_t blockdatasize = 0;
			
			// A,C,G,T only
			if ( regcnt == (high-low) )
			{
				// block type
				dataCOS.put(0);
				blockdatasize++;
			
				uint8_t const * inp = upat + low;
				for ( uint64_t i = 0; i < (high-low)/4; ++i )
				{
					uint8_t v = 0;
					         v |= libmaus::fastx::mapChar(*(inp++));
					v <<= 2; v |= libmaus::fastx::mapChar(*(inp++));
					v <<= 2; v |= libmaus::fastx::mapChar(*(inp++));
					v <<= 2; v |= libmaus::fastx::mapChar(*(inp++));
					dataCOS.put(v);
					blockdatasize++;
				}
				uint64_t rlow = low + ((high-low)/4)*4;
				uint64_t rest = high-rlow;
				assert ( rest < 4 );
				if ( rest )
				{
					uint8_t v = 0;
					for ( uint64_t i = 0; i < rest; ++i )
						v |= libmaus::fastx::mapChar(*(inp++)) << (6-2*i);
					dataCOS.put(v);
					blockdatasize++;
				}
				assert ( inp == upat + high );
				assert ( blockdatasize == 1+(high-low+3)/4 );
			
				rcnt += 1;
			}
			// A,C,G,T,N only
			else if ( regncnt == (high-low) )
			{
				// block type
				dataCOS.put(1);
				blockdatasize++;

				uint8_t const * inp = upat + low;
				for ( uint64_t i = 0; i < (high-low)/3; ++i )
				{
					uint8_t v = 0;
					        v += libmaus::fastx::mapChar(*(inp++));
					v *= 5; v += libmaus::fastx::mapChar(*(inp++));
					v *= 5; v += libmaus::fastx::mapChar(*(inp++));
					dataCOS.put(v);
					blockdatasize++;
				}
				uint64_t rlow = low + ((high-low)/3)*3;
				uint64_t rest = high-rlow;
				assert ( rest < 3 );

				if ( rest == 2 )
				{
					uint8_t v = 0;
					        v += libmaus::fastx::mapChar(*(inp++));
					v *= 5; v += libmaus::fastx::mapChar(*(inp++));
					v *= 5;						
					dataCOS.put(v);
					blockdatasize++;
				}
				else if ( rest == 1 )
				{
					assert ( rest == 1 );
					dataCOS.put( libmaus::fastx::mapChar(*(inp++)) * 5 * 5 );
					blockdatasize++;
				}
				assert ( inp == upat + high );
				
				// std::cerr << "block data size " << blockdatasize << " expected " << (high-low+2)/3 << std::endl;
				
				assert ( blockdatasize == 1+(high-low+2)/3 );

				rncnt += 1;
			}
			// more than A,C,G,T,N
			else
			{
				ncnt += 1;
				
				for ( uint64_t i = low; i < high; ++i )
					if ( (! deftab[upat[i]]) && (upat[i] != 'N') )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "Sequence " << pat.sid << " contains unknown symbol " << upat[i] << std::endl;
						lme.finish();
						throw lme;
					}


				// block type
				dataCOS.put(2);
				blockdatasize++;

				uint8_t const * inp = upat + low;
				for ( uint64_t i = 0; i < (high-low)/2; ++i )
				{
					uint8_t  v = 0;
					         v |= bamencodetab[*(inp++)];
					v <<= 4; v |= bamencodetab[*(inp++)];
					dataCOS.put(v);
					blockdatasize++;
				}
				uint64_t rlow = low + ((high-low)/2)*2;
				uint64_t rest = high-rlow;
				assert ( rest < 2 );

				if ( rest )
				{
					dataCOS.put(bamencodetab[*(inp++)] << 4);
					blockdatasize++;
				}
				assert ( inp == upat + high );
				
				assert ( blockdatasize == 1+(high-low+1)/2 );				
			}
			
			blocksizes.push_back(blockdatasize);

			low = high;
		}
		
		uint64_t sum = 0;
		for ( uint64_t i = 0; i < blocksizes.size(); ++i )
		{
			uint64_t t = blocksizes[i];
			blocksizes[i] = sum;
			sum += t;
		}
		blocksizes.push_back(sum);

		uint64_t const seqsize = FAICPO.c + (numblocks+1)*sizeof(uint64_t) + blocksizes.back();
		seqsizes.push_back(seqsize);
		
		libmaus::util::NumberSerialisation::serialiseNumberVector(blockptrCOS,blocksizes);
		
		totaldatasize += sum;
		
		std::cerr << "[V] " << pat.sid << " " << sum << " " << totaldatasize << std::endl;		
		
		totalseqlen += s.size();
	}
	
	blockptrCOS.flush();
	dataCOS.flush();
	blockptrCOS.close();
	dataCOS.close();
	
	libmaus::aio::CheckedInputStream blockptrCIS(blockptrstmpfilename);
	libmaus::aio::CheckedInputStream dataCIS(datatmpfilename);

	std::cerr << "[V] rcnt=" << rcnt << std::endl;
	std::cerr << "[V] rncnt=" << rncnt << std::endl;
	std::cerr << "[V] ncnt=" << ncnt << std::endl;
	
	std::vector<uint64_t> seqpos;
	
	uint64_t filepos = 0;
	finalout.put('F');
	finalout.put('A');
	finalout.put('B');
	finalout.put('\0');
	// length of magic
	filepos += 4;
	// block size
	filepos += libmaus::util::NumberSerialisation::serialiseNumber(finalout,bs);
	// number of sequences
	filepos += libmaus::util::NumberSerialisation::serialiseNumber(finalout,infovec.size());

	uint64_t const blockptrstart = filepos;
	uint64_t seqacc = 0;
	for ( uint64_t s = 0; s < seqsizes.size(); ++s )
	{
		uint64_t const seqpos_s = blockptrstart + seqsizes.size() * sizeof(uint64_t) + seqacc;
		filepos += libmaus::util::NumberSerialisation::serialiseNumber(finalout,seqpos_s);
		seqacc += seqsizes[s];
	}

	for ( uint64_t s = 0; s < infovec.size(); ++s )
	{
		FastAInfo const & info = infovec[s];
		seqpos.push_back(filepos);	
		
		// sequence meta data (name + length)
		filepos += info.serialise(finalout);
		
		std::vector<uint64_t> blockptrs = libmaus::util::NumberSerialisation::deserialiseNumberVector<uint64_t>(blockptrCIS);
		uint64_t datasize = blockptrs[blockptrs.size()-1] - blockptrs[0];
		uint64_t bpsize = 0;
		uint64_t bpoffset = (blockptrs.size()*sizeof(uint64_t));
		for ( uint64_t i = 0; i < blockptrs.size(); ++i )
			bpsize += libmaus::util::NumberSerialisation::serialiseNumber(finalout,blockptrs[i] + filepos + bpoffset);
			
		filepos += bpsize;
		
		libmaus::util::GetFileSize::copy(dataCIS,finalout,datasize);
		
		filepos += datasize;
	}
	
	finalout.flush();
	
	std::cerr << "[V] total bases " << totalseqlen << std::endl;
	std::cerr << "[V] " << static_cast<double>(filepos*8)/totalseqlen << " bits per base" << std::endl;
}

#include <libmaus/bambam/BamAlignmentDecoderBase.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);

		// countFastA(arginfo,std::cin,std::cout);	
		
		std::ostringstream ostr;
		countFastA(arginfo,std::cin,ostr);

		std::vector<double> rates;
		for ( uint64_t zz = 0; zz < 20; ++zz )
		{		
			std::istringstream istr(ostr.str());

			libmaus::timing::RealTimeClock rtc; rtc.start();			
			char magic[5];
			for ( unsigned int i = 0; i < 4; ++i )
			{
				int c = istr.get();
				assert ( c >= 0 );
				magic[i] = c;
			}
			magic[4] = 0;
			assert ( strcmp(&magic[0],"FAB\0") == 0 );
			
			// block size in symbols
			uint64_t const bs = libmaus::util::NumberSerialisation::deserialiseNumber(istr);
			assert ( bs == 64*1024 );
			
			// number of sequences
			uint64_t const numseq = libmaus::util::NumberSerialisation::deserialiseNumber(istr);
			uint64_t totalbases = 0;
			
			libmaus::autoarray::AutoArray<char> Bin((bs+1)/2,false);
			libmaus::autoarray::AutoArray<char> Bout(bs,false);
			
			// pointers to sequence start positions
			std::vector<uint64_t> seqpos;
			for ( uint64_t s = 0; s < numseq; ++s )
				seqpos.push_back(libmaus::util::NumberSerialisation::deserialiseNumber(istr));
			
			for ( uint64_t s = 0; s < numseq; ++s )
			{
				// check sequence start pointer
				assert ( istr.tellg() == static_cast<int64_t>(seqpos[s]) );
			
				FastAInfo info(istr);
				std::cerr << ">" << info.sid << std::endl;
				
				uint64_t const numsym = info.len;
				uint64_t const numblocks = (numsym + bs-1)/bs;
				
				totalbases += numsym;
				
				std::vector<uint64_t> blockptrs;
				for ( uint64_t i = 0; i < numblocks+1; ++i )
					blockptrs.push_back(libmaus::util::NumberSerialisation::deserialiseNumber(istr));
					
				for ( uint64_t i = 0; i < numblocks; ++i )
				{
					// check block pointer
					assert ( istr.tellg() == static_cast<int64_t>(blockptrs[i]) );
					
					uint64_t const low = i * bs;
					uint64_t const high = std::min(low+bs,numsym);
					
					int const blocktype = istr.get();
					assert ( blocktype >= 0 );
					assert ( blocktype < 3 );
					
					#if 0
					std::cerr << "[D] block type " << blocktype << std::endl;
					#endif
					
					switch ( blocktype )
					{
						case 0:
						{
							istr.read(Bin.begin(),(high-low+3)/4);
							assert ( istr.gcount() == static_cast<int64_t>((high-low+3)/4) );
							uint64_t k = 0;
							
							for ( uint64_t j = 0; j < (high-low)/4; ++j )
							{
								uint8_t const u = static_cast<uint8_t>(Bin[j]);
								
								Bout[k++] = libmaus::fastx::remapChar((u >> 6) & 3);
								Bout[k++] = libmaus::fastx::remapChar((u >> 4) & 3);
								Bout[k++] = libmaus::fastx::remapChar((u >> 2) & 3);
								Bout[k++] = libmaus::fastx::remapChar((u >> 0) & 3);
							}
							
							if ( (high-low) % 4 )
							{
								uint8_t const u = static_cast<uint8_t>(Bin[(high-low)/4]);

								for ( uint64_t j = 0; j < ((high-low)%4); ++j )								
									Bout[k++] = libmaus::fastx::remapChar((u >> (6-2*j)) & 3);								
							}
							
							assert ( k == high-low );
							
							#if 0
							std::cout.write(Bout.begin(),k);
							std::cout.put('\n');
							#endif
							
							break;
						}
						case 1:
						{
							istr.read(Bin.begin(),(high-low+2)/3);
							assert ( istr.gcount() == static_cast<int64_t>((high-low+2)/3) );
							uint64_t k = 0;
						
							for ( uint64_t j = 0; j < (high-low)/3; ++j )
							{
								uint8_t const u = Bin[j];
								
								Bout[k++] = libmaus::fastx::remapChar((u/(5*5))%5);
								Bout[k++] = libmaus::fastx::remapChar((u/(5*1))%5);
								Bout[k++] = libmaus::fastx::remapChar((u/(1*1))%5);
							}
							if ( (high-low) % 3 )
							{
								uint8_t const u = Bin[(high-low)/3];
								
								switch ( (high-low) % 3 )
								{
									case 1:
										Bout[k++] = libmaus::fastx::remapChar((u/(5*5))%5);
										break;
									case 2:
										Bout[k++] = libmaus::fastx::remapChar((u/(5*5))%5);
										Bout[k++] = libmaus::fastx::remapChar((u/(5*1))%5);
										break;
								}
							}
							
							assert ( k == high-low );
							
							#if 0
							std::cout.write(Bout.begin(),k);
							std::cout.put('\n');
							#endif

							break;
						}
						case 2:
						{
							istr.read(Bin.begin(),(high-low+1)/2);
							assert ( istr.gcount() == static_cast<int64_t>((high-low+1)/2) );
							uint64_t k = 0;

							for ( uint64_t j = 0; j < (high-low)/2; ++j )
							{
								uint8_t const u = Bin[j];

								Bout[k++] = libmaus::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(u >> 4 );
								Bout[k++] = libmaus::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(u & 0xF);
							}
							if ( (high-low)&1 )
							{
								uint8_t const u = Bin[(high-low)/2];
								Bout[k++] = libmaus::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(u >> 4);
							}
							
							assert ( k == high-low );

							#if 0
							std::cout.write(Bout.begin(),k);						
							std::cout.put('\n');
							#endif
							break;
						}
						default:
							break;
					}
				}

				assert ( istr.tellg() == static_cast<int64_t>(blockptrs[numblocks]) );
			}
		
			rates.push_back(totalbases/rtc.getElapsedSeconds());
			std::cerr << rates.back() << " bases/s" << std::endl;
		}
		
		double avg = std::accumulate(rates.begin(),rates.end(),0.0)/rates.size();
		std::cerr << "average rate " << avg << " bases/s" << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}

