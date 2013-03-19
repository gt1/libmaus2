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
#if ! defined(LIBMAUS_FASTX_COMPACTFASTQBLOCKGENERATOR_HPP)
#define LIBMAUS_FASTX_COMPACTFASTQBLOCKGENERATOR_HPP

#include <libmaus/util/CountPutObject.hpp>
#include <libmaus/util/CountPutIterator.hpp>
#include <libmaus/fastx/FastQReader.hpp>
#include <libmaus/fastx/FqWeightQuantiser.hpp>
#include <libmaus/fastx/CompactFastQContainerDictionaryCreator.hpp>
#include <libmaus/fastx/CompactFastEncoder.hpp>
#include <libmaus/bitio/FastWriteBitWriter.hpp>
#include <libmaus/fastx/CompactFastDecoderBase.hpp>

namespace libmaus
{
	namespace fastx
	{
		struct CompactFastQBlockGenerator
		{			
			typedef ::libmaus::fastx::FastQReader reader_type;
			typedef reader_type::unique_ptr_type reader_ptr_type;
			typedef reader_type::pattern_type pattern_type;
			
			static ::libmaus::fastx::FastInterval encodeCompactFastQBlock(
				std::vector<std::string> const & filenames,
				int const qualityOffset,
				uint64_t & fileoffset,
				uint64_t const blocksize,
				unsigned int qbits,
				std::ostream & out,
				::libmaus::fastx::CompactFastQContainerDictionaryCreator * clrc = 0,
				::libmaus::fastx::FastInterval const * inFI = 0
			)
			{
				reader_ptr_type preader;
				
				if ( inFI )
					preader = UNIQUE_PTR_MOVE(reader_ptr_type(new reader_type(filenames,*inFI,qualityOffset)));
				else
					preader = UNIQUE_PTR_MOVE(reader_ptr_type(new reader_type(filenames,qualityOffset,
						reader_type::getDefaultNumberOfBuffers(),
						reader_type::getDefaultBufferSize(),fileoffset)));

				pattern_type pattern;
				
				if ( clrc )
					clrc->reset();
				
				/* count number of reads and symbols in output block, compute quality score histogram */
				uint64_t numreads = 0;
				::libmaus::util::CountPutObject CPO;
				::libmaus::util::Histogram qhist;
				uint64_t maxl = 0, minl = std::numeric_limits<uint64_t>::max();
				uint64_t numsyms = 0;
				for ( ; numreads < blocksize && preader->getNextPatternUnlocked(pattern); ++numreads )
				{
					uint64_t const codepospre = CPO.c;
					
					uint64_t const l = pattern.getPatternLength();

					// encode pattern/query
					::libmaus::fastx::CompactFastEncoder::encodePattern(pattern,CPO,(1ul<<1));
					
					// length of quality info
					CPO.c += ((l*qbits + 7)/8);
					for ( uint64_t i = 0; i < l; ++i )
						qhist(pattern.quality[i]);
						
					maxl = std::max(maxl,l);
					minl = std::min(minl,l);
					numsyms += l;

					uint64_t const codepospost = CPO.c;
					uint64_t const codelen = codepospost-codepospre;
					
					if ( clrc )
						(*clrc)(::libmaus::fastx::CompactFastQContainerDictionaryCreator::codelenrun_first,codelen);
				}
				
				if ( numreads )
				{
					if ( clrc )
						clrc->setupDataStructures();

					std::map < uint64_t, uint64_t > const M = qhist.get();
					std::vector<double> VQ;

					// if number of quantisation levels is smaller than existing quality score levels
					// compute quantiser levels by k means algorithm
					if ( (1ull << qbits) < M.size() )
					{
						std::map < uint64_t, uint64_t > const Qfreq = ::libmaus::fastx::FqWeightQuantiser::histogramToWeights(M,1024*1024);
						::libmaus::fastx::FqWeightQuantiser::constructSampleVector(Qfreq,VQ);
					}
					// we have sufficient quantiser levels to represent all input quality levels
					// exactly
					else
					{
						for ( std::map < uint64_t, uint64_t >::const_iterator ita = M.begin(); ita != M.end(); ++ita )
							VQ.push_back(::libmaus::fastx::Phred::phred_error[ita->first]);
						std::reverse(VQ.begin(),VQ.end());
					}
					
					uint64_t const numsteps = std::min(
						static_cast<uint64_t>(1ull<<qbits),
						static_cast<uint64_t>(M.size())
					);
					::libmaus::fastx::FqWeightQuantiser quant(VQ,numsteps);
										
					std::string const squant = quant.serialise();
					
					uint64_t const blockcodelength = 2*sizeof(uint64_t) + 1 + squant.size() + CPO.c;
					
					::libmaus::util::CountPutIterator< std::ostream, uint8_t > CPI(&out);

					// length of header: 8 + 8 + 1 + size(quantiser)					
					::libmaus::util::NumberSerialisation::serialiseNumber(CPI,blockcodelength); // size of block in bytes
					::libmaus::util::NumberSerialisation::serialiseNumber(CPI,numreads); // number of reads in block
					CPI.put(qbits); // number of bits per quality value
					CPI.put(squant); // serialised quantiser for quality scores

					if ( inFI )
						preader = UNIQUE_PTR_MOVE(reader_ptr_type(new reader_type(filenames,*inFI,qualityOffset)));
					else
						preader = UNIQUE_PTR_MOVE(reader_ptr_type(new reader_type(filenames,qualityOffset,
							reader_type::getDefaultNumberOfBuffers(),
							reader_type::getDefaultBufferSize(),fileoffset)));

					uint64_t i;
					for ( i = 0 ; i < numreads && preader->getNextPatternUnlocked(pattern); ++i )
					{
						uint64_t const codepospre = CPI.c;
						
						// std::cerr << pattern;
					
						::libmaus::fastx::CompactFastEncoder::encodePattern(pattern,CPI,(1ul<<1));

						::std::ostream_iterator<uint8_t> it(out);
						::libmaus::bitio::FastWriteBitWriterStream8 FWBWS(it);
						
						uint64_t const l = pattern.getPatternLength();
						// std::cerr << "l=" << l << std::endl;
						for ( uint64_t j = 0; j < l; ++j )
						{
							uint64_t const qv = pattern.quality[j];
							// assert ( quant.quantForPhred[qv] == quant(qv) );
							// std::cerr << "qv=" << qv << " -> " << quant(qv) << std::endl;
							FWBWS.write( quant.quantForPhred[qv], qbits );
						}
						
						FWBWS.flush();
						
						CPI.c += (l*qbits+7)/8;
						
						uint64_t const codepospost = CPI.c;
						uint64_t const codelen = codepospost-codepospre;

						if ( clrc )
							(*clrc)(::libmaus::fastx::CompactFastQContainerDictionaryCreator::codelenrun_second,codelen);
					}
					assert ( i == numreads );
					
					fileoffset += preader->getFilePointer();

					if ( clrc )
						clrc->flush();

					if ( clrc )
						clrc->shiftLongPointers(2*sizeof(uint64_t)+1+squant.size());

					#if 0				
					if ( clrc )
						std::cerr << "Dict byte size " << clrc->byteSize() << std::endl;
					#endif

					return ::libmaus::fastx::FastInterval(0,numreads,0,CPI.c,numsyms,minl,maxl);
				}
				else
				{
					::libmaus::fastx::FqWeightQuantiser quant;
					std::string const squant = quant.serialise();

					uint64_t const blockcodelength = 2*sizeof(uint64_t) + 1 + squant.size() + 
						::libmaus::fastx::CompactFastDecoderBase::getTermLength();
					
					::libmaus::util::CountPutIterator< std::ostream, uint8_t > CPI(&out);

					::libmaus::util::NumberSerialisation::serialiseNumber(CPI,blockcodelength); // size of block in bytes
					::libmaus::util::NumberSerialisation::serialiseNumber(CPI,numreads); // number of reads in block
					CPI.put(qbits);
					CPI.put(squant); // quantiser for quality scores
					
					::libmaus::fastx::CompactFastEncoder::encodeEndMarker(CPI);

					return ::libmaus::fastx::FastInterval(0,0,0,0,0,0,0);
				}
			}
			
