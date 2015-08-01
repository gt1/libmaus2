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


#if ! defined(FASTAREADER_HPP)
#define FASTAREADER_HPP

#if defined(FASTXASYNC)
#include <libmaus2/aio/AsynchronousReader.hpp>
#endif

#include <libmaus2/aio/SynchronousFastReaderBase.hpp>
#include <libmaus2/fastx/Pattern.hpp>
#include <libmaus2/fastx/SpaceTable.hpp>
#include <libmaus2/fastx/CharTermTable.hpp>
#include <libmaus2/fastx/CharBuffer.hpp>
#include <libmaus2/fastx/PatternBlock.hpp>
#include <libmaus2/fastx/FastIDBlock.hpp>
#include <libmaus2/fastx/FastInterval.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/aio/FileFragment.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/aio/CheckedOutputStream.hpp>
#include <limits>
#include <vector>

namespace libmaus2
{
	namespace fastx
	{
	        template<typename _reader_base_type>
                struct FastAReaderTemplate : public _reader_base_type, public SpaceTable
                {
                        typedef _reader_base_type reader_base_type;

                        typedef FastAReaderTemplate reader_type;
                        typedef Pattern pattern_type;
                        typedef PatternBlock<pattern_type> block_type;
                        
                        typedef reader_type idfile_type;
                        typedef FastIDBlock idblock_type;

                        #if defined(FASTXASYNC)
                        typedef ::libmaus2::aio::AsynchronousStreamReaderData<reader_type> stream_data_type;
                        typedef ::libmaus2::aio::AsynchronousStreamReader< ::libmaus2::aio::AsynchronousStreamReaderData<reader_type> > stream_reader_type;
                        typedef ::libmaus2::aio::AsynchronousIdData<reader_type> stream_id_type;
                        typedef ::libmaus2::aio::AsynchronousStreamReader<stream_id_type> stream_idreader_type;
                        #endif

                        typedef typename ::libmaus2::util::unique_ptr<reader_type>::type unique_ptr_type;

                        CharTermTable scanterm;
                        CharTermTable newlineterm;

                        CharBuffer idbuffer;	
                        CharBuffer patbuffer;

                        bool foundnextmarker;
                        uint64_t nextid;

			FastInterval const interval;
                        
                        FastAReaderTemplate(
				std::string const & filename, 
				int = -1 /* qualityOffset*/, 
				unsigned int numbuffers = 16, 
				unsigned int bufsize = 16*1024,
				uint64_t const fileoffset = 0,
				uint64_t const rnextid = 0)
                        : reader_base_type(filename,numbuffers,bufsize,fileoffset),
                          scanterm('>'), newlineterm('\n'),
                          foundnextmarker(false), nextid(rnextid),
 			  interval(nextid, std::numeric_limits<uint64_t>::max(), fileoffset, ::libmaus2::util::GetFileSize::getFileSize(filename), 0 /* syms */, 0/*minlen*/, 0/*maxlen*/)
                        {
                                findNextMarker();
                        }

                        FastAReaderTemplate(
				std::vector<std::string> const & filenames, 
				int = -1 /* qualityOffset*/, 
				unsigned int numbuffers = 16, 
				unsigned int bufsize = 16*1024,
				uint64_t const fileoffset = 0,
				uint64_t const rnextid = 0)
                        : reader_base_type(filenames,numbuffers,bufsize,fileoffset),
                          scanterm('>'), newlineterm('\n'),
                          foundnextmarker(false), nextid(rnextid),
			  interval(nextid, std::numeric_limits<uint64_t>::max(), fileoffset, ::libmaus2::util::GetFileSize::getFileSize(filenames), 0 /* syms */, 0/*minlen*/, 0 /*maxlen*/)
                        {
                                findNextMarker();
                        }

			FastAReaderTemplate(
				std::string const & filename,
				FastInterval const & rinterval,
				int = -1 /* qualityOffset*/,
				unsigned int numbuffers = 16,
				unsigned int bufsize = 16*1024)
			: reader_base_type(filename,numbuffers,bufsize,rinterval.fileoffset),
                          scanterm('>'), newlineterm('\n'),
                          foundnextmarker(false), nextid(rinterval.low),
			  interval(rinterval)
			{
				findNextMarker();
			}

