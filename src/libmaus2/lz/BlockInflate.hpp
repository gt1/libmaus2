/*
    libmaus2
    Copyright (C) 2009-2016 German Tischler
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
#if ! defined(LIBMAUS2_LZ_BLOCKINFLATE_HPP)
#define LIBMAUS2_LZ_BLOCKINFLATE_HPP

#include <libmaus2/lz/Inflate.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BlockInflate
		{
			typedef BlockInflate this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef libmaus2::aio::InputStreamInstance istream_type;
			typedef ::libmaus2::util::unique_ptr<istream_type>::type istream_ptr_type;

			std::vector < std::pair<uint64_t,uint64_t> > index;
			uint64_t blockptr;
			uint64_t n;

			istream_ptr_type Pistr;
			istream_type & istr;

			::libmaus2::autoarray::AutoArray<uint8_t,::libmaus2::autoarray::alloc_type_c> B;
			uint8_t * pa;
			uint8_t * pc;
			uint8_t * pe;

			static std::vector < std::pair<uint64_t,uint64_t> > loadIndex(std::string const & filename)
			{
				libmaus2::aio::InputStreamInstance istr(filename);
				std::vector < std::pair<uint64_t,uint64_t> > index;

				istr.seekg(-8,std::ios::end);
				uint64_t const indexpos = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				// std::cerr << "index at position " << indexpos << std::endl;
				istr.seekg(indexpos,std::ios::beg);
				uint64_t const indexlen = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				// std::cerr << "indexlen " << indexlen << std::endl;
				for ( uint64_t i = 0; i < indexlen; ++i )
				{
					uint64_t const blockpos = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
					uint64_t const blocklen = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
					index.push_back(std::pair<uint64_t,uint64_t>(blockpos,blocklen));
					// std::cerr << "blockpos=" << blockpos << " blocklen=" << blocklen << std::endl;
				}
				istr.seekg(0,std::ios::beg);

				return index;
			}

			static uint64_t computeSize(std::vector < std::pair<uint64_t,uint64_t> > const & index)
			{
				uint64_t n = 0;
				for ( uint64_t i = 0; i < index.size(); ++i )
					n += index[i].second;
				return n;
			}

			static uint64_t computeSize(std::string const & filename)
			{
				return computeSize(loadIndex(filename));
			}

			static uint64_t computeSize(std::vector<std::string> const & filenames)
			{
				uint64_t n = 0;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					n += computeSize(filenames[i]);
				return n;
			}

			static std::vector<uint64_t> computeSizeVector(std::vector<std::string> const & filenames)
			{
				std::vector<uint64_t> sizes;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					sizes.push_back ( computeSize(filenames[i]) );
				return sizes;
			}

			BlockInflate(std::string const & filename, uint64_t pos = 0)
			: index(loadIndex(filename)), blockptr(0), n(computeSize(index)),
			  Pistr(new istream_type(filename)), istr(*Pistr),
			  B(), pa(0), pc(0), pe(0)
			{
				// std::cerr << "pos=" << pos << std::endl;
				while ( blockptr < index.size() && pos >= index[blockptr].second )
				{
					pos -= index[blockptr].second;
					blockptr++;
				}
				// std::cerr << "pos=" << pos << std::endl;
				decodeBlock();
				assert ( pc + pos <= pe );
				pc += pos;
			}

			uint64_t read(uint8_t * p, uint64_t n)
			{
				uint64_t red = 0;

				while ( n )
				{
					// std::cerr << "n=" << n << " red=" << red << std::endl;

					if ( pc == pe )
					{
						if ( ! decodeBlock() )
							return red;
					}

					uint64_t const toread = std::min(static_cast<uint64_t>(pe-pc),n);
					std::copy ( pc, pc + toread, p );
					pc += toread;
					p += toread;
					red += toread;
					n -= toread;
				}

				return red;
			}

			int get()
			{
				if ( pc == pe )
				{
					if ( ! decodeBlock() )
						return -1;
				}
				assert ( pc != pe );
				return *(pc++);
			}

			bool decodeBlock()
			{
				if ( blockptr >= index.size() )
					return false;

				istr.clear();
				istr.seekg ( index[blockptr].first, std::ios::beg );
				uint64_t const blocklen = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				if ( B.size() < blocklen )
				{
					B.resize(blocklen);
				}
				pa = B.begin();
				pc = pa;
				pe = pa + blocklen;
				Inflate inflate(istr);
				inflate.read(reinterpret_cast<char *>(pa),blocklen);

				blockptr++;
				return true;
			}

			::libmaus2::autoarray::AutoArray<uint8_t> getReverse()
			{
				::libmaus2::autoarray::AutoArray<uint8_t> A(n,false);
				uint8_t * outptr = A.begin();

				for ( uint64_t i = 0; i < index.size(); ++i )
				{
					blockptr = index.size()-i-1;
					decodeBlock();
					std::reverse(pa,pe);
					while ( pc != pe )
						*(outptr++) = *(pc++);
				}

				assert ( outptr == A.end() );

				return A;
			}
		};
	}
}
#endif
