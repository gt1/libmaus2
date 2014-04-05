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
#if ! defined(LIBMAUS_LZ_RAZFINDEX_HPP)
#define LIBMAUS_LZ_RAZFINDEX_HPP

#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/lz/GzipHeader.hpp>
#include <libmaus/lz/RAZFConstants.hpp>
#include <libmaus/util/NumberSerialisation.hpp>

namespace libmaus
{
	namespace lz
	{
		struct RAZFIndex : public ::libmaus::lz::RAZFConstants
		{
			uint64_t uncompressed;
			uint64_t compressed;

			std::vector<uint64_t> binoffsets;
			std::vector<uint32_t> celloffsets;
			
			uint64_t headerlength;
			
			void init(std::istream & in)
			{
				in.clear();
				in.seekg(-2*sizeof(uint64_t),std::ios::end);
				
				uint64_t const indexpos = in.tellg();
				
				uncompressed = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				compressed = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				
				in.clear();
				in.seekg(compressed,std::ios::beg);
				
				uint64_t const indexsize = libmaus::util::NumberSerialisation::deserialiseNumber(in,sizeof(uint32_t));
				
				uint64_t const v32 = indexsize / razf_bin_size + 1;
				
				binoffsets.resize(v32);
				for ( uint64_t i = 0; i < v32; ++i )
					binoffsets[i] = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				celloffsets.resize(indexsize);
				for ( uint64_t i = 0; i < indexsize; ++i )
					celloffsets[i] = libmaus::util::NumberSerialisation::deserialiseNumber(in,sizeof(uint32_t));

				assert ( in.tellg() == static_cast<int64_t>(indexpos) );
				
				in.clear();
				in.seekg(0,std::ios::beg);
				libmaus::lz::GzipHeader firstHeader(in);
			
				headerlength = in.tellg();
				
				in.clear();
				in.seekg(0,std::ios::beg);
			}
			
			RAZFIndex()
			: uncompressed(0), compressed(0), headerlength(0)
			{
			
			}
			
			RAZFIndex(std::istream & in)
			: uncompressed(0), compressed(0), headerlength(0)
			{
				init(in);
			}

			RAZFIndex(std::string const & fn)
			: uncompressed(0), compressed(0), headerlength(0)
			{
				libmaus::aio::CheckedInputStream CIS(fn);
				init(CIS);
			}	
			
			std::pair<uint64_t,uint64_t> seekg(uint64_t const pos) const
			{
				int64_t const idx = static_cast<int64_t>(pos / razf_block_size) - 1;
				
				if ( idx < 0 )
					return std::pair<uint64_t,uint64_t>(headerlength,pos);
				else
					return std::pair<uint64_t,uint64_t>(celloffsets[idx] + binoffsets[idx / razf_bin_size],pos-(idx+1)*razf_block_size);
			}
			
			uint64_t operator[](int64_t const pidx) const
			{
				int64_t const idx = pidx-1;
				
				if ( idx < 0 )
					return headerlength;
				else
					return celloffsets[idx] + binoffsets[idx / razf_bin_size];
			}
		};

		std::ostream & operator<<(std::ostream & out, RAZFIndex const & index);
	}
}
#endif
