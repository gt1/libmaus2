/*
    libmaus2
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
#if ! defined(LIBMAUS_BAMBAM_GENEFLATFILE_HPP)
#define LIBMAUS_BAMBAM_GENEFLATFILE_HPP

#include <cassert>
#include <libmaus2/aio/CheckedInputStream.hpp>
#include <libmaus2/bambam/GeneFlatFileEntry.hpp>
#include <libmaus2/lz/BufferedGzipStream.hpp>
#include <libmaus2/lz/IsGzip.hpp>
#include <libmaus2/util/LineAccessor.hpp>

namespace libmaus2
{
	namespace bambam
	{
		/**
		 * flat file class for data files following the format of
		 * http://hgdownload.cse.ucsc.edu/goldenPath/hg19/database/refFlat.txt.gz
		 **/
		struct GeneFlatFile
		{
			typedef GeneFlatFile this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			libmaus2::autoarray::AutoArray<char> C;
			libmaus2::util::LineAccessor::unique_ptr_type LA;
			uint64_t nl;
			
			GeneFlatFile() : nl(0)
			{
			
			}
		
			static libmaus2::autoarray::AutoArray<char> loadFile(std::istream & in)
			{
				libmaus2::autoarray::AutoArray<char> C(1);
				uint64_t p = 0;
				
				while ( in )
				{
					in.read(C.begin() + p, C.size()-p);
					
					if ( ! in.gcount() )
						break;

					p += in.gcount();
					
					if ( p == C.size() )
					{
						libmaus2::autoarray::AutoArray<char> Cn(2*C.size(),false);
						std::copy(C.begin(),C.end(),Cn.begin());
						C = Cn;
					}
				}
				
				libmaus2::autoarray::AutoArray<char> Cn(p,false);
				std::copy(C.begin(),C.begin()+p,Cn.begin());
				
				return Cn;
			}

			public:
			void get(uint64_t const i, libmaus2::bambam::GeneFlatFileEntry & entry) const
			{
				if ( i >= nl )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "GeneFlatFile::get(): line " << i << " is out of range." << std::endl;
					lme.finish();
					throw lme;
				}
				
				std::pair<uint64_t,uint64_t> P = LA->lineInterval(i);
				while ( P.second != P.first && isspace(C[P.second-1]) )
					--P.second;
					
				entry.reset(C.begin() + P.first,C.begin() + P.second);				
			}
			
			uint64_t size() const
			{
				return nl;
			}
			
			libmaus2::bambam::GeneFlatFileEntry operator[](uint64_t const i) const
			{
				libmaus2::bambam::GeneFlatFileEntry entry;
				get(i,entry);
				return entry;
			}
					
			static unique_ptr_type construct(std::string const & fn)
			{
				libmaus2::autoarray::AutoArray<char> C;
				
				libmaus2::aio::CheckedInputStream PFIS(fn);
				
				if ( libmaus2::lz::IsGzip::isGzip(PFIS) )
				{
					libmaus2::lz::BufferedGzipStream gzstr(PFIS);
					C = loadFile(gzstr);
				}
				else
				{
					C = loadFile(PFIS);
				}
								
				unique_ptr_type ptr(new this_type);
				
				ptr->C = C;
				libmaus2::util::LineAccessor::unique_ptr_type TLA(new libmaus2::util::LineAccessor(ptr->C.begin(),ptr->C.end()));
				ptr->LA = UNIQUE_PTR_MOVE(TLA);
				
				uint64_t nl = ptr->LA->size();
				libmaus2::bambam::GeneFlatFileEntry entry;
				
				while ( nl )
				{
					std::pair<uint64_t,uint64_t> P = ptr->LA->lineInterval(nl-1);
					while ( P.second != P.first && isspace(ptr->C[P.second-1]) )
						--P.second;
						
					try
					{
						entry.reset(
							ptr->C.begin() + P.first,
							ptr->C.begin() + P.second
						);
						break;
					}
					catch(std::exception const &ex)
					{
						std::cerr << ex.what();
						--nl;
					}
				}
				ptr->nl = nl;
								
				return UNIQUE_PTR_MOVE(ptr);
			}	
		};
		
		std::ostream & operator<<(std::ostream & out, GeneFlatFile const & GFL);
	}
}
#endif
