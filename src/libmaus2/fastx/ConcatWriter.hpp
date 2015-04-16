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

#if ! defined(CONCATWRITER_HPP)
#define CONCATWRITER_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/bitio/FastWriteBitWriter.hpp>
#include <libmaus/bitio/putBit.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/util/StringSerialisation.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/fastx/isFastQ.hpp>
#include <libmaus/fastx/FastAReaderSplit.hpp>
#include <libmaus/fastx/FastQReaderSplit.hpp>
#include <ostream>
#include <vector>

namespace libmaus
{
	namespace fastx
	{
		struct ConcatWriter
		{
			struct ReadFileInfo
			{
				uint64_t numreads;
				uint64_t numsyms;
				uint64_t maxlen;
			
				ReadFileInfo() : numreads(0), numsyms(0), maxlen(0) {}
				ReadFileInfo(uint64_t const rnumreads, uint64_t const rnumsyms, uint64_t const rmaxlen)
				: numreads(rnumreads), numsyms(rnumsyms), maxlen(rmaxlen) {}
				~ReadFileInfo() {}
				
				ReadFileInfo(std::istream & in)
				: numreads(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				  numsyms(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				  maxlen(::libmaus::util::NumberSerialisation::deserialiseNumber(in))
				{
				
				}
				
				void serialize(std::ostream & out) const
				{
					::libmaus::util::NumberSerialisation::serialiseNumber(out,numreads);
					::libmaus::util::NumberSerialisation::serialiseNumber(out,numsyms);
					::libmaus::util::NumberSerialisation::serialiseNumber(out,maxlen);
				}
			};

			static uint64_t const base = ((1ull<<3) - 5 - 1);
			static uint64_t const longbase = ((1ull << 8) - 5 - 1);

			std::vector < std::string > const filenames;
			ReadFileInfo const rfi;
			uint64_t const numreads;
			uint64_t const numreadsyms;
			uint64_t const maxlen;
			uint64_t const expo;
			uint64_t const totalreads;
			uint64_t const totalsyms;
			uint64_t const numsymbits;
			uint64_t const numsymwords;
			uint64_t const numpadbits;
			uint64_t const numpadwords;
			uint64_t const numtotalwords;
			uint64_t const totalarraymem;
			uint64_t const maxtotalmem;
			uint64_t const longexpo;
			uint64_t const bytearraysize;

			void serialize(std::ostream & out) const
			{
				::libmaus::util::StringSerialisation::serialiseStringVector(out,filenames);
				rfi.serialize(out);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,numreads);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,numreadsyms);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,maxlen);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,expo);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,totalreads);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,totalsyms);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,numsymbits);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,numsymwords);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,numpadbits);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,numpadwords);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,numtotalwords);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,totalarraymem);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,maxtotalmem);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,longexpo);
				::libmaus::util::NumberSerialisation::serialiseNumber(out,bytearraysize);
			}

			ConcatWriter(
				std::istream & in
			)
			:
				filenames ( ::libmaus::util::StringSerialisation::deserialiseStringVector(in) ),
				rfi(in),
				numreads(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				numreadsyms(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				maxlen(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				expo(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				totalreads(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				totalsyms(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				numsymbits(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				numsymwords(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				numpadbits(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				numpadwords(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				numtotalwords(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				totalarraymem(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				maxtotalmem(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				longexpo(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
				bytearraysize(::libmaus::util::NumberSerialisation::deserialiseNumber(in))
			{}
				
			uint64_t getByteSymbolCount() const
			{
				return numreadsyms + numreads * longexpo + 1;
			}

			ConcatWriter(
				std::vector < std::string > const & rfilenames, 
				uint64_t const rmaxtotalmem
			)
			: 
			  // input file names
			  filenames(rfilenames), 
			  // count reads and symbols
			  rfi(countSwitch(filenames)), 
			  // number of reads
			  numreads(rfi.numreads), 
			  // number of read symbols
			  numreadsyms(rfi.numsyms), 
			  // maximum read length
			  maxlen(rfi.maxlen),
			  // exponent \lceil \log_2 numreads \rceil
			  expo(computeTermExponent(numreads)),
			  // total number of reads
			  totalreads(numreads), 
			  // total number of symbols (compact version)
			  totalsyms(1 + numreadsyms + numreads*(expo+2) + 1),
			  // number of symbol bits (3 bits per symbol in compact version)
			  numsymbits ( (totalsyms * 3) ),
			  // number of symbol words (compact version)
			  numsymwords ( ( numsymbits + 63 ) / 64 ),
			  // number of pad bits (compact version)
			  numpadbits ( (maxlen + expo + 2) * 3 ),
			  // number of pad words
			  numpadwords ( 2 * ( (numpadbits + 63)/64 ) ),
			  numtotalwords ( numsymwords + numpadwords ),
			  totalarraymem ( numtotalwords * sizeof(uint64_t) ),
			  maxtotalmem(rmaxtotalmem),
			  longexpo(computeLongTermExponent(numreads)),
			  bytearraysize ( numreadsyms + numreads * longexpo + 1 + maxlen )
			{
				std::cerr << "numreads " << numreads 
					<< " base " << base << " expo " << expo 
					<< " longbase " << longbase << " longexpo " << longexpo 
					<< std::endl;

		#define COMPACTCONCAT
		#if defined(COMPACTCONCAT)
				if ( totalarraymem > maxtotalmem )
				{
					::libmaus::exception::LibMausException ex;
					ex.getStream() << "file requires " << totalarraymem << " bytes but we only have " << maxtotalmem;
					ex.finish();
					throw ex;
				}
		#else
				if ( bytearraysize > maxtotalmem )
				{
					::libmaus::exception::LibMausException ex;
					ex.getStream() << "file requires " << bytearraysize << " bytes but we only have " << maxtotalmem;
					ex.finish();
					throw ex;
				}
		#endif
			}

			static unsigned int computeTermExponent(uint64_t const numreads)
			{
				unsigned int expo = 0;
				uint64_t p = 1;
			
				while ( !(numreads < p) )
				{
					p *= base;
					expo++;
				}

				return expo;
			}

			static unsigned int computeLongTermExponent(uint64_t const numreads)
			{
				unsigned int expo = 0;
				uint64_t p = 1;
			
				while ( !(numreads < p) )
				{
					p *= longbase;
					expo++;
				}

				return expo;
			}


			template<typename reader_type>
			static ReadFileInfo countPatterns(std::vector<std::string> const & filenames)
			{
				uint64_t numreads = 0;
				uint64_t numsyms = 0;
				uint64_t maxlen = 0;
				typedef typename reader_type::pattern_type pattern_type;
				reader_type reader(filenames);
				pattern_type pattern;
			
				while ( reader.getNextPatternUnlocked(pattern) )
				{
					if ( (numreads & (1024*1024-1)) == 0 )
						std::cerr << numreads << std::endl;
			
					numreads += 1;
					numsyms += pattern.getPatternLength();
					maxlen = std::max(maxlen,static_cast<uint64_t>(pattern.getPatternLength()));
				}
			
				return ReadFileInfo(numreads,numsyms,maxlen);
			}

			template<typename reader_type> void doConcatCompact(
				std::vector<std::string> const & filenames, 
				::libmaus::bitio::FastWriteBitWriter8 & W,
				::libmaus::autoarray::AutoArray<uint64_t> & B)
			{
				uint64_t const bvwords = (totalsyms + 63) / 64;
				B = ::libmaus::autoarray::AutoArray<uint64_t>(bvwords);

				reader_type reader(filenames);
				typedef typename reader_type::pattern_type pattern_type;
				pattern_type pattern;
				uint64_t totalsyms = 0;
				::libmaus::autoarray::AutoArray<uint8_t> termbuf(expo);
				uint64_t idcnt = numreads;
				
				W.write ( 1, 3 );
				totalsyms += 1;

				while ( reader.getNextPatternUnlocked(pattern) )
				{
					if ( (pattern.getPatID() & (1024*1024-1)) == 0 )
						std::cerr << pattern.getPatID() << std::endl;

					::libmaus::bitio::putBit(B.get(), totalsyms, 1);
			
					pattern.computeMapped();
					uint64_t const l = pattern.getPatternLength();
			
					for ( uint64_t i = 0; i < l; ++i )
						W.write ( pattern.mapped[i] + 3, 3);

					W.write ( 1, 3 );
			
					uint64_t id = --idcnt;

					uint8_t * t = termbuf.get() + termbuf.getN();
					for ( unsigned int i = 0; i < expo; ++i )
					{
						*(--t) = (id % base) + 1;
						id /= base;
					}
					assert ( t == termbuf.get() );
					for ( unsigned int i = 0; i < expo; ++i )
						W.write ( (*(t++)), 3 );
					
					W.write ( 1, 3 );

					totalsyms += (l + expo + 2);
				}
				W.write ( 0, 3 );
				totalsyms += 1;
			
				std::cerr << "this->totalsyms " << this->totalsyms << " totalsyms " << totalsyms << std::endl;
				assert ( totalsyms == this->totalsyms );
			}

			template<typename reader_type> void doConcatByte(
				std::vector<std::string> const & filenames, 
				uint8_t * T)
			{
				reader_type reader(filenames);
				typedef typename reader_type::pattern_type pattern_type;
				pattern_type pattern;
				uint64_t totalsyms = 0;
				::libmaus::autoarray::AutoArray<uint8_t> termbuf(longexpo);
				uint64_t idcnt = numreads;

				while ( reader.getNextPatternUnlocked(pattern) )
				{
					if ( (pattern.getPatID() & (1024*1024-1)) == 0 )
						std::cerr << pattern.getPatID() << std::endl;
			
					pattern.computeMapped();
					uint64_t const l = pattern.getPatternLength();
			
					for ( uint64_t i = 0; i < l; ++i )
						*(T++) = pattern.mapped[i] + longbase;
			
					uint64_t id = --idcnt;

					uint8_t * t = termbuf.get() + termbuf.getN();
					for ( unsigned int i = 0; i < longexpo; ++i )
					{
						*(--t) = (id % longbase) + 1;
						id /= longbase;
					}
					assert ( t == termbuf.get() );
					for ( unsigned int i = 0; i < longexpo; ++i )
						*(T++) = pattern.mapped[i] + *(t++);
					
					totalsyms += (l + longexpo);
				}
				*(T++) = 0;
				totalsyms += 1;

				for ( uint64_t i = 0; i < maxlen; ++i )
				{
					*(T++) = 0;
					totalsyms += 1;
				}
			
				std::cerr << "bytearraysize " << this->bytearraysize << " totalsyms " << totalsyms << std::endl;
				assert ( totalsyms == this->bytearraysize );
			}

			void writeSwitchCompact(
				std::vector<std::string> const & filenames, 
				::libmaus::bitio::FastWriteBitWriter8 & W,
				::libmaus::autoarray::AutoArray<uint64_t> & B)
			{
				if ( filenames.size() )
				{
					std::string const firstfilename = filenames.front();
			
					if ( ::libmaus::fastx::IsFastQ::isFastQ(firstfilename) )
					{
						doConcatCompact<libmaus::fastx::FastQReaderSplit>(filenames,W,B);
					}
					else
					{
						doConcatCompact<libmaus::fastx::FastAReaderSplit>(filenames,W,B);
					}
				}
			}

			void writeSwitchByte(
				std::vector<std::string> const & filenames, 
				uint8_t * const T)
			{
				if ( filenames.size() )
				{
					std::string const firstfilename = filenames.front();
			
					if ( ::libmaus::fastx::IsFastQ::isFastQ(firstfilename) )
					{
						doConcatByte<libmaus::fastx::FastQReaderSplit>(filenames,T);
					}
					else
					{
						doConcatByte<libmaus::fastx::FastAReaderSplit>(filenames,T);
					}
				}
			}


			::libmaus::autoarray::AutoArray<uint64_t> processCompact(::libmaus::autoarray::AutoArray<uint64_t> & B)
			{
				::libmaus::autoarray::AutoArray<uint64_t> T(numtotalwords);
				::libmaus::bitio::FastWriteBitWriter8 FWBW(T.get());

				writeSwitchCompact(filenames,FWBW,B);
				FWBW.flush();
				for ( uint64_t i = 0; i < this->numpadwords; ++i )
					FWBW.write(0,64);
				FWBW.flush();
			
				return T;
			}

			::libmaus::autoarray::AutoArray<uint8_t> processByte()
			{
				::libmaus::autoarray::AutoArray<uint8_t> T(bytearraysize);
				writeSwitchByte(filenames,T.get());	
				return T;
			}

			static ReadFileInfo countSwitch(std::vector<std::string> const & filenames)
			{
				if ( filenames.size() )
				{
					std::string const firstfilename = filenames.front();
			
					if ( ::libmaus::fastx::IsFastQ::isFastQ(firstfilename) )
					{
						return countPatterns<libmaus::fastx::FastQReaderSplit>(filenames);
					}
					else
					{
						return countPatterns<libmaus::fastx::FastAReaderSplit>(filenames);
					}
				}
				else
				{
					return ReadFileInfo(0,0,0);
				}
			}

		};
	}
}
#endif