			FastAReaderTemplate(
				std::vector<std::string> const & filenames,
				FastInterval const & rinterval,
				int = -1 /* qualityOffset*/,
				unsigned int numbuffers = 16,
				unsigned int bufsize = 16*1024)
			: reader_base_type(filenames,numbuffers,bufsize,rinterval.fileoffset),
                          scanterm('>'), newlineterm('\n'),
                          foundnextmarker(false), nextid(rinterval.low),
			  interval(rinterval)
			{
				findNextMarker();
			}

			template<typename reader_init_type>
			FastAReaderTemplate(
			        reader_init_type * rinit,
			        std::vector<FastInterval> const & intervals,
			        uint64_t const i,
				uint64_t const bufsize = 16*1024)
			: reader_base_type(rinit,bufsize),
                          scanterm('>'), newlineterm('\n'),
                          foundnextmarker(false), nextid(intervals[i].low),
			  interval(intervals[i])
			{
			        rinit->writeMessage(0,&i,1);
				findNextMarker();
			}

			template<typename reader_init_type>
			FastAReaderTemplate(reader_init_type * rinit, uint64_t const bufsize = 16*1024)
			: reader_base_type(rinit,bufsize),
                          scanterm('>'), newlineterm('\n'),
                          foundnextmarker(false), nextid(0),
 			  interval(nextid, std::numeric_limits<uint64_t>::max(), 0, 
 			           std::numeric_limits<uint64_t>::max(), 0 /* syms */, 0/*minlen*/, 0/*maxlen*/)
			{
				findNextMarker();
			}

			template<typename reader_init_type>
			FastAReaderTemplate(
			        reader_init_type * rinit,
			        FastInterval const & FI,
				uint64_t const bufsize = 16*1024
			)
			: reader_base_type(rinit,bufsize),
                          scanterm('>'), newlineterm('\n'),
                          foundnextmarker(false), nextid(FI.low),
			  interval(FI)
			{
				findNextMarker();
			}
                        
                        void findNextMarker()
                        {
                                int c;
                                
                                while ( !scanterm[(c=reader_base_type::getNextCharacter())] )
                                {
                                        // std::cerr << "Got " << static_cast<char>(c) << " without term, " << scanterm[c] << std::endl;
                                }
                                
                                foundnextmarker = (c=='>');
                        }
                        
                        uint64_t countPatterns()
                        {
                                uint64_t count = 0;
                                
                                while ( skipPattern() )
                                        ++count;

                                return count;
                        }

