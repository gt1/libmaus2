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

#if ! defined(IMPEXTERNALWAVELETGENERATOR_HPP)
#define IMPEXTERNALWAVELETGENERATOR_HPP

#include <libmaus/util/TempFileNameGenerator.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/rank/ImpCacheLineRank.hpp>

namespace libmaus
{
	namespace wavelet
	{
		struct ImpExternalWaveletGenerator
		{
			typedef ImpExternalWaveletGenerator this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::rank::ImpCacheLineRank rank_type;
			typedef rank_type::WriteContextExternal context_type;
			typedef context_type::unique_ptr_type context_ptr_type;
			typedef ::libmaus::autoarray::AutoArray<context_ptr_type> context_vector_type;
		
			private:
			static uint64_t const bufsize = 64*1024;
		
			uint64_t const b;
			::libmaus::util::TempFileNameGenerator & tmpgen;
			
			std::vector < std::vector<std::string> > outputfilenames;
			::libmaus::autoarray::AutoArray<context_vector_type> contexts;
			
			uint64_t symbols;

			void flush()
			{
				for ( uint64_t i = 0; i < contexts.size(); ++i )
					for ( uint64_t j = 0; j < contexts[i].size(); ++j )
					{
						contexts[i][j]->writeBit(0);
						contexts[i][j]->flush();
					}
			}
			
			public:
			ImpExternalWaveletGenerator(uint64_t const rb, ::libmaus::util::TempFileNameGenerator & rtmpgen)
			: b(rb), tmpgen(rtmpgen), outputfilenames(b), contexts(b), symbols(0)
			{
				for ( uint64_t ib = 0; ib < b; ++ib )
				{
					contexts[ib] = context_vector_type( 1ull << ib );	
					for ( uint64_t i = 0; i < (1ull<<ib); ++i )
					{
						outputfilenames[ib].push_back(tmpgen.getFileName());
						contexts[ib][i] = UNIQUE_PTR_MOVE(context_ptr_type(new context_type(outputfilenames[ib].back(), 0, false /* no header */)));
					}
				}
			}
			

			void putSymbol(uint64_t const s)
			{
				for ( uint64_t i = 0; i < b; ++i )
				{
					uint64_t const prefix = s>>(b-i);
					uint64_t const bit = (s>>(b-i-1))&1;
					#if 0
					std::cerr << "Symbol " << s
						<< " i=" << i
						<< " prefix=" << prefix
						<< " bit=" << bit << std::endl;
					#endif
					contexts[i][prefix]->writeBit(bit);
				}
				
				symbols += 1;
			}

			void createFinalStream(std::ostream & out)
			{			
				flush();

				::libmaus::util::NumberSerialisation::serialiseNumber(out,symbols); // n
				::libmaus::util::NumberSerialisation::serialiseNumber(out,b); // b
				
				for ( uint64_t i = 0; i < contexts.size(); ++i )
					for ( uint64_t j = 0; j < contexts[i].size(); ++j )
					{
						uint64_t const blockswritten = contexts[i][j]->blockswritten;
						uint64_t const datawordswritten = 6*blockswritten;
						uint64_t const allwordswritten = 8*blockswritten;
						
						contexts[i][j].reset();
						// bits written
						::libmaus::serialize::Serialize<uint64_t>::serialize(out,64*datawordswritten);
						// auto array header
						::libmaus::serialize::Serialize<uint64_t>::serialize(out,allwordswritten);
						std::string const filename = outputfilenames[i][j];
						std::ifstream istr(filename.c_str(),std::ios::binary);
						::libmaus::util::GetFileSize::copy (istr, out, allwordswritten, sizeof(uint64_t));
						
						remove(filename.c_str());
					}
				
				out.flush();
			}
			
			void createFinalStream(std::string const & filename)
			{
				std::ofstream ostr(filename.c_str(),std::ios::binary);
				createFinalStream(ostr);
				ostr.flush();
				ostr.close();
			}
		};
	}
}
#endif
