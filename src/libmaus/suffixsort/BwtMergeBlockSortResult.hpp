/**
    suds
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
#if ! defined(LIBMAUS_SUFFIXSORT_BWTMERGEBLOCKSORTRESULT_HPP)
#define LIBMAUS_SUFFIXSORT_BWTMERGEBLOCKSORTRESULT_HPP

#include <libmaus/suffixsort/BwtMergeTempFileNameSet.hpp>
#include <libmaus/suffixsort/BwtMergeZBlock.hpp>

namespace libmaus
{
	namespace suffixsort
	{
		struct BwtMergeBlockSortResult
		{
			uint64_t blockp0rank;
			std::vector < ::libmaus::suffixsort::BwtMergeZBlock > zblocks;
			
			uint64_t blockstart;
			uint64_t cblocksize;
			
			::libmaus::suffixsort::BwtMergeTempFileNameSet files;

			BwtMergeBlockSortResult()
			: blockp0rank(0), zblocks(), blockstart(0), cblocksize(0), files()
			{
			
			}
			
			static std::vector < ::libmaus::suffixsort::BwtMergeZBlock > mergeZBlockVectors(std::vector<BwtMergeBlockSortResult> const & V)
			{
				std::vector < std::vector< ::libmaus::suffixsort::BwtMergeZBlock >::const_iterator > VIT;
				bool running = V.size() != 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					VIT.push_back(V[i].zblocks.begin());
					running = running && (VIT.back() != V[i].zblocks.end() );
				}
					
				std::vector < ::libmaus::suffixsort::BwtMergeZBlock > R;
				
				while ( running )
				{
					uint64_t acc = 0;
					for ( uint64_t i = 0; i < VIT.size(); ++i )
					{
						assert ( VIT[i] != V[i].zblocks.end() );
						assert ( VIT[i]->zabspos == VIT[0]->zabspos );
						acc += VIT[i]->zrank;
					}
					
					uint64_t const zabspos = VIT[0]->zabspos;
					
					R.push_back(::libmaus::suffixsort::BwtMergeZBlock(zabspos,acc));

					for ( uint64_t i = 0; i < VIT.size(); ++i )
						if ( ++VIT[i] == V[i].zblocks.end() )
							running = false;
				}
						
				return R;
			}
			
			BwtMergeBlockSortResult(std::istream & stream)
			{
				blockp0rank = ::libmaus::util::NumberSerialisation::deserialiseNumber(stream);
				
				zblocks.resize(::libmaus::util::NumberSerialisation::deserialiseNumber(stream));
				for ( uint64_t i = 0; i < zblocks.size(); ++i )
					zblocks[i] = ::libmaus::suffixsort::BwtMergeZBlock(stream);

				blockstart = ::libmaus::util::NumberSerialisation::deserialiseNumber(stream);
				cblocksize = ::libmaus::util::NumberSerialisation::deserialiseNumber(stream);
				
				files = ::libmaus::suffixsort::BwtMergeTempFileNameSet(stream);
			}
			
			static BwtMergeBlockSortResult load(std::string const & s)
			{
				std::istringstream istr(s);
				return BwtMergeBlockSortResult(istr);
			}
			
			template<typename stream_type>
			void serialise(stream_type & stream) const
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,blockp0rank);
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,zblocks.size());
				for ( uint64_t i = 0; i < zblocks.size(); ++i )
					zblocks[i].serialise(stream);

				::libmaus::util::NumberSerialisation::serialiseNumber(stream,blockstart);
				::libmaus::util::NumberSerialisation::serialiseNumber(stream,cblocksize);
				
				files.serialise(stream);
			}
			std::string serialise() const
			{
				std::ostringstream ostr;
				serialise(ostr);
				return ostr.str();
			}
		};
	}
}
#endif

