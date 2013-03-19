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


#if ! defined(MULTIFILECONCATWRITER_HPP)
#define MULTIFILECONCATWRITER_HPP

#include <libmaus/util/IntervalTree.hpp>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/TempFileNameGenerator.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <fstream>
#include <iomanip>

namespace libmaus
{
	namespace aio
	{
		struct MultiFileConcatWriter
		{
			typedef ::libmaus::util::IntervalTree it_type;
			typedef it_type::unique_ptr_type it_ptr_type;
			typedef std::pair<uint64_t,uint64_t> upair;
			
			::libmaus::autoarray::AutoArray<upair> const intervals;
			uint64_t const numfiles;
			it_ptr_type it;	
			::std::streampos pptr;
			std::vector < std::string > filenames;
			::std::streampos const endptr;
			::libmaus::parallel::OMPLock lock;
			
			static ::libmaus::autoarray::AutoArray<upair> scaleClone(
				::libmaus::autoarray::AutoArray<upair> const & rintervals,
				uint64_t const factor)
			{
				::libmaus::autoarray::AutoArray<upair> scaled = rintervals.clone();
				for ( uint64_t i = 0; i < scaled.getN(); ++i )
				{
					scaled[i].first *= factor;
					scaled[i].second *= factor;
				}
				return scaled;
			}
			
			MultiFileConcatWriter(
				::libmaus::util::TempFileNameGenerator & tmpgen, 
				::libmaus::autoarray::AutoArray<upair> const & rintervals,
				uint64_t const scalefactor
			)
			: intervals(scaleClone(rintervals,scalefactor)), numfiles(intervals.getN()), it (new it_type(intervals,0,intervals.getN())), pptr(0),
			  endptr ( intervals.getN() ? intervals[intervals.getN()-1].second : 0 )
			{
				for ( uint64_t i = 0; i < numfiles; ++i )
				{
					filenames.push_back(tmpgen.getFileName());
					std::ofstream ostr(filenames.back().c_str(),std::ios::binary);
					ostr.flush();
					ostr.close();
				}
			}

			MultiFileConcatWriter(
				::libmaus::util::TempFileNameGenerator & tmpgen, 
				::libmaus::autoarray::AutoArray<upair> const & rintervals
			)
			: intervals(rintervals.clone()), numfiles(intervals.getN()), it (new it_type(intervals,0,intervals.getN())), pptr(0),
			  endptr ( intervals.getN() ? intervals[intervals.getN()-1].second : 0 )
			{
				for ( uint64_t i = 0; i < numfiles; ++i )
				{
					filenames.push_back(tmpgen.getFileName());
					std::ofstream ostr(filenames.back().c_str(),std::ios::binary);
					ostr.flush();
					ostr.close();
				}
			}
			
			void write(char const * data, ::std::streamsize n, ::std::streampos pptr) const
			{
				assert ( pptr + static_cast< ::std::streampos > (n) <= endptr );
				uint64_t i = it->find(pptr);
				
				while ( n )
				{
					::std::streampos  const fileoff = pptr-static_cast< ::std::streampos>(intervals[i].first);
					::std::streamsize const filefit = intervals[i].second-pptr;
					::std::streamsize const towrite = std::min(filefit,n);

					std::fstream out(filenames[i].c_str(), std::ios::in|std::ios::out|std::ios::binary|std::ios::ate);
					out.seekp(fileoff,std::ios::beg);
					out.write(data,towrite);
					out.flush();
					assert ( out );
					out.close();
					
					data += towrite;
					pptr += towrite;
					n -= towrite;
					i++;
				}
			}

			void write(char const * data, ::std::streamsize n)
			{
				lock.lock();
				
				assert ( pptr + static_cast< ::std::streampos>(n) <= endptr );
				uint64_t i = it->find(pptr);
				
				while ( n )
				{
					::std::streampos  const fileoff = pptr-static_cast< ::std::streampos>(intervals[i].first);
					::std::streamsize const filefit = intervals[i].second-pptr;
					::std::streamsize const towrite = std::min(filefit,n);

					std::fstream out(filenames[i].c_str(), std::ios::in|std::ios::out|std::ios::binary|std::ios::ate);
					out.seekp(fileoff,std::ios::beg);
					out.write(data,towrite);
					out.flush();
					assert ( out );
					out.close();
					
					data += towrite;
					pptr += towrite;
					n -= towrite;
					i++;
				}
				
				lock.unlock();
			}
			
			void seekp(::std::streampos shift)
			{
				lock.lock();
				pptr = shift;
				lock.unlock();
			}
			
			void seekp(::std::streamoff shift, std::ios_base::seekdir dir)
			{
				lock.lock();
				switch ( dir  )
				{
					case std::ios_base::cur:
						assert ( static_cast< ::std::streamoff > ( pptr ) + shift >= 0 );
						pptr = static_cast< ::std::streampos>(static_cast< ::std::streamoff > ( pptr ) + shift);
						break;
					case std::ios_base::beg:
						assert ( shift >= 0 );
						pptr = static_cast< ::std::streampos>(shift);
						break;
					case std::ios_base::end:
						assert ( static_cast< ::std::streamoff > ( endptr ) + shift >= 0 );
						pptr = static_cast< ::std::streampos>(static_cast< ::std::streamoff > ( pptr ) + shift);
						break;
					default:
						assert ( false );
						break;	
				}
				lock.unlock();
			}
			
			void distributeSingle(std::string const freqfilename, uint64_t const bufsize = 128*1024*1024)
			{
				uint64_t freqpos = 0;
				uint64_t freqlen = ::libmaus::util::GetFileSize::getFileSize(freqfilename);
				std::ifstream freqistr(freqfilename.c_str(),std::ios::binary);
				::libmaus::autoarray::AutoArray<char> buf(bufsize,false);
				while ( freqlen )
				{
					uint64_t const toread = std::min(freqlen,buf.getN());
					freqistr.read(buf.get(),toread);
					assert ( freqistr.gcount() == static_cast<int64_t>(toread) );
					write(buf.get(), toread, freqpos);
					freqpos += toread;
					freqlen -= toread;
				}
			
			}

			void moveFiles(std::string const & pref)
			{
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					::std::ostringstream freqfnstr;
					freqfnstr << pref << std::setw(3) << std::setfill('0') << i;
					std::string const freqfn = freqfnstr.str();
					rename ( filenames[i].c_str(), freqfn.c_str() );
				}
			}

			static uint64_t distributeSingleAndMove(
				::libmaus::util::TempFileNameGenerator & tmpgen,
				::libmaus::autoarray::AutoArray<upair> const & freqintervals,
				std::string const & freqfilename,
				std::string const & freqfnpref
				)
			{
				MultiFileConcatWriter freqwriter(tmpgen,freqintervals);
				freqwriter.distributeSingle(freqfilename);
				freqwriter.moveFiles(freqfnpref);	
				return freqintervals.getN();
			}

			static uint64_t distributeSingleAndMove(
				::libmaus::util::TempFileNameGenerator & tmpgen,
				::libmaus::autoarray::AutoArray<upair> const & freqintervals,
				std::string const & freqfilename,
				std::string const & freqfnpref,
				uint64_t const scalefactor
				)
			{
				MultiFileConcatWriter freqwriter(tmpgen,freqintervals,scalefactor);
				freqwriter.distributeSingle(freqfilename);
				freqwriter.moveFiles(freqfnpref);	
				return freqintervals.getN();
			}
		};
	}
}
#endif
