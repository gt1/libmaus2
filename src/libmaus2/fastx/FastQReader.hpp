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

#if ! defined(FASTQREADER_HPP)
#define FASTQREADER_HPP

#if defined(FASTXASYNC)
#include <libmaus2/aio/AsynchronousReader.hpp>
#endif

#include <libmaus2/aio/SynchronousFastReaderBase.hpp>
#include <libmaus2/fastx/CharBuffer.hpp>
#include <libmaus2/fastx/CharTermTable.hpp>
#include <libmaus2/fastx/FastIDBlock.hpp>
#include <libmaus2/fastx/FastInterval.hpp>
#include <libmaus2/fastx/FASTQEntry.hpp>
#include <libmaus2/fastx/PatternBlock.hpp>
#include <libmaus2/fastx/SpaceTable.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/digest/md5.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/aio/FileFragment.hpp>
#include <libmaus2/fastx/FastQElement.hpp>

#include <limits>
#include <vector>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace libmaus2
{
	namespace fastx
	{
	        template<typename _reader_base_type>
                struct FastQReaderTemplate : public _reader_base_type, public SpaceTable
                {
                        typedef _reader_base_type reader_base_type;

                        typedef FastQReaderTemplate<reader_base_type> reader_type;
                        typedef reader_type this_type;
                        typedef FASTQEntry pattern_type;
                        typedef PatternBlock<pattern_type> block_type;

                        typedef reader_type idfile_type;
                        typedef FastIDBlock idblock_type;

                        #if defined(FASTXASYNC)
                        // types for asynchronous reader
                        typedef ::libmaus2::aio::AsynchronousStreamReaderData<reader_type> stream_data_type;
                        typedef ::libmaus2::aio::AsynchronousStreamReader< ::libmaus2::aio::AsynchronousStreamReaderData<reader_type> > stream_reader_type;
                        typedef ::libmaus2::aio::AsynchronousIdData<reader_type> stream_id_type;
                        typedef ::libmaus2::aio::AsynchronousStreamReader<stream_id_type> stream_idreader_type;
                        #endif

                        typedef typename ::libmaus2::util::unique_ptr<reader_type>::type unique_ptr_type;

                        // lookup tables for scanning
                        CharTermTable atscanterm;
                        CharTermTable plusscanterm;
                        CharTermTable newlineterm;

                        // character buffers
                        CharBuffer idbuffer;
                        CharBuffer patbuffer;
                        CharBuffer plusbuffer;
                        CharBuffer qualbuffer;

                        bool foundnextmarker;
                        int qualityOffset;

                        uint64_t nextid;

			FastInterval interval;

			bool checkbytecount;

                        FastQReaderTemplate(
				std::string const & filename,
				int rqualityOffset = 0,
				unsigned int numbuffers = 16,
				unsigned int bufsize = 16*1024,
				uint64_t const fileoffset = 0,
				uint64_t const rnextid = 0)
                        : reader_base_type(filename,numbuffers,bufsize,fileoffset),
                          atscanterm('@'), plusscanterm('+'), newlineterm('\n'),
                          foundnextmarker(false),
                          qualityOffset(rqualityOffset), nextid(rnextid),
			  interval(nextid,std::numeric_limits<uint64_t>::max(),fileoffset,::libmaus2::util::GetFileSize::getFileSize(filename), 0 /* syms */, 0 /* minlen */, 0 /* maxlen */),
			  checkbytecount(true)
                        {
                                findNextMarker();
                        }

                        static unsigned int getDefaultNumberOfBuffers()
                        {
                        	return 16;
                        }

                        static unsigned int getDefaultBufferSize()
                        {
                        	return 16*1024;
                        }

                        static int getDefaultQualityOffset()
                        {
                        	return 0;
                        }

                        FastQReaderTemplate(
				std::vector<std::string> const & filenames,
				int rqualityOffset = getDefaultQualityOffset(),
				unsigned int numbuffers = getDefaultNumberOfBuffers(),
				unsigned int bufsize = getDefaultBufferSize(),
				uint64_t const fileoffset = 0,
				uint64_t const rnextid = 0)
                        : reader_base_type(filenames,numbuffers,bufsize,fileoffset),
                          atscanterm('@'), plusscanterm('+'), newlineterm('\n'),
                          foundnextmarker(false),
                          qualityOffset(rqualityOffset), nextid(rnextid),
			  interval(nextid,std::numeric_limits<uint64_t>::max(),fileoffset,::libmaus2::util::GetFileSize::getFileSize(filenames), 0 /* numsyms */, 0 /* minlen */, 0 /* maxlen */),
			  checkbytecount(true)
                        {
                                findNextMarker();
                        }


			FastQReaderTemplate(
				std::string const & filename,
				FastInterval const & rinterval,
				int rqualityOffset = getDefaultQualityOffset(),
				unsigned int const numbuffers = getDefaultNumberOfBuffers(),
				unsigned int const bufsize = getDefaultBufferSize())
			: reader_base_type(filename,numbuffers,bufsize,rinterval.fileoffset),
                          atscanterm('@'), plusscanterm('+'), newlineterm('\n'),
                          foundnextmarker(false),
                          qualityOffset(rqualityOffset), nextid(rinterval.low),
			  interval(rinterval), checkbytecount(true)
                        {
                                findNextMarker();
                        }

			FastQReaderTemplate(
				std::vector<std::string> const & filenames,
				FastInterval const & rinterval,
				int rqualityOffset = getDefaultQualityOffset(),
				unsigned int const numbuffers = getDefaultNumberOfBuffers(),
				unsigned int const bufsize = getDefaultBufferSize())
			: reader_base_type(filenames,numbuffers,bufsize,rinterval.fileoffset),
                          atscanterm('@'), plusscanterm('+'), newlineterm('\n'),
                          foundnextmarker(false),
                          qualityOffset(rqualityOffset), nextid(rinterval.low),
			  interval(rinterval), checkbytecount(true)
                        {
                                findNextMarker();
                        }

                        template<typename reader_init_type>
			FastQReaderTemplate(
			        reader_init_type * rinit,
				::std::vector<FastInterval> const & intervals,
				uint64_t const i,
                                uint64_t const bufsize = getDefaultBufferSize())
			: reader_base_type(rinit,bufsize),
                          atscanterm('@'), plusscanterm('+'), newlineterm('\n'),
                          foundnextmarker(false),
                          qualityOffset(0), nextid(intervals[i].low),
			  interval(intervals[i]), checkbytecount(true)
                        {
                                rinit->writeMessage(0,&i,1);
                                findNextMarker();
                        }

                        template<typename reader_init_type>
			FastQReaderTemplate(
			        reader_init_type * rinit,
				FastInterval const & rinterval,
                                uint64_t const bufsize = getDefaultBufferSize()
			)
			: reader_base_type(rinit,bufsize),
                          atscanterm('@'), plusscanterm('+'), newlineterm('\n'),
                          foundnextmarker(false),
                          qualityOffset(0), nextid(rinterval.low),
			  interval(rinterval), checkbytecount(true)
                        {
                                rinit->writeString(interval.serialise());
                                findNextMarker();
                        }

                        template<typename reader_init_type>
			FastQReaderTemplate(
			        reader_init_type * rinit,
			        int const rqualityOffset,
			        uint64_t const bufsize = getDefaultBufferSize()
			)
			: reader_base_type(rinit,bufsize),
                          atscanterm('@'), plusscanterm('+'), newlineterm('\n'),
                          foundnextmarker(false),
                          qualityOffset(rqualityOffset), nextid(0),
			  interval(nextid,std::numeric_limits<uint64_t>::max(),0,std::numeric_limits<uint64_t>::max(), 0 /* syms */, 0 /* minlen */, 0 /* maxlen */),
			  checkbytecount(true)
                        {
                                findNextMarker();
                        }

                        template<typename reader_init_type>
			FastQReaderTemplate(
			        reader_init_type * rinit,
			        int const rqualityOffset,
				FastInterval const & rinterval,
                                uint64_t const bufsize = getDefaultBufferSize()
			)
			: reader_base_type(rinit,bufsize),
                          atscanterm('@'), plusscanterm('+'), newlineterm('\n'),
                          foundnextmarker(false),
                          qualityOffset(rqualityOffset), nextid(rinterval.low),
			  interval(rinterval),
			  checkbytecount(true)
                        {
                                findNextMarker();
                        }


                        void findNextMarker()
                        {
                                int c;

                                while ( !atscanterm[(c=reader_base_type::getNextCharacter())] )
                                {
                                        // std::cerr << "Got " << static_cast<char>(c) << " without term, " << scanterm[c] << std::endl;
                                }

                                foundnextmarker = (c=='@');
                        }


                        static uint64_t getPatternLength(std::string const & filename)
                        {
                                reader_type reader(filename);
                                pattern_type pat;

                                if ( ! reader.getNextPatternUnlocked(pat) )
                                        return 0;
                                else
                                        return pat.patlen;
                        }

                        static uint64_t getPatternLength(std::vector<std::string> const & filenames)
                        {
                                reader_type reader(filenames);
                                pattern_type pat;

                                if ( ! reader.getNextPatternUnlocked(pat) )
                                        return 0;
                                else
                                        return pat.patlen;
                        }


                        bool skipPattern(pattern_type & pattern)
                        {
                                if ( ! foundnextmarker )
                                        return false;

                                foundnextmarker = false;

                                int c;

                                /* read id line */
                                while ( !newlineterm[c=reader_base_type::getNextCharacter()] )
                                {}

                                if ( c < 0 )
                                        return false;

                                /* read sequence line */
                                patbuffer.reset();
                                while ( !plusscanterm[(c=reader_base_type::getNextCharacter())] )
                                        if ( nospacetable[c] )
                                                patbuffer.bufferPush(c);

                                if ( c < 0 )
                                        return false;

                                pattern.patlen = patbuffer.length;

                                /* plus line */
                                while ( !newlineterm[c=reader_base_type::getNextCharacter()] )
                                {}

                                if ( c < 0 )
                                        return false;

                                /* quality line */
                                qualbuffer.reset();
                                while ( ((c=reader_base_type::getNextCharacter()) >= 0) && (qualbuffer.length < pattern.patlen) )
                                        if ( nospacetable[c] )
                                                qualbuffer.bufferPush(c);

                                if ( c < 0 )
                                        return false;

                                if ( qualbuffer.length < pattern.patlen )
                                        return false;

                                findNextMarker();

                                return true;
                        }

                        bool getNextElement(FastQElement & element)
                        {
                        	pattern_type pattern;
				if ( ! getNextPatternUnlocked(pattern) )
					return false;

				element = FastQElement(pattern);

				return true;
                        }

                        bool getNextElements(FastQElement * elements, uint64_t const n, uint64_t * const filepos = 0)
                        {
                        	if ( filepos )
                        		*filepos = getFilePointer();

                        	bool ok = true;
                        	for ( uint64_t i = 0; ok && i < n; ++i )
                        		ok = getNextElement(elements[i]);

				return ok;
                        }

                        std::vector < FastQElement > getElements()
                        {
                        	std::vector < FastQElement > V;
				FastQElement element;
				while ( getNextElement(element) )
					V.push_back(element);
				return V;
                        }

                        static std::vector < FastQElement > getElements(std::vector<std::string> const & filenames)
                        {
                        	this_type reader(filenames);
                        	return reader.getElements();
                        }

                        static std::vector < FastQElement > getElements(std::string const & filename)
                        {
                        	return getElements(std::vector<std::string>(1,filename));
                        }

                        void disableByteCountChecking()
                        {
                        	checkbytecount = false;
                        }

                        bool getNextPatternUnlocked(pattern_type & pattern)
                        {
                                /* no marker found */
                                if ( ! foundnextmarker )
                                        return false;
                                /* left region of interest in file */
				if ( nextid >= interval.high )
					return false;
				if ( checkbytecount && (reader_base_type::getC() >= (interval.fileoffsethigh-interval.fileoffset)) )
					return false;

                                foundnextmarker = false;

                                int c;

                                /* read id */
                                idbuffer.reset();
                                while ( !newlineterm[c=reader_base_type::getNextCharacter()] )
                                        idbuffer.bufferPush(c);

                                if ( c < 0 )
                                {
                                        std::cerr << "Found EOF while getting read id." << std::endl;
                                        return false;
                                }

                                /* assign buffer to string id */
                                idbuffer.assign ( pattern.sid );

                                /* read pattern */
                                patbuffer.reset();
                                while ( ((c=reader_base_type::getNextCharacter())>=0) && (!plusscanterm[c]) )
                                {
                                        assert ( c < 256 );
                                        if ( nospacetable[c] )
                                                patbuffer.bufferPush(c);
                                }

                                if ( c < 0 )
                                {
                                        std::cerr << "Found EOF while getting read sequence." << std::endl;
                                        return false;
                                }

                                patbuffer.assign(pattern.spattern);
                                pattern.pattern = pattern.spattern.c_str();
                                pattern.patlen = pattern.spattern.size();

                                /* read plus line */
                                plusbuffer.reset();
                                while ( !newlineterm[c=reader_base_type::getNextCharacter()] )
                                        plusbuffer.bufferPush(c);

                                if ( c < 0 )
                                {
                                        std::cerr << "Found EOF while getting plus line." << std::endl;
                                        return false;
                                }
                                plusbuffer.assign(pattern.plus);

                                /* read quality line */
                                qualbuffer.reset();
                                while ( ((c=reader_base_type::getNextCharacter()) >= 0) && (qualbuffer.length < pattern.patlen) )
                                        if ( nospacetable[c] )
                                                qualbuffer.bufferPush(c-qualityOffset);

                                if ( c < 0 )
                                {
                                        std::cerr << "Found EOF while getting quality line." << std::endl;
                                        return false;
                                }

                                if ( qualbuffer.length < pattern.patlen )
                                        return false;

                                qualbuffer.assign ( pattern.quality );

                                pattern.patid = nextid++;

                                /* search for next marker */
                                findNextMarker();

                                return true;
                        }

                        unsigned int fillPatternBlock(
                                pattern_type * pat,
                                unsigned int const s)
                        {
                                unsigned int i = 0;

                                while ( i < s )
                                {
                                        if ( getNextPatternUnlocked(pat[i]) )
                                                ++i;
                                        else
                                                break;
                                }

                                return i;
                        }

                        unsigned int fillIdBlock(FastIDBlock & block, unsigned int const s)
                        {
                                block.setup(s);
                                block.idbase = nextid;

                                unsigned int i = 0;

                                while ( i < block.numid )
                                {
                                        pattern_type pat;

                                        if ( getNextPatternUnlocked(pat) )
                                                block.ids[i++] = pat.sid;
                                        else
                                                break;
                                }

                                block.blocksize = i;

                                return i;
                        }

                        int getOffset()
                        {
                                FASTQEntry entry;
                                while ( getNextPatternUnlocked(entry) )
                                {
                                        std::string const & quality = entry.quality;

                                        for ( unsigned int i = 0; i < quality.size(); ++i )
                                                // Sanger
                                                if ( quality[i] <= 54 )
                                                        return 33;
                                                // Illumina
                                                else if ( quality[i] >= 94 )
                                                        return 64;
                                }

                                return 0;

                        }

			static int getOffset(std::string const & inputfile)
			{
				return getOffset(std::vector<std::string>(1,inputfile));
			}

                        static int getOffset(std::vector<std::string> const & inputfiles)
                        {
                                reader_type file ( inputfiles );
                                return file.getOffset();
                        }

                        /**
                         * pattern counting
                         **/
                        uint64_t countPatterns()
                        {
                                pattern_type pattern;
                                uint64_t count = 0;

                                while ( skipPattern(pattern) )
                                        ++count;

                                return count;
                        }

                        static uint64_t countPatterns(std::string const & filename)
                        {
                                reader_type reader(filename);
                                return reader.countPatterns();
                        }
                        static uint64_t countPatterns(std::vector<std::string> const & filenames)
                        {
                                reader_type reader(filenames);
                                return reader.countPatterns();
                        }

                        static PatternCount countPatternsInInterval(std::vector<std::string> const & filenames, uint64_t const apos, uint64_t const bpos)
                        {
                                reader_type reader(filenames,0,16,16*1024,apos,0);

                                pattern_type pattern;
                                uint64_t numpat = 0;
                                uint64_t numsyms = 0;
                                uint64_t minlen = std::numeric_limits<uint64_t>::max();
                                uint64_t maxlen = 0;

                                while ( apos + reader.getC() < bpos && reader.skipPattern(pattern) )
                                {
                                	numpat++;
                                        numsyms += pattern.patlen;
                                        maxlen = std::max(maxlen,static_cast<uint64_t>(pattern.patlen));
                                        minlen = std::min(minlen,static_cast<uint64_t>(pattern.patlen));
                                }

				return PatternCount(numpat,numsyms,minlen,maxlen);
                                // return std::pair<uint64_t,uint64_t>(numpat,numsyms);
                        }

                        static std::string getNameAtPos(std::vector<std::string> const & filenames, uint64_t const pos)
                        {
                        	uint64_t const flen = ::libmaus2::util::GetFileSize::getFileSize(filenames);
				assert ( pos < flen );
                                ::libmaus2::aio::SynchronousFastReaderBase istr(filenames,16,1024,pos);
                                assert ( istr.getNextCharacter() == '@' );
                                std::string name;
                                bool const ok = istr.getLine(name);
                                assert ( ok );
                                return name;
                        }

			template<typename strip_type>
                        static std::vector < ::libmaus2::fastx::FastInterval > computeCommonNameAlignedFrags(
                        	std::vector<std::string> const & filenames, uint64_t const numfrags, uint64_t const mod, strip_type & strip
			)
			{
                        	uint64_t const flen = ::libmaus2::util::GetFileSize::getFileSize(filenames);
                        	uint64_t const fragsize = ( flen + numfrags - 1 ) / numfrags;
                        	std::vector < uint64_t > fragstarts;

                        	for ( uint64_t f = 0; f < numfrags; ++f )
                        	{
                        		uint64_t const ppos = std::min(f * fragsize,flen);
                        		#if 0
                        		std::cerr << "ppos=" << ppos << std::endl;
                        		#endif
                        		uint64_t const fragpos = searchNextStartCommonName(filenames,ppos,mod,strip);
                        		fragstarts.push_back(fragpos);
                        	}

                        	fragstarts.push_back(flen);

                        	std::vector < ::libmaus2::fastx::FastInterval > V;

                        	for ( uint64_t f = 1; f < fragstarts.size(); ++f )
                        		if ( fragstarts[f-1] != fragstarts[f] )
                        		{
                        			V.push_back(
	                        			::libmaus2::fastx::FastInterval(
        	                				0,::std::numeric_limits<uint64_t>::max(),
                	        				fragstarts[f-1],fragstarts[f],
                        					::std::numeric_limits<uint64_t>::max(),
                        					0,
                        					::std::numeric_limits<uint64_t>::max()
                        					)
							);
					}

                        	return V;
			}

			template<typename strip_type>
                        static uint64_t searchNextStartCommonName(
                        	std::vector<std::string> const & filenames, uint64_t const pos, uint64_t const mod, strip_type & strip)
                        {
                        	uint64_t const flen = ::libmaus2::util::GetFileSize::getFileSize(filenames);

                        	assert ( mod );
                        	if ( pos >= flen )
                        		return flen;

                        	std::deque<uint64_t> startvec;
                        	std::deque<std::string> namevec;

                        	uint64_t prepos = pos;
                        	for ( uint64_t i = 0; i < mod; ++i )
                        	{
                        		uint64_t const nextpos = searchNextStart(filenames,prepos);
                        		if ( nextpos == flen )
                        			return flen;
                        		startvec.push_back(nextpos);
                        		namevec.push_back(strip(getNameAtPos(filenames,nextpos)));
					prepos = nextpos+1;
				}

				bool namesequal = true;

				for ( uint64_t i = 1; i < mod; ++i )
					namesequal = namesequal && (namevec[i] == namevec[0]);

				while ( ! namesequal )
				{
					startvec.pop_front();
					namevec.pop_front();
					assert ( startvec.size() == namevec.size() );
					assert ( startvec.size() );
					uint64_t const nextpos = searchNextStart(filenames,startvec.back()+1);

					if ( nextpos == flen )
						return flen;

					startvec.push_back(nextpos);
					namevec.push_back(strip(getNameAtPos(filenames,nextpos)));

					namesequal = true;
					for ( uint64_t i = 1; i < mod; ++i )
						namesequal = namesequal && (namevec[i] == namevec[0]);
				}

				return startvec[0];
                        }

                        /*
                         * search next pattern start. Does not work for files with newlines in data
                         */
                        static uint64_t searchNextStart(std::vector<std::string> const & filenames, uint64_t const pos)
                        {
                                uint64_t const flen = ::libmaus2::util::GetFileSize::getFileSize(filenames);
                                ::libmaus2::aio::SynchronousFastReaderBase istr(filenames,16,1024,pos);

                                if ( pos )
                                {
                                        int c = -1;

                                        while ( (c=istr.getNextCharacter()) >= 0 && c != '\n' )
                                        {

                                        }

                                        if ( c < 0 )
                                                return flen;

                                        assert ( c == '\n' );
                                }

                                std::string lines[4];
                                uint64_t linestarts[4] = {0,0,0,0};
                                bool running = true;

                                while ( running )
                                {
                                        for ( uint64_t i = 1; i < 4; ++i )
                                        {
                                                lines[i-1] = lines[i];
                                                linestarts[i-1] = linestarts[i];
                                        }

                                        linestarts[3] = istr.getC();
                                        running = running && istr.getLine(lines[3]);

                                        if (
                                                lines[0].size() && lines[1].size() && lines[2].size() && lines[3].size() &&
                                                lines[0][0] == '@' &&
                                                lines[2][0] == '+' &&
                                                lines[1].size() == lines[3].size()
                                        )
                                        {
                                                #if 0
                                                std::cerr << lines[0] << std::endl;
                                                std::cerr << lines[1] << std::endl;
                                                std::cerr << lines[2] << std::endl;
                                                std::cerr << lines[3] << std::endl;
                                                #endif

                                                return pos + linestarts[0];
                                        }
                                }

                                return flen;
                        }

                        /**
                         * compute intervals for fracs parts
                         **/
                        static std::vector<FastInterval> parallelIndex(
                        	std::vector<std::string> const & filenames, uint64_t const fracs,
                        	uint64_t const mod /* = 1 */,
                        	bool const verbose /* = false */,
                        	uint64_t const numthreads
			)
                        {
                                std::string hash;
                                bool const havehash = ::libmaus2::util::MD5::md5(filenames,fracs,hash);
                                hash += ".idx";

                                if ( havehash && ::libmaus2::util::GetFileSize::fileExists(hash) )
                                {
                                	// std::cerr << "Loading index from " << hash << std::endl;
                                        libmaus2::aio::InputStreamInstance istr(hash);
                                        return ::libmaus2::fastx::FastInterval::deserialiseVector(istr);
                                }
                                else if ( ! fracs )
                                {
                                	return std::vector < ::libmaus2::fastx::FastInterval >();
                                }
                                else
                                {
                                        uint64_t const flen = ::libmaus2::util::GetFileSize::getFileSize(filenames);
                                        uint64_t const fracsize = (flen+fracs-1)/fracs;
                                        std::vector < uint64_t > fracstarts(fracs+1);
                                        fracstarts.back() = flen;

                                        if ( verbose )
                                        	std::cerr << "Searching " << fracs << " frac starts...";
                                        #if defined(_OPENMP)
                                        #pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
                                        #endif
                                        for ( int64_t i = 0; i < static_cast<int64_t>(fracs); ++i )
                                                fracstarts[i] = searchNextStart(filenames,i*fracsize);
                                        if ( verbose )
                                        	std::cerr << "done." << std::endl;

					// keep non-empty intervals
					uint64_t jj = 0;
					if ( fracstarts.size() )
					{
						jj = 1;
						for ( uint64_t i = 1; i < fracstarts.size(); ++i )
							if ( fracstarts[i] != fracstarts[i-1] )
								fracstarts[jj++] = fracstarts[i];
						fracstarts.resize(jj);
					}

					// compute interval lengths in number of reads
                                        std::vector< PatternCount > intlen(fracstarts.size() ? (fracstarts.size()-1) : 0);

                                        if ( verbose )
                                        	std::cerr << "Counting patterns...";
                                        #if defined(_OPENMP)
                                        #pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
                                        #endif
                                        for ( int64_t i = 1; i < static_cast<int64_t>(fracstarts.size()); ++i )
                                        	intlen.at(i-1) = countPatternsInInterval(filenames,fracstarts.at(i-1),fracstarts.at(i));
					if ( verbose )
						std::cerr << "done." << std::endl;

					// construct intervals using counters
					if ( verbose )
						std::cerr << "Realigning intervals...";
					uint64_t ilow = 0;
					std::vector < ::libmaus2::fastx::FastInterval > FIV;
					for ( uint64_t i = 0; i < intlen.size(); ++i )
					{
						// std::cerr << "intlen[" << i << "]=" << intlen[i].numpat << std::endl;

						// take reads from next interval if necessary
						while ( intlen[i].numpat % mod )
						{
							uint64_t nextnonempty = i+1;

							while (
								(nextnonempty < intlen.size())
								&&
								(!intlen[nextnonempty].numpat)
							)
								nextnonempty++;

							// there has to be a next non empty interval
							assert ( nextnonempty != intlen.size() );
							// next start cannot be end of file
							assert ( fracstarts[nextnonempty] != flen );
							// next interval must not be empty
							assert ( intlen[nextnonempty].numpat );

							intlen[i].numpat           ++;
							intlen[nextnonempty].numpat--;
							intlen[i].minlen = std::min(intlen[i].minlen,intlen[nextnonempty].minlen);
							intlen[i].maxlen = std::max(intlen[i].maxlen,intlen[nextnonempty].maxlen);
							fracstarts.at(nextnonempty) = searchNextStart(
								filenames,fracstarts.at(nextnonempty)+1);
						}

						uint64_t ihigh = ilow+intlen[i].numpat;

						if ( ilow != ihigh )
						{
							FIV.push_back(
								::libmaus2::fastx::FastInterval(
									ilow,ihigh,
									fracstarts[i],fracstarts[i+1],
									intlen[i].numsyms,
									intlen[i].minlen,
									intlen[i].maxlen
								)
							);
						}
						ilow = ihigh;

						// std::cerr << "*intlen[" << i << "]=" << intlen[i].numpat << std::endl;
					}
					if ( verbose )
						std::cerr << "done." << std::endl;

					#if 0
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t i = 0; i < static_cast<int64_t>(FIV.size()); ++i )
					{
						PatternCount const numpat = countPatternsInInterval(
							filenames,
							FIV[i].fileoffset,
							FIV[i].fileoffsethigh);

						bool const ok = (
							(FIV[i].high-FIV[i].low) &&
							((FIV[i].high-FIV[i].low) % mod == 0) &&
							((FIV[i].high-FIV[i].low) == numpat.numpat) &&
							(FIV[i].minlen <= numpat.minlen) &&
							(FIV[i].maxlen >= numpat.maxlen)
						);

						if ( !ok )
						{
							std::cerr << "FAILURE: " << FIV[i] << std::endl;
							assert ( ok );
						}
					}
					#endif

					if ( havehash )
                                        {
                                                libmaus2::aio::OutputStreamInstance ostr(hash);
                                                ::libmaus2::fastx::FastInterval::serialiseVector(ostr,FIV);
                                                ostr.flush();
                                        }

                                        return FIV;
                                }
                        }

                        uint64_t getFilePointer() const
                        {
                        	return foundnextmarker ? (reader_base_type::getC() - 1)  :  (reader_base_type::getC());
                        }

			std::vector<FastInterval> enumerateOffsets(uint64_t const steps = 1)
                        {
				std::vector<FastInterval> V;

				if ( foundnextmarker )
				{
                	                pattern_type pattern;

					uint64_t count = 0;

					do
					{
						uint64_t const o = reader_base_type::getC() - 1;
						uint64_t const scount = count;
						uint64_t symcount = 0;
						uint64_t minlen = std::numeric_limits<uint64_t>::max();
						uint64_t maxlen = 0;

						for ( uint64_t i = 0; i < steps && foundnextmarker; ++i )
						{
							skipPattern(pattern);
							symcount += pattern.patlen;
							minlen = std::min(minlen,static_cast<uint64_t>(pattern.patlen));
							maxlen = std::max(maxlen,static_cast<uint64_t>(pattern.patlen));
							++count;
						}

						std::cerr << "[" << scount << "," << count << ") at " << o << std::endl;

                                                uint64_t const endo =
                                                        foundnextmarker ?
                                                                (reader_base_type::getC() - 1)
                                                                :
                                                                (reader_base_type::getC());
						V.push_back(FastInterval(scount,count,o,endo,symcount,minlen,maxlen));
					}
                        	        while ( foundnextmarker );
				}

				return V;
                        }
                        static std::vector<FastInterval> enumerateOffsets(std::string const & filename, uint64_t const steps = 1)
                        {
                                reader_type reader(filename);
				return reader.enumerateOffsets(steps);
                        }

                        static std::vector<FastInterval> enumerateOffsets(std::vector<std::string> const & filenames,
				uint64_t const steps = 1)
                        {
                                reader_type reader(filenames);
				return reader.enumerateOffsets(steps);
                        }

                        static std::string getIndexFileName(std::string const & filename, uint64_t const steps)
                        {
                                std::ostringstream ostr;
                                ostr << filename << "_" << steps << ".idx";
                                return ostr.str();
                        }

                        static std::vector<FastInterval> buildIndex(std::string const & filename, uint64_t const steps = 1)
                        {
				std::string const indexfilename = getIndexFileName(filename,steps);

				if ( ::libmaus2::util::GetFileSize::fileExists ( indexfilename ) )
				{
				        libmaus2::aio::InputStreamInstance istr(indexfilename);
				        std::vector < FastInterval > intervals = ::libmaus2::fastx::FastInterval::deserialiseVector(istr);
				        return intervals;
				}
				else
				{
                                        reader_type reader(filename);
	        			std::vector<FastInterval> intervals = reader.enumerateOffsets(steps);

        				libmaus2::aio::OutputStreamInstance ostr(indexfilename);
	        			FastInterval::serialiseVector(ostr,intervals);
		        		ostr.flush();

			        	return intervals;
                                }
                        }

                        static std::vector<FastInterval> buildIndex(std::vector < std::string > const & filenames, uint64_t const steps /* = 1 */, uint64_t const numthreads)
                        {
                                std::vector < std::vector < FastInterval > > subintervalvec(filenames.size());

                                #if defined(_OPENMP)
                                #pragma omp parallel for num_threads(numthreads)
                                #endif
                                for ( int64_t i = 0; i < static_cast<int64_t>(filenames.size()); ++i )
                                        subintervalvec[i] = buildIndex(filenames[i], steps);

                                std::vector < FastInterval > intervals;
                                for ( uint64_t i = 0; i < filenames.size(); ++i )
                                {
                                        std::vector<FastInterval> const subintervals = subintervalvec[i];

                                        uint64_t const eo = intervals.size() ? intervals.back().fileoffsethigh : 0;
                                        uint64_t const eh = intervals.size() ? intervals.back().high : 0;

                                        for ( uint64_t j = 0; j < subintervals.size(); ++j )
                                        {
                                                FastInterval fi = subintervals[j];
                                                fi.fileoffset += eo;
                                                fi.fileoffsethigh += eo;
                                                fi.low += eh;
                                                fi.high += eh;
                                                intervals.push_back(fi);
                                        }
                                }

                                return intervals;
                        }

			static ::libmaus2::util::Histogram::unique_ptr_type getHistogram(std::vector<std::string> const & filenames, FastInterval const & rinterval)
			{
		                reader_type reader(filenames,rinterval);
			        pattern_type pattern;
			        ::libmaus2::util::Histogram::unique_ptr_type Phist(new ::libmaus2::util::Histogram());
			        ::libmaus2::util::Histogram & hist = *Phist;

			        while ( reader.getNextPatternUnlocked(pattern) )
			        {
			                for ( uint64_t i = 0; i < pattern.patlen; ++i )
			                        hist (
			                                ::libmaus2::fastx::remapChar(::libmaus2::fastx::mapChar(pattern.pattern[i]))
                                                );
			        }

			        return UNIQUE_PTR_MOVE(Phist);
			}

			static ::libmaus2::util::Histogram::unique_ptr_type getHistogram(std::vector<std::string> const & filenames, std::vector<FastInterval> const & rinterval, uint64_t const numthreads)
			{
			        ::libmaus2::util::Histogram::unique_ptr_type Phist(new ::libmaus2::util::Histogram());
			        ::libmaus2::util::Histogram & hist = *Phist;
			        ::libmaus2::parallel::OMPLock lock;

                                #if defined(_OPENMP)
                                #pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
                                #endif
                                for ( int64_t i = 0; i < static_cast<int64_t>(rinterval.size()); ++i )
                                {
                                        ::libmaus2::util::Histogram::unique_ptr_type uhist = getHistogram(filenames,rinterval[i]);
                                        lock.lock();
                                        hist.merge ( *uhist );
                                        lock.unlock();
                                }

                                return UNIQUE_PTR_MOVE(Phist);
			}

			static std::vector < ::libmaus2::aio::FileFragment > getDataFragments(std::vector < std::string > const & filenames)
			{
			        std::vector < ::libmaus2::aio::FileFragment > fragments;
			        for ( uint64_t i = 0; i < filenames.size(); ++i )
			                fragments.push_back(getDataFragment(filenames[i]));

				return fragments;
			}

			static ::libmaus2::aio::FileFragment getDataFragment(std::string const & filename)
			{
			        return ::libmaus2::aio::FileFragment ( filename, 0, ::libmaus2::util::GetFileSize::getFileSize(filename) );
			}

                };

                typedef FastQReaderTemplate< ::libmaus2::aio::SynchronousFastReaderBase > FastQReader;
        }
}
#endif
