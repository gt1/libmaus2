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
#if ! defined(LIBMAUS2_FASTX_FASTQBGZFWRITER_HPP)
#define LIBMAUS2_FASTX_FASTQBGZFWRITER_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/lz/BgzfDeflateParallel.hpp>
#include <libmaus2/lz/BgzfDeflate.hpp>
#include <libmaus2/fastx/FastQReader.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <string>

namespace libmaus2
{
	namespace fastx
	{
		struct FastQBgzfWriter
		{
			typedef FastQBgzfWriter this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			private:
			std::string const indexfilename;
			uint64_t const patperblock;
			std::string const fifilename;
			#if defined(LIBMAUS2_FASTX_FASTQBGZFWRITER_PARALLEL)
			std::string const bgzfidxfilename;
			std::string const bgzfidxcntfilename;
			#endif

			#if defined(LIBMAUS2_FASTX_FASTQBGZFWRITER_PARALLEL)
			libmaus2::aio::OutputStreamInstance::unique_ptr_type bgzfidoutstr;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type bgzfidxcntoutstr;
			#endif
			libmaus2::aio::OutputStreamInstance::unique_ptr_type fioutstr;

			libmaus2::autoarray::AutoArray<char> C;
			uint64_t patlow;
			uint64_t blockcnt;

			#if defined(LIBMAUS2_FASTX_FASTQBGZFWRITER_PARALLEL)
			libmaus2::lz::BgzfDeflateParallel::unique_ptr_type bgzfenc;
			#else
			libmaus2::lz::BgzfDeflate<std::ostream>::unique_ptr_type bgzfenc;
			#endif

			uint64_t lnumsyms;
			uint64_t minlen;
			uint64_t maxlen;
			uint64_t pathigh;
			char * pc;
			uint64_t p;
			
			uint64_t cacc;

			struct FastQEntryStats
			{
				uint64_t namelen;
				uint64_t seqlen;
				uint64_t pluslen;
			};

			void computeEntryStats(char const * e, FastQEntryStats & entry) const
			{
				char const * la;
				la = e; while ( *e != '\n' ) { ++e; } entry.namelen = e-la; ++e ;
				la = e; while ( *e != '\n' ) { ++e; } entry.seqlen = e-la; ++e ;
				la = e; while ( *e != '\n' ) { ++e; } entry.pluslen = e-la; ++e ;
				
				entry.namelen -= 1;
				entry.pluslen -= 1;
			}

			
			uint64_t getFastQLength(uint64_t const qlen, uint64_t const nlen, uint64_t const psize)
			{
				return   
					1 /* @ */ + nlen /* name length */ + 1 /* new line */ + 
					qlen + 1 + 
					1 /* + */ + psize + 1 + 
					qlen + 1
				;
			}

			template<typename pattern_type>
			uint64_t getFastQLength(pattern_type const & pattern)
			{
				return getFastQLength(pattern.spattern.size(),pattern.sid.size(),pattern.plus.size());
			}

			void reset()
			{
				lnumsyms = 0;
				minlen = std::numeric_limits<uint64_t>::max();
				maxlen = 0;
				pc = C.begin();
				p = 0;
			}

			static std::string setupTempFile(std::string const & filename)
			{
				libmaus2::util::TempFileRemovalContainer::addTempFile(filename);	
				return filename;
			}

			void internalFlush()
			{
				if ( pathigh != patlow )
				{
					#if defined(LIBMAUS2_FASTX_FASTQBGZFWRITER_PARALLEL)
					uint64_t const bcnt = bgzfenc->writeSyncedCount(C.begin(),pc-C.begin());
					libmaus2::util::UTF8::encodeUTF8(bcnt,*bgzfidxcntoutstr);
					libmaus2::fastx::FastInterval const FI(patlow,pathigh,0,0,lnumsyms,minlen,maxlen);
					#else
					std::pair<uint64_t,uint64_t> bcntccnt = bgzfenc->writeSyncedCount(C.begin(),pc-C.begin());
					libmaus2::fastx::FastInterval const FI(patlow,pathigh,cacc,cacc+bcntccnt.second,lnumsyms,minlen,maxlen);
					cacc += bcntccnt.second;
					#endif
					
					(*fioutstr) << FI.serialise();				
					blockcnt += 1;
						
					std::cerr << FI << std::endl;
					
					reset();
					patlow = pathigh;
				}	
			}

