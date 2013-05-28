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

#if ! defined(FASTQREADERSPLIT_HPP)
#define FASTQREADERSPLIT_HPP

#include <libmaus/fastx/FastQReader.hpp>

namespace libmaus
{
	namespace fastx
	{
                struct FastQReaderSplit : public ::libmaus::fastx::FastQReader
                {
                        typedef FastQReaderSplit reader_type;
                        typedef FASTQEntry pattern_type;
                        typedef PatternBlock<pattern_type> block_type;
                        
                        typedef reader_type this_type;
                        typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

                        #if defined(FASTXASYNC)
                        typedef ::libmaus::aio::AsynchronousStreamReaderData<reader_type> stream_data_type;
                        typedef ::libmaus::aio::AsynchronousStreamReader< ::libmaus::aio::AsynchronousStreamReaderData<reader_type> > stream_reader_type;
                        #endif

                        pattern_type patbuf[2];
                        unsigned int patbuffill;
                        
                        FastQReaderSplit(
				std::string const & filename, 
				int rqualityOffset = 0, 
				unsigned int numbuffers = 16, 
				unsigned int bufsize = 16*1024,
				uint64_t const fileoffset = 0,
				uint64_t const rnextid = 0)
                        : FastQReader(filename,rqualityOffset,numbuffers,bufsize,fileoffset,rnextid), patbuffill(0)
                        {
                        }

                        FastQReaderSplit(
				std::vector<std::string> const & filenames, 
				int rqualityOffset = 0, 
				unsigned int numbuffers = 16, 
				unsigned int bufsize = 16*1024,
				uint64_t const fileoffset = 0,
				uint64_t const rnextid = 0)
                        : FastQReader(filenames,rqualityOffset,numbuffers,bufsize,fileoffset,rnextid), patbuffill(0)
                        {
                        }


			FastQReaderSplit(
				std::string const & filename,
				FastInterval const & rinterval,
				int rqualityOffset = 0,
				unsigned int const numbuffers = 16,
				unsigned int const bufsize = 16*1024)
			: FastQReader(filename,rinterval.unsplit(),rqualityOffset,numbuffers,bufsize), patbuffill(0)
                        {
                        }

			FastQReaderSplit(
				std::vector<std::string> const & filenames,
				FastInterval const & rinterval,
				int rqualityOffset = 0,
				unsigned int const numbuffers = 16,
				unsigned int const bufsize = 16*1024)
			: FastQReader(filenames,rinterval.unsplit(),rqualityOffset,numbuffers,bufsize), patbuffill(0)
                        {
                        }

                        
                        uint64_t countPatterns()
                        {
                                pattern_type pattern;
                                uint64_t count = 0;
                                
                                while ( skipPattern(pattern) )
                                        count += 2;

                                return count;
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
						uint64_t const o = ::libmaus::aio::SynchronousFastReaderBase::getC() - 1;
						uint64_t const scount = count;
						uint64_t symcount = 0;
						uint64_t minlen = std::numeric_limits<uint64_t>::max();
						uint64_t maxlen = 0;

						for ( uint64_t i = 0; i < steps && foundnextmarker; ++i )
						{
							skipPattern(pattern);
							count += 2;
							
							if ( pattern.patlen > 2 )
							{
							        symcount += pattern.patlen-2;
							        minlen = std::min(minlen,(symcount-1)/2);
							        maxlen = std::max(maxlen,(symcount-1)/2);
							}
						}

						std::cerr << "[" << scount << "," << count << ") at " << o << std::endl;

                                                uint64_t const endo = 
                                                        foundnextmarker ? 
                                                                (::libmaus::aio::SynchronousFastReaderBase::getC() - 1) 
                                                                : 
                                                                (::libmaus::aio::SynchronousFastReaderBase::getC());
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
                                ostr << filename << "_" << steps << ".split.idx";
                                return ostr.str();
                        }

                        static std::vector<FastInterval> buildIndex(std::string const & filename, uint64_t const steps = 1)
                        {
				std::string const indexfilename = getIndexFileName(filename,steps);
				
				if ( ::libmaus::util::GetFileSize::fileExists ( indexfilename ) )
				{
				        std::ifstream istr(indexfilename.c_str(), std::ios::binary);
				        std::vector < FastInterval > intervals = ::libmaus::fastx::FastInterval::deserialiseVector(istr);
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

                                #if defined(_OPENMP)
                                #pragma omp parallel for schedule(dynamic,1)
                                #endif
                                for ( int64_t i = 0; i < static_cast<int64_t>(filenames.size()); ++i )
                                        buildIndex(filenames[i], steps);
                        
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

                        bool getNextPatternUnlocked(pattern_type & pattern)
                        {
                                if ( patbuffill == 0 )
                                {
                                        pattern_type rawpat;
                                        bool const ok = FastQReader::getNextPatternUnlocked(rawpat);
                                        
                                        if ( ! ok )
                                                return false;

                                        patbuf[1].sid = rawpat.sid + "/1";
                                        patbuf[1].spattern = rawpat.spattern.substr(0, rawpat.spattern.size()/2 );        
                                        patbuf[1].patid = 2*rawpat.patid;
                                        patbuf[1].plus = rawpat.plus;
                                        patbuf[1].quality = rawpat.quality.substr(0, rawpat.spattern.size()/2 );

                                        if ( patbuf[1].spattern.size() )
                                                patbuf[1].spattern = patbuf[1].spattern.substr(0, patbuf[1].spattern.size()-1 );
                                        if ( patbuf[1].quality.size() )
                                                patbuf[1].quality = patbuf[1].quality.substr(0, patbuf[1].quality.size()-1 );

                                        patbuf[1].pattern = patbuf[1].spattern.c_str();
                                        patbuf[1].patlen = patbuf[1].spattern.size();
                                        
                                        patbuf[0].sid = rawpat.sid + "/2";
                                        patbuf[0].spattern = rawpat.spattern.substr(rawpat.spattern.size()/2 );
                                        patbuf[0].patid = 2*rawpat.patid + 1;
                                        patbuf[0].plus = rawpat.plus;
                                        patbuf[0].quality = rawpat.quality.substr(rawpat.quality.size()/2 );

                                        if ( patbuf[0].spattern.size() )
                                                patbuf[0].spattern = patbuf[0].spattern.substr(1);
                                        if ( patbuf[0].quality.size() )
                                                patbuf[0].quality = patbuf[0].quality.substr(1);

                                        patbuf[0].pattern = patbuf[0].spattern.c_str();
                                        patbuf[0].patlen = patbuf[0].spattern.size();
   
                                        patbuffill = 2;
                                }
                                
                                assert ( patbuffill );
                                
                                pattern = patbuf [ --patbuffill ] ;
                                
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

                        static int getOffset(std::string const inputfile)
                        {
                                FastQReader file ( inputfile );
                                FASTQEntry entry;
                                while ( file.getNextPatternUnlocked(entry) )
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
                };
	}
}
#endif