                        std::vector<FastInterval> enumerateOffsets(uint64_t const steps = 1)
                        {
				std::vector<FastInterval> V;

				if ( foundnextmarker )
				{
	                                uint64_t count = 0;
                                
					do
					{
						uint64_t const o = reader_base_type::getC() - 1;
						uint64_t const scount = count;
						uint64_t gpatlen = 0;
						uint64_t minlen = std::numeric_limits<uint64_t>::max();
						uint64_t maxlen = 0;

						for ( uint64_t i = 0; i < steps && foundnextmarker; ++i )
						{
						        uint64_t patlen;
							skipPattern(patlen);
							count++;
							gpatlen += patlen;
							minlen = std::min(minlen,static_cast<uint64_t>(patlen));
							maxlen = std::max(maxlen,static_cast<uint64_t>(patlen));
						}

                                                uint64_t const endo = 
                                                        foundnextmarker ? 
                                                                (reader_base_type::getC() - 1) 
                                                                : 
                                                                (reader_base_type::getC());
						V.push_back(FastInterval(scount,count,o,endo,gpatlen,minlen,maxlen));
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
				        std::ifstream istr(indexfilename.c_str(), std::ios::binary);
				        std::vector < FastInterval > intervals = ::libmaus2::fastx::FastInterval::deserialiseVector(istr);
				        return intervals;
				}
				else
				{
                                        reader_type reader(filename);
	        			std::vector<FastInterval> intervals = reader.enumerateOffsets(steps);
				
        				std::ofstream ostr(indexfilename.c_str(), std::ios::binary);
	        			FastInterval::serialiseVector(ostr,intervals);
		        		ostr.flush();
			        	ostr.close();
				
			        	return intervals;
                                }
                        }

                        static std::vector<FastInterval> buildIndex(std::vector < std::string > const & filenames, uint64_t const steps = 1)
                        {
                                std::vector < FastInterval > intervals;
                        
                                for ( uint64_t i = 0; i < filenames.size(); ++i )
                                {
                                        std::vector<FastInterval> subintervals = buildIndex(filenames[i], steps);
                                        
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

                        static std::vector<FastInterval> enumerateOffsets(std::vector<std::string> const & filenames, 
				uint64_t const steps = 1)
                        {
                                reader_type reader(filenames);
				return reader.enumerateOffsets(steps);
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

                        bool skipPattern()
                        {
                                if ( ! foundnextmarker )
                                        return false;
                                        
                                foundnextmarker = false;
                        
                                int c;

                                while ( !newlineterm[c=reader_base_type::getNextCharacter()] )
                                {
                                }
                                
                                if ( c < 0 )
                                        return false;
                                        
                                while ( !scanterm[(c=reader_base_type::getNextCharacter())] )
                                {
                                }

                                foundnextmarker = (c=='>');

                                return true;
                        }

                        bool skipPattern(uint64_t & patlen)
                        {
                                if ( ! foundnextmarker )
                                        return false;
                                        
                                foundnextmarker = false;
                        
                                int c;

                                while ( !newlineterm[c=reader_base_type::getNextCharacter()] )
                                {
                                }
                                
                                if ( c < 0 )
                                        return false;
                                        
                                patlen = 0;
                                        
                                while ( !scanterm[(c=reader_base_type::getNextCharacter())] )
                                {
                                        if ( nospacetable[c] )
                                                patlen += 1;
                                }

                                foundnextmarker = (c=='>');

                                return true;
                        }

                        bool getNextPatternUnlocked(pattern_type & pattern)
                        {
                                if ( ! foundnextmarker )
                                        return false;
				if ( nextid >= interval.high )
					return false;
				if ( reader_base_type::getC() >= (interval.fileoffsethigh-interval.fileoffset) )
					return false;
                                        
                                foundnextmarker = false;
                        
                                int c;

                                idbuffer.reset();
                                while ( !newlineterm[c=reader_base_type::getNextCharacter()] )
                                        idbuffer.bufferPush(c);
                                
                                if ( c < 0 )
                                        return false;
                                        
                                idbuffer.assign ( pattern.sid );

                                patbuffer.reset();
                                while ( !scanterm[(c=reader_base_type::getNextCharacter())] )
                                        if ( nospacetable[c] )
                                                patbuffer.bufferPush(c);

                                patbuffer.assign(pattern.spattern);
                                pattern.pattern = pattern.spattern.c_str();
                                pattern.patlen = pattern.spattern.size();
                                pattern.patid = nextid++;
                                
                                foundnextmarker = (c=='>');

                                return true;
                        }

                        struct RewriteInfo
                        {
                        	bool valid;
                        	uint64_t idlen;
                        	uint64_t seqlen;
                        	std::string sid;
                        	
                        	RewriteInfo() : valid(false) {}
                        	RewriteInfo(bool const rvalid, uint64_t const ridlen = 0, uint64_t const rseqlen = 0, std::string const & rsid = std::string())
                        	: valid(rvalid), idlen(ridlen), seqlen(rseqlen), sid(rsid) {}
                        	
                        	operator bool() const
                        	{
                        		return valid;
                        	}
                        	
                        	uint64_t getEntryLength() const
                        	{
                        		if ( ! valid )
                        			return 0;
					else
						return idlen+seqlen+3; // '>' + 2 newlines
                        	}
                        	
                        	std::string getIdPrefix() const
                        	{
                        		uint64_t i = 0;
                        		while ( i < sid.size() && (!isspace(sid[i])) )
                        			i++;
					return sid.substr(0,i);
                        	}
                        	
                        	template<typename stream_type>
                        	stream_type & serialise(stream_type & stream) const
                        	{
                        		stream.put(valid);
                        		::libmaus2::util::UTF8::encodeUTF8(idlen,stream);
                        		::libmaus2::util::UTF8::encodeUTF8(seqlen,stream);
                        		::libmaus2::util::StringSerialisation::serialiseString(stream,sid);
                        		return stream;
                        	}
                        	
                        	template<typename stream_type>
                        	static RewriteInfo load(stream_type & stream)
                        	{
                        		RewriteInfo info;
                        		info.valid = stream.get();
                        		info.idlen = ::libmaus2::util::UTF8::decodeUTF8(stream);
                        		info.seqlen = ::libmaus2::util::UTF8::decodeUTF8(stream);
                        		info.sid = ::libmaus2::util::StringSerialisation::deserialiseString(stream);
                        		return info;
                        	}
                        };
                        
                        struct RewriteInfoDecoder
                        {
                        	typedef RewriteInfoDecoder this_type;
                        	typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
                        
                        	::libmaus2::aio::InputStreamInstance::unique_ptr_type CIS;
                        	
                        	RewriteInfoDecoder(std::string const & filename)
                        	: CIS(new ::libmaus2::aio::InputStreamInstance(filename))
                        	{
                        	
                        	}
                        	
                        	bool get(RewriteInfo & info)
                        	{
                        		if ( CIS->peek() < 0 )
                        			return false;
					else
					{
						info = RewriteInfo::load(*CIS);
						return true;
					}
                        	}
                        };

                        template<typename buffer_type>
                        RewriteInfo rewritePattern(buffer_type & buffer)
                        {
                                if ( ! foundnextmarker )
                                        return RewriteInfo(false);
				if ( nextid >= interval.high )
					return RewriteInfo(false);
				if ( reader_base_type::getC() >= (interval.fileoffsethigh-interval.fileoffset) )
					return RewriteInfo(false);
                                        
                                foundnextmarker = false;

                                int c;
                                uint64_t idlen = 0;

                                buffer.put('>');
                                idbuffer.reset();
                                while ( !newlineterm[c=reader_base_type::getNextCharacter()] )
                                	buffer.put(c), idbuffer.put(c), ++idlen;
				buffer.put('\n');
				std::string sid;
				idbuffer.assign(sid);
                                
                                if ( c < 0 )
                                        return false;
                                        
				uint64_t seqlen = 0;
                                while ( !scanterm[(c=reader_base_type::getNextCharacter())] )
                                        if ( nospacetable[c] )
                                        	buffer.put(c), ++seqlen;
                                        	
				buffer.put('\n');

                                foundnextmarker = (c=='>');

                                return RewriteInfo(true,idlen,seqlen,sid);
                        }
                        
                        template<typename buffer_type>
                        static std::vector<RewriteInfo> rewriteFilesToBuffer(std::vector<std::string> const & filenames, buffer_type & buffer)
                        {
                        	reader_type reader(filenames);
                        	std::vector < RewriteInfo > V;
                        	RewriteInfo R;
                        	
                        	while ( (R = reader.rewritePattern(buffer)).valid )
                        		V.push_back(R);
                        		
				return V;
			}

                        template<typename buffer_type, typename stream_type>
                        static uint64_t rewriteFilesToBuffer(std::vector<std::string> const & filenames, buffer_type & buffer, stream_type & index)
                        {
                        	reader_type reader(filenames);
                        	RewriteInfo R;
                        	uint64_t c = 0;
                        	
                        	while ( (R = reader.rewritePattern(buffer)).valid )
                        	{
                        		R.serialise(index);
                        		c += 1;
				}
				
				return c;
			}

			static std::vector<RewriteInfo> rewriteFiles(std::vector<std::string> const & filenames, std::string const & outfilename)
                        {
				::libmaus2::aio::SynchronousGenericOutput<char> SGO(outfilename,64*1024);
				std::vector<RewriteInfo> const V = rewriteFilesToBuffer(filenames,SGO);
				SGO.flush();
				return V;
                        }

			static uint64_t rewriteFiles(std::vector<std::string> const & filenames, std::string const & outfilename, std::string const & indexfilename)
                        {
				::libmaus2::aio::SynchronousGenericOutput<char> SGO(outfilename,64*1024);
				::libmaus2::aio::CheckedOutputStream COS(indexfilename);
				uint64_t const c = rewriteFilesToBuffer(filenames,SGO,COS);
				SGO.flush();
				COS.flush();
				return c;
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
                        
                        static int getOffset(std::string const /*inputfile*/)
                        {
                                return -1;
                        }	

                        static PatternCount countPatternsInInterval(std::vector<std::string> const & filenames, uint64_t const apos, uint64_t const bpos)
                        {
                                reader_type reader(filenames,0,16,16*1024,apos,0);

                                uint64_t patlen;
                                uint64_t numpat = 0;
                                uint64_t numsyms = 0;
                                uint64_t minlen = std::numeric_limits<uint64_t>::max();
                                uint64_t maxlen = 0;
                                
                                while ( apos + reader.getC() < bpos && reader.skipPattern(patlen) )
                                {
                                        numpat++;
                                        numsyms += patlen;
                                        maxlen = std::max(maxlen,static_cast<uint64_t>(patlen));
                                        minlen = std::min(minlen,static_cast<uint64_t>(patlen));
                                }

                                
				return PatternCount(numpat,numsyms,minlen,maxlen);      
				// return std::pair<uint64_t,uint64_t>(numpat,numsyms);
                        }

                        static std::string getNameAtPos(std::vector<std::string> const & filenames, uint64_t const pos)
                        {
                        	uint64_t const flen = ::libmaus2::util::GetFileSize::getFileSize(filenames);
				assert ( pos < flen );                        	
                                ::libmaus2::aio::SynchronousFastReaderBase istr(filenames,16,1024,pos);
                                assert ( istr.getNextCharacter() == '>' );
                                std::string name;
                                bool const ok = istr.getLine(name);
                                assert ( ok );                                
                                return name;
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
                                
                                std::string lines[2];
                                uint64_t linestarts[2] = {0,0};
                                bool running = true;
                                
                                while ( running )
                                {
                                        for ( uint64_t i = 1; i < 2; ++i )
                                        {
                                                lines[i-1] = lines[i];
                                                linestarts[i-1] = linestarts[i];
                                        }
                                        
                                        linestarts[1] = istr.getC();
                                        running = running && istr.getLine(lines[1]);
                                        
                                        if ( 
                                                lines[0].size() && lines[1].size()
                                                &&
                                                lines[0][0] == '>'
                                        )
                                        {
                                                #if 0
                                                std::cerr << lines[0] << std::endl;
                                                std::cerr << lines[1] << std::endl;
                                                #endif
                                        
                                                return pos + linestarts[0];
                                        }
                                }
                                
                                return flen;
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



                        static std::vector<FastInterval> parallelIndex(std::vector<std::string> const & filenames, uint64_t const fracs)
                        {
                                uint64_t const flen = ::libmaus2::util::GetFileSize::getFileSize(filenames);
                                uint64_t const fracsize = (flen+fracs-1)/fracs;
                                std::vector < uint64_t > fracstarts(fracs+1);
                                fracstarts.back() = flen;
                                
                                #if defined(_OPENMP)
                                #pragma omp parallel for schedule(dynamic,1)
                                #endif
                                for ( int64_t i = 0; i < static_cast<int64_t>(fracs); ++i )
                                        fracstarts[i] = searchNextStart(filenames,i*fracsize);
                                
                                std::vector< PatternCount > intlen(fracs);
                                
                                #if defined(_OPENMP)
                                #pragma omp parallel for schedule(dynamic,1)
                                #endif
                                for ( int64_t i = 0; i < static_cast<int64_t>(fracs); ++i )
                                        intlen[i] = countPatternsInInterval(filenames,fracstarts.at(i),fracstarts.at(i+1));

                                uint64_t ilow = 0;
                                std::vector < ::libmaus2::fastx::FastInterval > FIV;
                                for ( uint64_t i = 0; i < fracs; ++i )
                                {
                                        uint64_t const ihigh = ilow+intlen[i].numpat;
                                        if ( ihigh != ilow )
	                                        FIV.push_back( ::libmaus2::fastx::FastInterval(ilow,ihigh,fracstarts[i],fracstarts[i+1],intlen[i].numsyms,intlen[i].minlen,intlen[i].maxlen));
                                        ilow = ihigh;			
                                }
                                
                                return FIV;
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

			static ::libmaus2::util::Histogram::unique_ptr_type getHistogram(std::vector<std::string> const & filenames, std::vector<FastInterval> const & rinterval)
			{
			        ::libmaus2::util::Histogram::unique_ptr_type Phist(new ::libmaus2::util::Histogram());
			        ::libmaus2::util::Histogram & hist = *Phist;
			        ::libmaus2::parallel::OMPLock lock;
                                
                                #if defined(_OPENMP)
                                #pragma omp parallel for schedule(dynamic,1)
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

                typedef FastAReaderTemplate < ::libmaus2::aio::SynchronousFastReaderBase > FastAReader;
	}
}

#endif