			char const * prevStart(char const * e) const
			{
				if ( e == C.begin() )
					return 0;
				
				assert ( e[-1] == '\n' );
				// step over last/quality line's newline
				--e;
				
				// search for plus line's newline
				while ( *--e != '\n' ) {}
				// search for sequence line's newline
				while ( *--e != '\n' ) {}
				// search for id line's newline
				while ( *--e != '\n' ) {}
				// search for start of line
				while ( e != C.begin() && e[-1] != '\n' )
					--e;
					
				return e;
			}
			
			char const * nextStart(char const * e) const
			{
				while ( *e != '\n' ) { ++e; } ++e ;
				while ( *e != '\n' ) { ++e; } ++e ;
				while ( *e != '\n' ) { ++e; } ++e ;
				while ( *e != '\n' ) { ++e; } ++e ;
				
				if ( e == pc )
					return 0;
				else
					return e;
			}
			
			char const* getEnd() const
			{
				if ( pc != C.begin() )
					return pc;
				else
					return 0;
			}

			char const* getStart() const
			{
				if ( pc != C.begin() )
					return C.begin();
				else
					return 0;
			}
			

			public:
			FastQBgzfWriter(
				::std::string rindexfilename,
				uint64_t const rpatperblock,
				std::ostream & out,
				int level = Z_DEFAULT_COMPRESSION
			) : indexfilename(rindexfilename), patperblock(rpatperblock),
			    fifilename(setupTempFile(indexfilename + ".tmp.fi")), 
			    #if defined(LIBMAUS2_FASTX_FASTQBGZFWRITER_PARALLEL)
			    bgzfidxfilename(setupTempFile(indexfilename + ".tmp.bgzfidx")),
			    bgzfidxcntfilename(setupTempFile(indexfilename + ".tmp.bgzfidx.cnt")),
			    bgzfidoutstr(new libmaus2::aio::OutputStreamInstance(bgzfidxfilename)),
			    bgzfidxcntoutstr(new libmaus2::aio::OutputStreamInstance(bgzfidxcntfilename)),
			    #endif
			    fioutstr(new libmaus2::aio::OutputStreamInstance(fifilename)),
			    C(0,false), patlow(0), blockcnt(0),
			    #if defined(LIBMAUS2_FASTX_FASTQBGZFWRITER_PARALLEL)
			    bgzfenc(new libmaus2::lz::BgzfDeflateParallel(out,32,128,level,bgzfidoutstr.get())),
			    #else
			    bgzfenc(new libmaus2::lz::BgzfDeflate<std::ostream>(out,level)),
			    #endif
			    lnumsyms(0),
			    minlen(std::numeric_limits<uint64_t>::max()),
			    maxlen(0),
			    pathigh(patlow),
			    pc(C.begin()),
			    p(0),
			    cacc(0)
			{
			}

			void testPrevStart() const
			{
				char const * e = getEnd();
		
				while ( e )
				{
					std::cerr << std::string(80,'-') << std::endl;
					std::cerr << std::string(e,static_cast<char const *>(pc));
					e = prevStart(e);
				}
			}

			void testNextStart() const
			{
				char const * e = getStart();
		
				while ( e )
				{
					std::cerr << std::string(80,'-') << std::endl;
					std::cerr << std::string(e,static_cast<char const *>(pc));
					
					FastQEntryStats entry;
					computeEntryStats(e,entry);
					std::cerr << "namelen=" << entry.namelen << ",seqlen=" << entry.seqlen
						<< ",pluslen=" << entry.pluslen << std::endl;
					
					e = nextStart(e);
				}
			}
						
