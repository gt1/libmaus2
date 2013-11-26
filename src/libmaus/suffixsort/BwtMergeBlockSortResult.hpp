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
			private:
			uint64_t blockp0rank;
			libmaus::autoarray::AutoArray < ::libmaus::suffixsort::BwtMergeZBlock > zblocks;
			
			uint64_t blockstart;
			uint64_t cblocksize;
			
			::libmaus::suffixsort::BwtMergeTempFileNameSet files;

			public:
			BwtMergeBlockSortResult()
			: blockp0rank(0), zblocks(), blockstart(0), cblocksize(0), files()
			{
			
			}
			
			BwtMergeBlockSortResult(BwtMergeBlockSortResult const & o)
			: blockp0rank(o.blockp0rank), zblocks(o.zblocks.clone()), blockstart(o.blockstart), cblocksize(o.cblocksize), files(o.files)
			{
			
			}
			
			BwtMergeBlockSortResult & operator=(BwtMergeBlockSortResult const & o)
			{
				if ( this != &o )
				{
					blockp0rank = o.blockp0rank;
					zblocks = o.zblocks.clone();
					blockstart = o.blockstart;
					cblocksize = o.cblocksize;
					files = o.files;
				}
				return *this;
			}
			
			uint64_t getBlockP0Rank() const { return blockp0rank; }
			uint64_t getBlockStart() const { return blockstart; }
			uint64_t getCBlockSize() const { return cblocksize; }
			void setBlockP0Rank(uint64_t const rblockp0rank) { blockp0rank = rblockp0rank; }
			void setBlockStart(uint64_t const rblockstart) { blockstart = rblockstart; }
			void setCBlockSize(uint64_t const rcblocksize) { cblocksize = rcblocksize; }
			void setBWT(std::vector<std::string> const & bwt) { files.setBWT(bwt); }
			::libmaus::suffixsort::BwtMergeTempFileNameSet const & getFiles() const { return files; }
			void removeFiles() const { files.removeFiles(); }
			void removeFilesButBwt() const { files.removeFilesButBwt(); }
			void setTempPrefixAndRegisterAsTemp(std::string const & prefix, uint64_t const numbwt) 
			{ files.setPrefixAndRegisterAsTemp(prefix,numbwt); }
			libmaus::autoarray::AutoArray < ::libmaus::suffixsort::BwtMergeZBlock > const & getZBlocks() const { return zblocks; }
			void resizeZBlocks(uint64_t const n) { zblocks.resize(n); }
			void setZBlock(uint64_t const i, ::libmaus::suffixsort::BwtMergeZBlock const & z) { zblocks.at(i) = z; }
			void setTempFileSet(::libmaus::suffixsort::BwtMergeTempFileNameSet const & rfiles) { files = rfiles; }
			
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

