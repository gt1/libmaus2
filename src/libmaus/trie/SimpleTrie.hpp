/*
    libmaus
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
#if ! defined(LIBMAUS_TRIE_SIMPLETRIE_HPP)
#define LIBMAUS_TRIE_SIMPLETRIE_HPP

#include <libmaus/types/types.hpp>
#include <libmaus/util/SimpleHashMap.hpp>

namespace libmaus
{
	namespace trie
	{
		struct SimpleTrie
		{
			public:
			typedef SimpleTrie this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			private:
			static double const critload;
			
			uint64_t nextfree;
			libmaus::util::SimpleHashMap<uint64_t,uint64_t> H;
			uint64_t nextid;
			std::vector<int32_t> idvec;
			
			uint64_t insertTransition(uint64_t const from, uint8_t const sym)
			{
				uint64_t const trans = (from<<8) | sym;
				uint64_t curtarg;
				
				if ( H.contains(trans,curtarg) )
					return curtarg;
				else
				{
					while ( H.loadFactor() >= critload )
						H.extendInternal();
						
					uint64_t const newstate = nextfree++;
					H.insert(trans,newstate);
					idvec.push_back(-1);
					return newstate;
				}
			}
			
			public:
			uint64_t insert(uint8_t const * pa, uint8_t const * pe)
			{
				uint64_t state = 0;
				
				for ( uint8_t const * pc = pa; pc != pe; ++pc )
					state = insertTransition(state,*pc);
					
				if ( idvec[state] < 0 )
					idvec[state] = nextid++;
					
				return idvec[state];
			}
			
			uint64_t insert(char const * pa, char const * pe)
			{
				return insert(
					reinterpret_cast<uint8_t const *>(pa),
					reinterpret_cast<uint8_t const *>(pe)
				);
			}
			
			uint64_t insert(std::string const & s)
			{
				return insert(s.c_str(), s.c_str() + s.size());
			}
			
			SimpleTrie() : nextfree(1), H(1), nextid(0), idvec(1,-1) {}	
		};
	}
}
#endif