			void put(libmaus2::fastx::FastQReader::pattern_type const & pattern)
			{
				uint64_t const patlen = getFastQLength(pattern);
				
				while ( (C.end() - pc) < static_cast<ptrdiff_t>(patlen) )
				{
					uint64_t const off = pc-C.begin();
					uint64_t const newclen = std::max(2*C.size(),static_cast<uint64_t>(1ull));
					C.resize(newclen);
					pc = C.begin()+off;
				}

				*(pc)++ = '@';
				std::copy(pattern.sid.begin(),pattern.sid.end(),pc);
				pc += pattern.sid.size();
				*(pc++) = '\n';

				std::copy(pattern.spattern.begin(), pattern.spattern.end(),pc);
				pc += pattern.spattern.size();
				*(pc++) = '\n';

				*(pc)++ = '+';
				std::copy(pattern.plus.begin(), pattern.plus.end(),pc);
				pc += pattern.plus.size();
				*(pc++) = '\n';

				std::copy(pattern.quality.begin(), pattern.quality.end(),pc);
				pc += pattern.quality.size();
				*(pc++) = '\n';
				
				assert ( pc <= C.end() );
				
				lnumsyms += pattern.spattern.size();
				minlen = std::min(minlen,static_cast<uint64_t>(pattern.spattern.size()));
				maxlen = std::max(maxlen,static_cast<uint64_t>(pattern.spattern.size()));
				pathigh++;
				
				if ( pathigh - patlow == patperblock )
					internalFlush();
			}
			
			void flush()
			{
				if ( bgzfenc )
				{
					internalFlush();

					bgzfenc->flush();
					bgzfenc->addEOFBlock();
					bgzfenc.reset();
					fioutstr->flush();
					fioutstr.reset();

					libmaus2::aio::OutputStreamInstance indexCOS(indexfilename);
					::libmaus2::util::NumberSerialisation::serialiseNumber(indexCOS,blockcnt);
					libmaus2::aio::InputStreamInstance fiCIS(fifilename);
					
					#if defined(LIBMAUS2_FASTX_FASTQBGZFWRITER_PARALLEL)
					bgzfidoutstr->flush();
					bgzfidoutstr.reset();
					bgzfidxcntoutstr->flush();
					bgzfidxcntoutstr.reset();
					
					libmaus2::aio::InputStreamInstance bgzfidxCIS(bgzfidxfilename);
					libmaus2::aio::InputStreamInstance bgzfidxcntCIS(bgzfidxcntfilename);
					
					uint64_t uncompacc = 0;
					uint64_t compacc = 0;
					
					for ( uint64_t i = 0; i < blockcnt; ++i )
					{
						uint64_t const bgzfblocks = libmaus2::util::UTF8::decodeUTF8(bgzfidxcntCIS);
						uint64_t uncomp = 0;
						uint64_t comp = 0;
						
						for ( uint64_t j = 0; j < bgzfblocks; ++j )
						{
							uncomp += libmaus2::util::UTF8::decodeUTF8(bgzfidxCIS);
							comp += libmaus2::util::UTF8::decodeUTF8(bgzfidxCIS);
						}

						libmaus2::fastx::FastInterval FI = libmaus2::fastx::FastInterval::deserialise(fiCIS);
						FI.fileoffset = compacc;
						FI.fileoffsethigh = compacc + comp;
						
						uncompacc += uncomp;
						compacc += comp;
						
						indexCOS << FI.serialise();
					}
					
					#else

					for ( uint64_t i = 0; i < blockcnt; ++i )
					{
						libmaus2::fastx::FastInterval FI = libmaus2::fastx::FastInterval::deserialise(fiCIS);
						indexCOS << FI.serialise();
					}
					
					#endif

					indexCOS.flush();
				}
			}
		};
	}
}
#endif
