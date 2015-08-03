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
#if ! defined(LIBMAUS2_LZ_RAZFINDEX_HPP)
#define LIBMAUS2_LZ_RAZFINDEX_HPP

#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/lz/GzipHeader.hpp>
#include <libmaus2/lz/RAZFConstants.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct RAZFIndex : public ::libmaus2::lz::RAZFConstants
		{
			uint64_t uncompressed;
			uint64_t compressed;

			std::vector<uint64_t> binoffsets;
			std::vector<uint32_t> celloffsets;
			
			uint64_t blocksize;
			uint64_t headerlength;
						
			static bool hasRazfHeader(std::istream & in, uint64_t & headerlength, uint64_t & blocksize)
			{
				in.clear();
				in.seekg(0,std::ios::beg);

				try
				{
					libmaus2::lz::GzipHeader firstHeader(in);
					
					if ( 
						firstHeader.extradata.size() != 7
						||
						firstHeader.extradata.substr(0,4) != "RAZF"
					)
						return false;
					
					uint8_t const * bp = reinterpret_cast<uint8_t const *>(firstHeader.extradata.c_str());
					blocksize = 
						(static_cast<uint64_t>(bp[5]) << 8) |
						(static_cast<uint64_t>(bp[6]) << 0);
						
					headerlength = in.tellg();

					in.clear();
					in.seekg(0,std::ios::beg);
					
					return true;
				}
				catch(...)
				{
					in.clear();
					in.seekg(0,std::ios::beg);

					return false;
				}
			}

			static bool hasRazfHeader(std::istream & in)
			{
				uint64_t headerlength, blocksize;
				return hasRazfHeader(in,headerlength,blocksize);
			}
			
			static bool hasRazfHeader(std::string const & filename)
			{
				libmaus2::aio::InputStreamInstance CIS(filename);
				return hasRazfHeader(CIS);
			}
			
			void init(std::istream & in)
			{
				if ( ! hasRazfHeader(in,headerlength,blocksize) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "RAZFIndex::init(): file does not have a valid razf header" << std::endl;
					lme.finish();
					throw lme;				
				}

				in.clear();
				in.seekg(-2*sizeof(uint64_t),std::ios::end);
				
				uint64_t const indexpos = in.tellg();
				
				uncompressed = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				compressed = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				
				in.clear();
				in.seekg(compressed,std::ios::beg);
				
				uint64_t const indexsize = libmaus2::util::NumberSerialisation::deserialiseNumber(in,sizeof(uint32_t));
				
				uint64_t const v32 = indexsize / razf_bin_size + 1;
				
				binoffsets.resize(v32);
				for ( uint64_t i = 0; i < v32; ++i )
					binoffsets[i] = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				celloffsets.resize(indexsize);
				for ( uint64_t i = 0; i < indexsize; ++i )
					celloffsets[i] = libmaus2::util::NumberSerialisation::deserialiseNumber(in,sizeof(uint32_t));

				assert ( static_cast<int64_t>(in.tellg()) == static_cast<int64_t>(indexpos) );
	
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
				libmaus2::aio::InputStreamInstance CIS(fn);
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
