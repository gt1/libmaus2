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

#include <libmaus/fastx/FastAInfo.hpp>

#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/fastx/StreamFastAReader.hpp>

#include <libmaus/fastx/FastADefinedBasesTable.hpp>
#include <libmaus/util/CountPutObject.hpp>

#include <libmaus/hashing/Crc32.hpp>

#include <libmaus/util/TempFileRemovalContainer.hpp>
#include <libmaus/aio/CheckedOutputStream.hpp>
#include <libmaus/bambam/BamSeqEncodeTable.hpp>
#include <zlib.h>

#include <libmaus/fastx/FastAStreamSet.hpp>

struct FastABPConstants
{
	static uint8_t const base_block_4    =   0;
	static uint8_t const base_block_5    =   1;
	static uint8_t const base_block_16   =   2;
	static uint8_t const base_block_mask = 0x3;
	// marker: last block for a sequence
	static uint8_t const base_block_last      = (1u << 2);
};

void countFastA(libmaus::util::ArgInfo const & arginfo, std::istream & fain, std::ostream & finalout, uint64_t const bs = 64*1024)
{
	// to upper case table
	libmaus::autoarray::AutoArray<uint8_t> toup(256,false);
	for ( uint64_t i = 0; i < 256; ++i )
		if ( isalpha(i) )
			toup[i] = toupper(i);
		else
			toup[i] = i;

	// base check table
	libmaus::fastx::FastADefinedBasesTable deftab;
	// histogram
	libmaus::autoarray::AutoArray<uint64_t> H(256,false);
	// bam encoding table
	libmaus::bambam::BamSeqEncodeTable const bamencodetab;

	// tmp file name prefix
	std::string const tmpprefix = arginfo.getUnparsedValue("T",arginfo.getDefaultTmpFileName());
	
	// block pointers tmp file
	std::string const blockptrstmpfilename = tmpprefix + "_blockptrs";	
	libmaus::util::TempFileRemovalContainer::addTempFile(blockptrstmpfilename);

	// write file magic
	char cmagic[] = { 'F', 'A', 'S', 'T', 'A', 'B', 'P', '\0' };
	std::string const magic(&cmagic[0],&cmagic[sizeof(cmagic)]);
	uint64_t filepos = 0;
	for ( unsigned int i = 0; i < magic.size(); ++i, ++filepos )
		finalout.put(magic[i]);
	// write block size
	filepos += libmaus::util::NumberSerialisation::serialiseNumber(finalout,bs);

	std::vector<uint64_t> seqmetaposlist;
	libmaus::fastx::FastAStreamSet streamset(fain);
	std::pair<std::string,libmaus::fastx::FastAStream::shared_ptr_type> P;
	libmaus::autoarray::AutoArray<char> Bin(bs,false);
	libmaus::autoarray::AutoArray<char> Bout((bs+1)/2,false);
	uint64_t rcnt = 0;
	uint64_t rncnt = 0;
	uint64_t ncnt = 0;
	uint64_t totalseqlen = 0;
	while ( streamset.getNextStream(P) )
	{
		std::string const & sid = P.first;
		std::istream & in = *(P.second);
		std::vector<uint64_t> blockstarts;
		uint64_t prevblocksize = bs;
		uint64_t seqlen = 0;
		
		// std::cerr << "[V] processing " << sid << std::endl;
		
		if ( ! sid.size() )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "Empty sequence names are not supported." << std::endl;
			lme.finish();
			throw lme;
		}
		
		// length of name
		filepos += libmaus::util::UTF8::encodeUTF8(sid.size(),finalout);
		// name
		finalout.write(sid.c_str(),sid.size());
		filepos += sid.size();
		
		while ( filepos % sizeof(uint64_t) )
		{
			finalout.put(0);
			filepos++;
		}
		
		while ( in )
		{
			in.read(Bin.begin(),bs);
			uint64_t const n = in.gcount();
			
			if ( n )
			{
				// check block size consistency
				assert ( prevblocksize == bs );
				prevblocksize = n;
				
				bool const lastblock = (n != bs);
				uint8_t const lastblockflag = lastblock ? FastABPConstants::base_block_last : 0;
				
				//
				seqlen += n;
				
				// block start
				blockstarts.push_back(filepos);
			
				// clear histogram table	
				std::fill(H.begin(),H.end(),0ull);
				
				// convert to upper case and fill histogram
				for ( uint64_t i = 0; i < n; ++i )
				{
					uint8_t const v = toup[static_cast<uint8_t>(Bin[i])];
					H[v]++;
					Bin[i] = static_cast<char>(v);
				}

				uint64_t const regcnt = H['A'] + H['C'] + H['G'] + H['T'];
				uint64_t const regncnt = regcnt + H['N'];
				
				// output block pointer
				uint8_t * outp = reinterpret_cast<uint8_t *>(Bout.begin());
				
				if ( regcnt == n )
				{
					// block type
					finalout.put(FastABPConstants::base_block_4 | lastblockflag);
					filepos++;
					
					// store length if last block
					if ( lastblockflag )
						filepos += libmaus::util::UTF8::encodeUTF8(n,finalout);
				
					char const * inp = Bin.begin();
					
					while ( inp != Bin.begin() + (n/4)*4 )
					{
						uint8_t v = 0;
							 v |= libmaus::fastx::mapChar(*(inp++));
						v <<= 2; v |= libmaus::fastx::mapChar(*(inp++));
						v <<= 2; v |= libmaus::fastx::mapChar(*(inp++));
						v <<= 2; v |= libmaus::fastx::mapChar(*(inp++));
						*(outp++) = v;
					}
					uint64_t const rest = n % 4;
					
					if ( rest )
					{
						uint8_t v = 0;
						for ( uint64_t i = 0; i < rest; ++i )
							v |= libmaus::fastx::mapChar(*(inp++)) << (6-2*i);
						*(outp++) = v;
					}
					
					assert ( outp == reinterpret_cast<uint8_t *>(Bout.begin() + (n+3)/4) );

					++rcnt;
				}
				else if ( regncnt == n )
				{
					// block type
					finalout.put(FastABPConstants::base_block_5 | lastblockflag);
					filepos++;

					if ( lastblockflag )
						filepos += libmaus::util::UTF8::encodeUTF8(n,finalout);

					char const * inp = Bin.begin();
					while ( inp != Bin.begin() + (n/3)*3 )
					{
						uint8_t v = 0;
							v += libmaus::fastx::mapChar(*(inp++));
						v *= 5; v += libmaus::fastx::mapChar(*(inp++));
						v *= 5; v += libmaus::fastx::mapChar(*(inp++));
						*(outp++) = v;
					}
					uint64_t const rest = n%3;
					if ( rest == 2 )
					{
						uint8_t v = 0;
							v += libmaus::fastx::mapChar(*(inp++));
						v *= 5; v += libmaus::fastx::mapChar(*(inp++));
						v *= 5;						
						*(outp++) = v;
					}
					else if ( rest == 1 )
					{
						*(outp++) = libmaus::fastx::mapChar(*(inp++)) * 5 * 5;
					}

					assert ( outp == reinterpret_cast<uint8_t *>(Bout.begin() + (n+2)/3) );

					++rncnt;
				}
				else
				{
					// block type
					finalout.put(FastABPConstants::base_block_16 | lastblockflag);
					filepos++;

					if ( lastblockflag )
						filepos += libmaus::util::UTF8::encodeUTF8(n,finalout);

					// check whether all symbols are valid
					for ( uint64_t i = 0; i < n; ++i )
						if ( (! deftab[static_cast<uint8_t>(Bin[i])]) && (Bin[i] != 'N') )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "Sequence " << sid << " contains unknown symbol " << Bin[i] << std::endl;
							lme.finish();
							throw lme;
						}


					char const * inp = Bin.begin();
					while ( inp != Bin.begin() + (n/2)*2 )
					{
						uint8_t  v = 0;
						         v |= bamencodetab[*(inp++)];
						v <<= 4; v |= bamencodetab[*(inp++)];
						*(outp++) = v;
					}
					uint64_t const rest = n % 2;

					if ( rest )
						*(outp++) = bamencodetab[*(inp++)] << 4;	

					assert ( outp == reinterpret_cast<uint8_t *>(Bout.begin() + (n+1)/2) );
						
					++ncnt;
				}
				
				uint64_t const outbytes = outp-reinterpret_cast<uint8_t *>(Bout.begin());
				finalout.write(Bout.begin(),outbytes);
				filepos += outbytes;
				
				while ( filepos%8 != 0 )
				{
					finalout.put(0);
					filepos++;
				}

				// input crc
				uint32_t const crcin = libmaus::hashing::Crc32::crc32_8bytes(Bin.begin(),n,0 /*prev*/);
				for ( unsigned int i = 0; i < sizeof(uint32_t); ++i )
				{
					finalout.put((crcin >> (24-i*8)) & 0xFF);
					filepos += 1;
				}
				// output crc
				uint32_t const crcout = libmaus::hashing::Crc32::crc32_8bytes(Bout.begin(),outbytes,0 /*prev*/);
				for ( unsigned int i = 0; i < sizeof(uint32_t); ++i )
				{
					finalout.put((crcout >> (24-i*8)) & 0xFF);
					filepos += 1;
				}
			}
		}
		
		std::cerr << "[V] processed " << sid << " length " << seqlen << std::endl;

		uint64_t const seqmetapos = filepos;
		seqmetaposlist.push_back(seqmetapos);

		libmaus::fastx::FastAInfo const fainfo(sid,seqlen);
		filepos += fainfo.serialise(finalout);
		
		// write block pointers
		for ( uint64_t i = 0; i < blockstarts.size(); ++i )
			filepos += libmaus::util::NumberSerialisation::serialiseNumber(finalout,blockstarts[i]);
			
		totalseqlen += seqlen;
	}
	
	for ( unsigned int i = 0; i < sizeof(uint64_t); ++i )
	{
		finalout.put(0);
		filepos += 1;
	}
	
	uint64_t const metapos = filepos;
	// number of sequences
	filepos += libmaus::util::NumberSerialisation::serialiseNumber(finalout,seqmetaposlist.size());
	for ( uint64_t i = 0; i < seqmetaposlist.size(); ++i )
		// sequence meta data pointers
		filepos += libmaus::util::NumberSerialisation::serialiseNumber(finalout,seqmetaposlist[i]);

	// meta data pointer
	filepos += libmaus::util::NumberSerialisation::serialiseNumber(finalout,metapos);	
	
	std::cerr << "[V] number of sequences        " << seqmetaposlist.size() << std::endl;
	std::cerr << "[V] number of A,C,G,T   blocks " << rcnt << std::endl;
	std::cerr << "[V] number of A,C,G,T,N blocks " << rncnt << std::endl;
	std::cerr << "[V] number of ambiguity blocks " << ncnt << std::endl;
	std::cerr << "[V] file size                  " << filepos << std::endl;
	std::cerr << "[V] total number of bases      " << totalseqlen << std::endl;
	std::cerr << "[V] bits per base              " << static_cast<double>(filepos*8)/totalseqlen << std::endl;
}

