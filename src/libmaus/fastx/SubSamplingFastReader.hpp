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

#if ! defined(SUBSAMPLINGFASTREADER_HPP)
#define SUBSAMPLINGFASTREADER_HPP

#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/fastx/FastInterval.hpp>
#include <libmaus/fastx/FastAReader.hpp>
#include <libmaus/fastx/FastQReader.hpp>
#include <libmaus/fastx/CompactFastDecoder.hpp>
#include <libmaus/random/Random.hpp>

#include <ctime>
#include <cstdlib>

namespace libmaus
{
	namespace fastx
	{
		template<typename _reader_type, bool _keep_pairs>
		struct SubSamplingFastReader : public ::libmaus::random::Random
		{
			typedef _reader_type reader_type;
			typedef typename reader_type::unique_ptr_type reader_ptr_type;
			
			static bool const keep_pairs = _keep_pairs;
			
			typedef SubSamplingFastReader<reader_type,keep_pairs> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			typedef typename reader_type::pattern_type pattern_type;
			
			reader_ptr_type Preader;
			
			uint64_t const c;
			uint64_t const d;
			
			static uint64_t const maxd = 64*1024;
			
			bool pairflag;
			
			void checkParameters() const
			{
				if ( d > maxd )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Give denominator " << d << " greater than allowed maximum " << maxd << " in SubSamplingFastReader<>::checkParameters()" << std::endl;
					se.finish();
					throw se;
				}
				if ( ! d )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Give denominator " << d << " is " << maxd << " in SubSamplingFastReader<>::checkParameters()" << std::endl;
					se.finish();
					throw se;
				}
				if ( c > d )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "Give counter " << c << " is larger than denomintator " << d << " in SubSamplingFastReader<>::checkParameters()" << std::endl;
					se.finish();
					throw se;
				}
			}
			
			SubSamplingFastReader(std::string const & filename, uint64_t const rc, uint64_t const rd) : Preader(new reader_type(std::vector<std::string>(1,filename))), c(rc), d(rd), pairflag(false)
			{
				checkParameters();
			}
			SubSamplingFastReader(std::vector<std::string> const & filenames, uint64_t const rc, uint64_t const rd) : Preader(new reader_type(filenames)), c(rc), d(rd), pairflag(false)
			{
				checkParameters();			
			}
			SubSamplingFastReader(std::string const & filename, ::libmaus::fastx::FastInterval const & FI, uint64_t const rc, uint64_t const rd) : Preader(new reader_type(std::vector<std::string>(1,filename),FI)), c(rc), d(rd), pairflag(false)
			{
				checkParameters();			
			}
			SubSamplingFastReader(std::vector<std::string> const & filenames, ::libmaus::fastx::FastInterval const & FI, uint64_t const rc, uint64_t const rd) : Preader(new reader_type(filenames,FI)), c(rc), d(rd), pairflag(false)
			{
				checkParameters();			
			}

                        bool getNextPatternUnlocked(pattern_type & pattern)
                        {
                        	if ( keep_pairs && pairflag )
                        	{
                        		pairflag = false;
                        		return Preader->getNextPatternUnlocked(pattern);
                        	}
                        	else
                        	{
                        		while ( true )
                        		{
                        			uint64_t const z = rand16() % d;
                        			
                        			// RNG keep
                        			if ( z < c )
                        			{
                        				bool const ok = Preader->getNextPatternUnlocked(pattern);
                        				
                        				if ( ok )
                        				{
                        					if ( keep_pairs )
                        						pairflag = true;
								return true;
                        				}
                        				else
                        				{
                        					// no more reads
                        					return false;
                        				}
                        			}
                        			// RNG drop
                        			else
                        			{
                        				uint64_t const skipcnt = keep_pairs ? 2 : 1;
                        				
                        				for ( uint64_t i = 0; i < skipcnt; ++i )
							{
	                        				bool const ok = Preader->getNextPatternUnlocked(pattern);
								if ( ! ok )
									return false;
							}
                        			}
                        		}
                        	}
                        }
		};
		
		typedef SubSamplingFastReader< ::libmaus::fastx::FastAReader, false > SubSamplingSingleFastAReader;
		typedef SubSamplingFastReader< ::libmaus::fastx::FastAReader, true > SubSamplingPairedFastAReader;
		typedef SubSamplingFastReader< ::libmaus::fastx::FastQReader, false > SubSamplingSingleFastQReader;
		typedef SubSamplingFastReader< ::libmaus::fastx::FastQReader, true > SubSamplingPairedFastQReader;
		typedef SubSamplingFastReader< ::libmaus::fastx::CompactFastConcatDecoder, false > SubSamplingSingleCompactFastReader;
		typedef SubSamplingFastReader< ::libmaus::fastx::CompactFastConcatDecoder, true > SubSamplingPairedCompactFastReader;
	}
}
#endif
