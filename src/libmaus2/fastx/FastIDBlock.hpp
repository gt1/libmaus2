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

#if ! defined(FASTIDBLOCK_HPP)
#define FASTIDBLOCK_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <string>

namespace libmaus2
{
	namespace fastx
	{
		struct FastIDBlock
		{
			unsigned int blockid;
			uint64_t numid;
			uint64_t blocksize;
			::libmaus2::autoarray::AutoArray < std::string > aids;
			std::string * ids;
			uint64_t idbase;
			
			private:
			FastIDBlock & operator=(FastIDBlock const &);
			FastIDBlock(FastIDBlock const &);
			
			public:
			FastIDBlock ( )
			: blockid(0), numid(0), blocksize(0), aids(numid,false), ids(aids.get()), idbase(0) {}
			
			void setup(uint64_t rnumid)
			{
				numid = rnumid;
				blocksize = 0;
				aids = ::libmaus2::autoarray::AutoArray < std::string >(numid,false);
				ids = aids.get();
				idbase = 0;
			}
		};
	}
}
#endif