#include <libmaus/bambam/BamAlignmentDecoderBase.hpp>

struct FastaBPSequenceDecoder
{
	typedef FastaBPSequenceDecoder this_type;
	typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

	std::istream & in;
	uint64_t const bs;
	libmaus::autoarray::AutoArray<char> Bin;
	bool eof;
	
	FastaBPSequenceDecoder(std::istream & rin, uint64_t const rbs)
	: in(rin), bs(rbs), Bin((bs+1)/2,false), eof(false)
	{
	
	}

	ssize_t read(char * p, uint64_t const m)
	{
		if ( eof )
			return 0;

		uint64_t bytesread = 0;
	
		int const flags = in.get();
		if ( flags < 0 )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastaBPSequenceDecoder::read(): EOF/error while reading block flags" << std::endl;
			lme.finish();
			throw lme;			
		}
		bytesread += 1;
				
		// check whether this is the last block
		eof = ( flags & FastABPConstants::base_block_last );
		// determine number of bytes to be produced from this block
		uint64_t const toread = eof ? libmaus::util::UTF8::decodeUTF8(in,bytesread) : bs;
		
		if ( m < toread )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastaBPSequenceDecoder::read(): buffer is too small for block data" << std::endl;
			lme.finish();
			throw lme;					
		}

		switch ( flags & FastABPConstants::base_block_mask )
		{
			case FastABPConstants::base_block_4:
			{
				in.read(Bin.begin(),(toread+3)/4);
				if ( in.gcount() != static_cast<int64_t>((toread+3)/4) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "FastaBPSequenceDecoder::read(): input failure" << std::endl;
					lme.finish();
					throw lme;					
				}
				bytesread += in.gcount();
				uint64_t k = 0;
				
				for ( uint64_t j = 0; j < toread/4; ++j )
				{
					uint8_t const u = static_cast<uint8_t>(Bin[j]);
					
					p[k++] = libmaus::fastx::remapChar((u >> 6) & 3);
					p[k++] = libmaus::fastx::remapChar((u >> 4) & 3);
					p[k++] = libmaus::fastx::remapChar((u >> 2) & 3);
					p[k++] = libmaus::fastx::remapChar((u >> 0) & 3);
				}
				
				if ( (toread) % 4 )
				{
					uint8_t const u = static_cast<uint8_t>(Bin[toread/4]);

					for ( uint64_t j = 0; j < ((toread)%4); ++j )								
						p[k++] = libmaus::fastx::remapChar((u >> (6-2*j)) & 3);								
				}								
				break;
			}
			case FastABPConstants::base_block_5:
			{
				in.read(Bin.begin(),(toread+2)/3);
				if ( in.gcount() != static_cast<int64_t>((toread+2)/3) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "FastaBPSequenceDecoder::read(): input failure" << std::endl;
					lme.finish();
					throw lme;				
				}
				bytesread += in.gcount();
				uint64_t k = 0;
			
				for ( uint64_t j = 0; j < (toread)/3; ++j )
				{
					uint8_t const u = Bin[j];
					
					p[k++] = libmaus::fastx::remapChar((u/(5*5))%5);
					p[k++] = libmaus::fastx::remapChar((u/(5*1))%5);
					p[k++] = libmaus::fastx::remapChar((u/(1*1))%5);
				}
				if ( toread % 3 )
				{
					uint8_t const u = Bin[toread/3];
					
					switch ( toread % 3 )
					{
						case 1:
							p[k++] = libmaus::fastx::remapChar((u/(5*5))%5);
							break;
						case 2:
							p[k++] = libmaus::fastx::remapChar((u/(5*5))%5);
							p[k++] = libmaus::fastx::remapChar((u/(5*1))%5);
							break;
					}
				}
				
				assert ( k == toread );
				break;
			}
			case FastABPConstants::base_block_16:
			{
				in.read(Bin.begin(),(toread+1)/2);
				if ( in.gcount() != static_cast<int64_t>((toread+1)/2) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "FastaBPSequenceDecoder::read(): input failure" << std::endl;
					lme.finish();
					throw lme;				
				}
				bytesread += in.gcount();
				uint64_t k = 0;

				for ( uint64_t j = 0; j < toread/2; ++j )
				{
					uint8_t const u = Bin[j];

					p[k++] = libmaus::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(u >> 4 );
					p[k++] = libmaus::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(u & 0xF);
				}
				if ( toread&1 )
				{
					uint8_t const u = Bin[toread/2];
					p[k++] = libmaus::bambam::BamAlignmentDecoderBase::decodeSymbolUnchecked(u >> 4);
				}
				
				assert ( k == toread );
				break;
			}
			default:
			{
				libmaus::exception::LibMausException lme;
				lme.getStream() << "FastaBPSequenceDecoder::read(): EOF/error while reading block type" << std::endl;
				lme.finish();
				throw lme;
			}
		}
		
		uint64_t const inputcount = in.gcount();
		
		// align file
		if ( bytesread % 8 )
		{
			uint64_t const toskip = 8-(bytesread % 8);
			in.ignore(toskip);
			if ( in.gcount() != static_cast<int64_t>(toskip) )
			{
				libmaus::exception::LibMausException lme;
				lme.getStream() << "FastaBPSequenceDecoder::read(): EOF/error while aligning file to 8 byte boundary" << std::endl;
				lme.finish();
				throw lme;
			}
		}
		
		// read crc
		uint8_t crcbytes[8];
		in.read(reinterpret_cast<char *>(&crcbytes[0]),sizeof(crcbytes));
		if ( in.gcount() != static_cast<int64_t>(sizeof(crcbytes)) )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastaBPSequenceDecoder::read(): EOF/error while reading block crc" << std::endl;
			lme.finish();
			throw lme;
		}
		
		#if 0
		uint32_t const crcin = 
			(static_cast<uint32_t>(crcbytes[0]) << 24) |
			(static_cast<uint32_t>(crcbytes[1]) << 16) |
			(static_cast<uint32_t>(crcbytes[2]) <<  8) |
			(static_cast<uint32_t>(crcbytes[3]) <<  0);
		uint32_t const crcout = 
			(static_cast<uint32_t>(crcbytes[4]) << 24) |
			(static_cast<uint32_t>(crcbytes[5]) << 16) |
			(static_cast<uint32_t>(crcbytes[6]) <<  8) |
			(static_cast<uint32_t>(crcbytes[7]) <<  0);

		uint32_t const crcincomp = libmaus::hashing::Crc32::crc32_8bytes(p,toread,0 /*prev*/);
		uint32_t const crcoutcomp = libmaus::hashing::Crc32::crc32_8bytes(Bin.begin(),inputcount,0 /*prev*/);
		
		if ( crcin != crcincomp )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastaBPSequenceDecoder::read(): crc error on uncompressed data" << std::endl;
			lme.finish();
			throw lme;		
		}
		if ( crcout != crcoutcomp )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastaBPSequenceDecoder::read(): crc error on compressed data" << std::endl;
			lme.finish();
			throw lme;				
		}
		#endif

		return toread;
	}
};

