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
#if ! defined(LIBMAUS_FASTX_FASTABPDECODER_HPP)
#define LIBMAUS_FASTX_FASTABPDECODER_HPP

#include <libmaus/fastx/FastaBPSequenceDecoder.hpp>
#include <libmaus/fastx/FastAInfo.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct FastaBPDecoder
		{	
			private:
			// block size
			uint64_t bs;
			// meta position
			uint64_t metapos;
			// number of sequences
			uint64_t numseq;
			
			//! read magic and block size
			uint64_t readBlockSize(std::istream & in) const
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
			
			// read position of meta data
			uint64_t readMetaPos(std::istream & in) const
			{
				in.clear();
				in.seekg(-static_cast<int>(sizeof(uint64_t)),std::ios::end);
				return libmaus::util::NumberSerialisation::deserialiseNumber(in);
			}

			// read number of sequences
			uint64_t readNumSeq(std::istream & in) const
			{
				in.clear();
				in.seekg(metapos,std::ios::beg);
				return libmaus::util::NumberSerialisation::deserialiseNumber(in);
			}

			// read pointer to sequence
			uint64_t readSequencePointer(
				std::istream & in, uint64_t const seq
			) const
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
			
			libmaus::fastx::FastAInfo readSequenceInfo(std::istream & in, uint64_t const seq) const
			{
				uint64_t const seqpos = readSequencePointer(in,seq);
				in.clear();
				in.seekg(seqpos);
				libmaus::fastx::FastAInfo info(in);
				return info;
			}
			
			uint64_t getNumBlocks(std::istream & in, uint64_t const seq) const
			{
				libmaus::fastx::FastAInfo info = readSequenceInfo(in,seq);
				return (info.len+bs-1)/bs;
			}
			
			std::string getSequenceName(std::istream & in, uint64_t const seq) const
			{
				return readSequenceInfo(in,seq).sid;
			}

			uint64_t getSequenceLength(std::istream & in, uint64_t const seq) const
			{
				return readSequenceInfo(in,seq).len;
			}
			
			
			uint64_t getBlockPointer(std::istream & in, uint64_t const seq, uint64_t const blockid) const
			{
				uint64_t const numblocks = getNumBlocks(in,seq);

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
			
			uint64_t getRestSymbols(std::istream & in, uint64_t const seq, uint64_t const blockid) const
			{
				uint64_t const numblocks = getNumBlocks(in,seq);
				
				if ( blockid >= numblocks )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "FastaBPDecoder::readSequencePointer: block id " << blockid << " is out of range for sequence " << seq << std::endl;
					lme.finish();
					throw lme;		
				}
				
				return getSequenceLength(in,seq) - blockid * bs;
			}

			void checkBlockPointers(std::istream & in, uint64_t const seq) const
			{
				libmaus::autoarray::AutoArray<char> Bout(bs,false);
				libmaus::fastx::FastaBPSequenceDecoder::unique_ptr_type ptr(getSequenceDecoder(in,seq,0));

				uint64_t i = 0;

				uint64_t const p = in.tellg();
				uint64_t const bp = getBlockPointer(in,seq,i++);
				assert ( bp == p );
				in.clear();
				in.seekg(p,std::ios::beg);
					
				uint64_t n = 0;
				while ( (n = ptr->read(Bout.begin(),Bout.size())) )
				{
					if ( ! ptr->eof )
					{
						uint64_t const p = in.tellg();
						uint64_t const bp = getBlockPointer(in,seq,i++);
						assert ( bp == p );
						in.clear();
						in.seekg(p,std::ios::beg);	
					}
				}
			}
				
			public:
			FastaBPDecoder(std::istream & in)
			{
				bs = readBlockSize(in);
				metapos = readMetaPos(in);
				numseq = readNumSeq(in);
			}	

			uint64_t getTotalSequenceLength(std::istream & in) const
			{
				uint64_t s = 0;
				for ( uint64_t i = 0; i < numseq; ++i )
					s += getSequenceLength(in,i);
				return s;
			}

			libmaus::fastx::FastaBPSequenceDecoder::unique_ptr_type getSequenceDecoder(std::istream & in, uint64_t const seqid, uint64_t const blockid) const
			{
				uint64_t const bp = getBlockPointer(in,seqid,blockid);
				in.clear();
				in.seekg(bp);
				libmaus::fastx::FastaBPSequenceDecoder::unique_ptr_type tptr(new libmaus::fastx::FastaBPSequenceDecoder(in,bs));
				return UNIQUE_PTR_MOVE(tptr);
			}
			

			void checkBlockPointers(std::istream & in) const
			{
				for ( uint64_t i = 0; i < numseq; ++i )
					checkBlockPointers(in,i);	
			}

			void decodeSequenceNull(std::istream & in, uint64_t const seq) const
			{
				getSequenceName(in,seq);

				libmaus::autoarray::AutoArray<char> Bout(bs,false);
				libmaus::fastx::FastaBPSequenceDecoder::unique_ptr_type ptr(getSequenceDecoder(in,seq,0));
					
				uint64_t n = 0;
				while ( (n = ptr->read(Bout.begin(),Bout.size())) )
				{
				}
			}

			void decodeSequencesNull(std::istream & in) const
			{
				for ( uint64_t i = 0; i < numseq; ++i )
					decodeSequenceNull(in,i);
			}

			void printSequence(std::istream & in, std::ostream & out, uint64_t const seq) const
			{
				out << ">" << getSequenceName(in,seq) << "\n";

				libmaus::autoarray::AutoArray<char> Bout(bs,false);
				libmaus::fastx::FastaBPSequenceDecoder::unique_ptr_type ptr(getSequenceDecoder(in,seq,0));
					
				uint64_t n = 0;
				while ( (n = ptr->read(Bout.begin(),Bout.size())) )
					std::cout.write(Bout.begin(),n);
				out.put('\n');	
			}
			
			void printSequences(std::istream & in, std::ostream & out) const
			{
				for ( uint64_t i = 0; i < numseq; ++i )
					printSequence(in,out,i);	
			}
			
			libmaus::autoarray::AutoArray<char> decodeSequence(std::istream & in, uint64_t const seq) const
			{
				uint64_t const seqlen = getSequenceLength(in,seq);
				libmaus::autoarray::AutoArray<char> A(seqlen,false);
				char * p = A.begin();
				uint64_t n = A.size();
				uint64_t r = 0;

				libmaus::fastx::FastaBPSequenceDecoder::unique_ptr_type ptr(getSequenceDecoder(in,seq,0));
				while ( (r = ptr->read(p,n) ) )
				{
					assert ( r <= n );
					n -= r;
					p += r;
				}
				
				return A;
			}
			
			uint64_t getNumSeq() const
			{
				return numseq;
			}
		};
	}
}
#endif
