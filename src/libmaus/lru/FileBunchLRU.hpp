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
#if ! defined(LIBMAUS_LRU_FILEBUNCHLRU_HPP)
#define LIBMAUS_LRU_FILEBUNCHLRU_HPP

#include <libmaus/lru/LRU.hpp>

namespace libmaus
{
	namespace lru
	{
		struct FileBunchLRU : public LRU
		{
			typedef FileBunchLRU this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			uint64_t const lrusize;
			std::vector < std::string > filenames;
			
			// file id -> lru id
			::libmaus::autoarray::AutoArray < uint64_t > mapping;
			// lru id -> file id
			::libmaus::autoarray::AutoArray < uint64_t > rmapping;
			
			// file pointers
			typedef ::libmaus::util::unique_ptr< std::ofstream  >::type file_ptr_type;
			::libmaus::autoarray::AutoArray < file_ptr_type > files;

			FileBunchLRU ( std::vector < std::string > const & rfilenames, uint64_t rlrusize = 1024)
			: LRU(rlrusize), lrusize(rlrusize), filenames ( rfilenames ), mapping(filenames.size()), rmapping(lrusize), files(lrusize)
			{
				std::fill ( mapping.get(), mapping.get() + mapping.getN(), lrusize );
			}
			
			~FileBunchLRU()
			{
				for ( uint64_t i = 0; i < files.size(); ++i )
					if ( files[i].get() )
					{
						files[i] -> flush();
						files[i] -> close();
						files[i].reset();
					}
			}
			
			std::ofstream & getFile(uint64_t const fileid)
			{
				if ( mapping[fileid] != lrusize )
				{
					update(mapping[fileid]);
					return *(files[mapping[fileid]]);
				}
				
				std::pair < std::pair < uint64_t, uint64_t >, bool > P = add();
				
				if ( ! P.second )
				{
					uint64_t const kickedlruid = P.first.first;
					uint64_t const kickedfileid = rmapping[kickedlruid];
					files [ kickedlruid ] -> flush ();
					files [ kickedlruid ] -> close ();
					files [ kickedlruid ] . reset ( );
					mapping [ kickedfileid ] = lrusize;
					rmapping [ kickedlruid ] = 0;
				}
				
				mapping [ fileid ] = P.first.first;
				rmapping [ P.first.first ] = fileid;
				file_ptr_type tptr(new std::ofstream( filenames[fileid].c_str(), std::ios::binary | std::ios::app ) );
				files [ mapping [ fileid] ] = UNIQUE_PTR_MOVE(tptr);
				
				return *(files[mapping[fileid]]);
			}
		};
	}
}
#endif