struct FastaBPDecoder
{
	libmaus::aio::CheckedInputStream::unique_ptr_type pCIS;
	std::istream & in;
	
	// block size
	uint64_t const bs;
	// meta position
	uint64_t const metapos;
	// number of sequences
	uint64_t const numseq;
	
	uint64_t readBlockSize() const
	{
		char magic[8];
		in.read(&magic[0],8);
		if ( in.gcount() != 8 )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastaBPDecoder::readBlockSize(): EOF/error while reading magic" << std::endl;
			lme.finish();
			throw lme;	
		}
		if ( std::string(&magic[0],&magic[8]) != std::string("FASTABP\0",8) )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastaBPDecoder::readBlockSize(): magic is wrong, this is not a fab file" << std::endl;
			lme.finish();
			throw lme;		
		}
		
		return libmaus::util::NumberSerialisation::deserialiseNumber(in);
	}
	
	uint64_t readMetaPos() const
	{
		in.clear();
		in.seekg(-static_cast<int>(sizeof(uint64_t)),std::ios::end);
		return libmaus::util::NumberSerialisation::deserialiseNumber(in);
	}

	uint64_t readNumSeq() const
	{
		in.clear();
		in.seekg(metapos,std::ios::beg);
		return libmaus::util::NumberSerialisation::deserialiseNumber(in);
	}

	uint64_t readSequencePointer(uint64_t const seq) const
	{
		if ( seq >= numseq )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastaBPDecoder::readSequencePointer: sequence id " << seq << " is out of range." << std::endl;
			lme.finish();
			throw lme;		
		}

		in.clear();
		in.seekg(metapos + (1+seq)*sizeof(uint64_t),std::ios::beg);
		return libmaus::util::NumberSerialisation::deserialiseNumber(in);
	}
	
	libmaus::fastx::FastAInfo readSequenceInfo(uint64_t const seq) const
	{
		uint64_t const seqpos = readSequencePointer(seq);
		in.clear();
		in.seekg(seqpos);
		libmaus::fastx::FastAInfo info(in);
		return info;
	}
	
	uint64_t getNumBlocks(uint64_t const seq) const
	{
		libmaus::fastx::FastAInfo info = readSequenceInfo(seq);
		return (info.len+bs-1)/bs;
	}
	
	std::string getSequenceName(uint64_t const seq) const
	{
		return readSequenceInfo(seq).sid;
	}

	uint64_t getSequenceLength(uint64_t const seq) const
	{
		return readSequenceInfo(seq).len;
	}
	
	uint64_t getTotalSequenceLength() const
	{
		uint64_t s = 0;
		for ( uint64_t i = 0; i < numseq; ++i )
			s += getSequenceLength(i);
		return s;
	}
	
	uint64_t getBlockPointer(uint64_t const seq, uint64_t const blockid) const
	{
		uint64_t const numblocks = getNumBlocks(seq);

		if ( blockid >= numblocks )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastaBPDecoder::readSequencePointer: block id " << blockid << " is out of range for sequence " << seq << std::endl;
			lme.finish();
			throw lme;		
		}

		in.clear();
		in.seekg(blockid * sizeof(uint64_t), std::ios::cur);
		return libmaus::util::NumberSerialisation::deserialiseNumber(in);
	}
	
	uint64_t getRestSymbols(uint64_t const seq, uint64_t const blockid) const
	{
		uint64_t const numblocks = getNumBlocks(seq);
		
		if ( blockid >= numblocks )
		{
			libmaus::exception::LibMausException lme;
			lme.getStream() << "FastaBPDecoder::readSequencePointer: block id " << blockid << " is out of range for sequence " << seq << std::endl;
			lme.finish();
			throw lme;		
		}
		
		return getSequenceLength(seq) - blockid * bs;
	}
		
	FastaBPDecoder(std::string const & s)
	: pCIS(new libmaus::aio::CheckedInputStream(s)), in(*pCIS), bs(readBlockSize()), metapos(readMetaPos()),
	  numseq(readNumSeq())
	{
	}
	
	FastaBPDecoder(std::istream & rin)
	: pCIS(), in(rin), bs(readBlockSize()), metapos(readMetaPos()),
	  numseq(readNumSeq())
	{
	}	
	
	FastaBPSequenceDecoder::unique_ptr_type getSequenceDecoder(uint64_t const seqid, uint64_t const blockid) const
	{
		uint64_t const bp = getBlockPointer(seqid,blockid);
		in.clear();
		in.seekg(bp);
		FastaBPSequenceDecoder::unique_ptr_type tptr(new FastaBPSequenceDecoder(in,bs));
		return UNIQUE_PTR_MOVE(tptr);
	}
	
	void checkBlockPointers(uint64_t const seq) const
	{
		libmaus::autoarray::AutoArray<char> Bout(bs,false);
		FastaBPSequenceDecoder::unique_ptr_type ptr(getSequenceDecoder(seq,0));

		uint64_t i = 0;

		uint64_t const p = in.tellg();
		uint64_t const bp = getBlockPointer(seq,i++);
		assert ( bp == p );
		in.clear();
		in.seekg(p,std::ios::beg);
			
		uint64_t n = 0;
		while ( (n = ptr->read(Bout.begin(),Bout.size())) )
		{
			if ( ! ptr->eof )
			{
				uint64_t const p = in.tellg();
				uint64_t const bp = getBlockPointer(seq,i++);
				assert ( bp == p );
				in.clear();
				in.seekg(p,std::ios::beg);	
			}
		}
	}

	void checkBlockPointers() const
	{
		for ( uint64_t i = 0; i < numseq; ++i )
			checkBlockPointers(i);	
	}

	void decodeSequenceNull(uint64_t const seq) const
	{
		getSequenceName(seq);

		libmaus::autoarray::AutoArray<char> Bout(bs,false);
		FastaBPSequenceDecoder::unique_ptr_type ptr(getSequenceDecoder(seq,0));
			
		uint64_t n = 0;
		while ( (n = ptr->read(Bout.begin(),Bout.size())) )
		{
		}
	}

	void decodeSequencesNull() const
	{
		for ( uint64_t i = 0; i < numseq; ++i )
			decodeSequenceNull(i);
	}

	void printSequence(std::ostream & out, uint64_t const seq) const
	{
		out << ">" << getSequenceName(seq) << "\n";

		libmaus::autoarray::AutoArray<char> Bout(bs,false);
		FastaBPSequenceDecoder::unique_ptr_type ptr(getSequenceDecoder(seq,0));
			
		uint64_t n = 0;
		while ( (n = ptr->read(Bout.begin(),Bout.size())) )
			std::cout.write(Bout.begin(),n);
		out.put('\n');	
	}
	
	void printSequences(std::ostream & out) const
	{
		for ( uint64_t i = 0; i < numseq; ++i )
			printSequence(out,i);	
	}
};

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);

		std::ostringstream ostr;
		countFastA(arginfo,std::cin,ostr);	

		std::istringstream istrpre(ostr.str());
		FastaBPDecoder fabd(istrpre);
		
		fabd.checkBlockPointers();
		
		fabd.printSequences(std::cout);
		
		uint64_t const totalseqlength = fabd.getTotalSequenceLength();
		
		uint64_t const runs = 10;
		std::vector<double> dectimes;
		for ( uint64_t i = 0; i < runs; ++i )
		{
			libmaus::timing::RealTimeClock rtc;
			rtc.start();
			fabd.decodeSequencesNull();
			dectimes.push_back(totalseqlength / rtc.getElapsedSeconds());
		}
		double const avg = std::accumulate(dectimes.begin(),dectimes.end(),0.0) / dectimes.size();
		double var = 0;
		for ( uint64_t i = 0; i < dectimes.size(); ++i )
			var += (dectimes[i]-avg)*(dectimes[i]-avg);
		var /= dectimes.size();
		double const stddev = ::std::sqrt(var);
		
		std::cerr << "[V] decoding speed " << avg << " += " << stddev << " bases/s" << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}