			static void encodeCompactFastQFile(
				std::vector<std::string> const & filenames,
				int qualityOffset,
				uint64_t const blocksize,
				unsigned int qbits,
				std::ostream & out)
			{
				qualityOffset = qualityOffset ? qualityOffset : reader_type::getOffset(filenames);
				if ( ! qualityOffset )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Failed to guess quality offset of input FastQ file." << std::endl;
					se.finish();
					throw se;
				}
				
				uint64_t fileoffset = 0;

				::libmaus::fastx::FastInterval FI;
				std::vector < ::libmaus::fastx::FastInterval > index;
				
				while ( (FI = encodeCompactFastQBlock(filenames,qualityOffset,fileoffset,blocksize,qbits,out)).high )
				{
					if ( index.size() )
					{
						FI.low += index.back().high;
						FI.high += index.back().high;
						FI.fileoffset += index.back().fileoffsethigh;
						FI.fileoffsethigh += index.back().fileoffsethigh;
					}
					
					index.push_back(FI);
					
					//std::cerr << fileoffset << "::" << FI << std::endl;
					std::cerr << FI << std::endl;
				}

				::libmaus::fastx::CompactFastEncoder::putIndex(index,out);
				out.flush();
			}
			
			static void encodeCompactFastQContainer(
				std::vector<std::string> const & filenames,
				int qualityOffset,
				unsigned int qbits,
				std::ostream & costr,
				::libmaus::fastx::FastInterval const * inFI = 0
			)
			{
				std::cerr << "Encoding for compact...";
				::libmaus::serialize::Serialize<uint64_t>::serialize(costr,0);
				::libmaus::fastx::CompactFastQContainerDictionaryCreator clrc;
				uint64_t allfo = 0;
				qualityOffset = qualityOffset ? qualityOffset : reader_type::getOffset(filenames);		
				::libmaus::fastx::FastInterval const allFI = CompactFastQBlockGenerator::encodeCompactFastQBlock(
					filenames,
					qualityOffset,
					allfo,
					std::numeric_limits<uint64_t>::max(),
					qbits,
					costr,
					&clrc,
					inFI);
				uint64_t const textsize = costr.tellp() - static_cast< ::std::streampos > ( sizeof(uint64_t) );
				std::cerr << "done." << std::endl;
				
				// write autoarray header
				costr.seekp(0);
				::libmaus::serialize::Serialize<uint64_t>::serialize(costr,textsize);
				costr.seekp(0,std::ios::end);
				
				// write dictionary
				clrc.serialise(costr);
				
				// write interval
				if ( inFI )
					::libmaus::fastx::FastInterval::serialise(costr,*inFI);
				else
					::libmaus::fastx::FastInterval::serialise(costr,allFI);
			}

			static std::string encodeCompactFastQContainer(
				std::vector<std::string> const & filenames,
				int qualityOffset,
				unsigned int qbits,
				::libmaus::fastx::FastInterval const * inFI = 0
			)
			{
				std::ostringstream costr;
				encodeCompactFastQContainer(filenames,qualityOffset,qbits,costr,inFI);
				return costr.str();
			}
		};
	}
}
#endif
