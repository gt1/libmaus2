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

#if ! defined(TEMPFILENAMEGENERATOR_HPP)
#define TEMPFILENAMEGENERATOR_HPP

#include <libmaus/parallel/OMPLock.hpp>
#include <libmaus/exception/LibMausException.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <vector>
#include <string>
#include <cassert>
#include <iomanip>
#include <algorithm>

/* for mkdir */
#include <sys/stat.h>
#include <sys/types.h>

#if defined(LIBMAUS_HAVE_UNISTD_H)
#include <unistd.h>
#endif
              
namespace libmaus
{
	namespace util
	{
		template<uint64_t b, uint64_t e>
		struct IPower
		{
			static uint64_t const n = b * IPower<b,e-1>::n;
		};
		
		template<uint64_t b>
		struct IPower<b,0>
		{
			static uint64_t const n = 1;
		};
	
		template<uint64_t num, uint64_t b>
		struct LogFloor
		{
			static int const log = 1 + LogFloor<num/b,b>::log;
		};
		
		template<uint64_t b>
		struct LogFloor<0,b>
		{
			static int const log = -1;
		};
		
		template<uint64_t num, uint64_t b>
		struct LogCeil
		{
			static int const log =
				(IPower< b,LogFloor<num,b>::log >::n == num) ? (LogFloor<num,b>::log) : (LogFloor<num,b>::log+1);
		};
	
		struct TempFileNameGeneratorState
		{
			static int const dirmod = 64;
			static int const filemod = 64;
			static int const maxmod = (dirmod>filemod)?dirmod:filemod;
			static int const digits = LogCeil<maxmod,10>::log;
			
			unsigned int depth;			
			std::vector < int > nextdir;
			int64_t nextfile;
			std::string const prefix;
			
			bool operator==(TempFileNameGeneratorState const & o) const
			{
				return
					depth == o.depth 
					&&
					nextdir == o.nextdir
					&&
					nextfile == o.nextfile;
			}
			
			bool operator!=(TempFileNameGeneratorState const & o) const
			{
				return !((*this) == o);
			}
			
			TempFileNameGeneratorState(unsigned int const rdepth, std::string const & rprefix)
			: depth(rdepth), nextfile(-1), prefix(rprefix)
			{
				assert ( depth );
				setup();
			}
			
			void setup()
			{
				nextdir = std::vector<int>(depth);
				std::fill ( nextdir.begin(), nextdir.end(), 0 );
				nextdir.back() = -1;
			}
			

			void next()
			{
				if ( (++nextfile) % filemod == 0 )
				{
					unsigned int idx = nextdir.size()-1;
					
					while ( (++ nextdir[idx]) == dirmod )
					{
						nextdir[idx] = 0;
						
						if ( ! idx )
						{
							--nextfile;
							depth += 1;
							setup();
							next();
						}
						
						--idx;
					}
				}
			}
			
			static std::string numToString(uint64_t const num, unsigned int dig)
			{
				std::ostringstream ostr;
				ostr << std::setw(dig) << std::setfill('0') << num;
				return ostr.str();
			}
			
			std::string getFileName()
			{
				next();
			
				std::vector < std::string > particles;
				
				for ( uint64_t i = 0; i < nextdir.size(); ++i )
					particles.push_back( numToString(nextdir[i],digits) );
				
				std::string dirname = prefix;
				#if defined(_WIN32)
				mkdir ( dirname.c_str() );
				#else
				mkdir ( dirname.c_str(), 0700 );
				#endif
				
				for ( uint64_t i = 0; i < particles.size(); ++i )
				{
					dirname += "/";
					dirname += particles[i];
					#if defined(_WIN32)
					mkdir ( dirname.c_str() );
					#else
					mkdir ( dirname.c_str(), 0700 );
					#endif
				}
				
				std::ostringstream fnostr;
				fnostr << dirname << "/" << "file" << numToString(nextfile,digits);
				std::string const fn = fnostr.str();
				
				return fn;
			}

			void removeDirs()
			{
				next();
			
				std::vector < std::string > particles;
				
				for ( uint64_t i = 0; i < nextdir.size(); ++i )
					particles.push_back(numToString(nextdir[i],digits));
				
				std::vector < std::string > dirs;
				
				std::string dirname = prefix;
				dirs.push_back(dirname);
				
				for ( uint64_t i = 0; i < particles.size(); ++i )
				{
					dirname += "/";
					dirname += particles[i];
					dirs.push_back(dirname);
				}
				
				std::reverse(dirs.begin(),dirs.end());
				for ( 
					::std::vector<std::string>::const_iterator ita = dirs.begin();
					ita != dirs.end();
					++ita )
				{
					// std::cerr << "Removing dir " << *ita << std::endl;
					rmdir ( ita->c_str() );								
				}
				std::reverse(dirs.begin(),dirs.end());
			}
		};
	
		struct TempFileNameGenerator
		{
			typedef TempFileNameGenerator this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			::libmaus::parallel::OMPLock lock;
			
			TempFileNameGeneratorState state;
			TempFileNameGeneratorState const startstate;
			
			TempFileNameGenerator(std::string const rprefix, unsigned int const rdepth)
			: state(rdepth,rprefix), startstate(state)
			{
			}
			~TempFileNameGenerator()
			{
				cleanupDirs();
			}
			
			std::string getFileName()
			{
				lock.lock();
				
				std::string const fn = state.getFileName();

				lock.unlock();
				
				return fn;
			}
			
			void cleanupDirs()
			{
				TempFileNameGeneratorState rmdirstate = startstate;
				
				while ( rmdirstate != state )
				{
					rmdirstate.removeDirs();
				}
			}
		};
	}
}
#endif
