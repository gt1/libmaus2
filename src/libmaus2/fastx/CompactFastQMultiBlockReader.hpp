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
#if ! defined(LIBMAUS2_FASTX_COMPACTFASTQMULTIBLOCKREADER_HPP)
#define LIBMAUS2_FASTX_COMPACTFASTQMULTIBLOCKREADER_HPP

#include <libmaus2/fastx/CompactFastQSingleBlockReader.hpp>
#include <libmaus2/aio/FileFragment.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct CompactFastQMultiBlockReaderBase
		{
			static std::vector< ::libmaus2::fastx::FastInterval > loadIndex(std::string const & filename)
			{
				return ::libmaus2::fastx::CompactFastDecoder::loadIndex(filename);
			}

			static uint64_t getNonEmptyDataSize(std::string const & filename)
			{
				std::vector< ::libmaus2::fastx::FastInterval > const index = loadIndex(filename);
				if ( ! index.size() )
					return 0;
				else
					return index.back().fileoffsethigh;
			}

			static uint64_t getEmptyBlockSize()
			{
				return CompactFastQHeader::getEmptyBlockHeaderSize();
			}

			static ::std::vector < ::libmaus2::aio::FileFragment > getFragments(std::vector<std::string> const & filenames)
			{
				::std::vector < ::libmaus2::aio::FileFragment > frags;

				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					std::string const & fn = filenames[i];
					uint64_t const offset = 0;
					uint64_t const length = getNonEmptyDataSize(fn) + (((i+1)<filenames.size()) ? 0 : getEmptyBlockSize());
					frags.push_back(::libmaus2::aio::FileFragment(fn,offset,length));
					// std::cerr << frags.back() << std::endl;
				}

				return frags;
			}

			static ::std::vector < ::libmaus2::aio::FileFragment > getFragments(std::string const & filename)
			{
				return getFragments(std::vector<std::string>(1,filename));
			}
		};

		template<typename _stream_type>
		struct CompactFastQMultiBlockReader
		{
			typedef _stream_type stream_type;
			typedef CompactFastQMultiBlockReader<stream_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef ::libmaus2::fastx::CompactFastQSingleBlockReader<stream_type> single_block_reader_type;
			typedef typename single_block_reader_type::unique_ptr_type single_block_reader_ptr_type;
			typedef typename single_block_reader_type::pattern_type pattern_type;

			stream_type & stream;
			uint64_t nextid;
			uint64_t maxdecode;
			single_block_reader_ptr_type rptr;
			bool finished;

			bool openNextStream()
			{
				if ( finished )
					return false;

				if ( rptr )
				{
					nextid = rptr->nextid;
					maxdecode -= rptr->numdecoded;
				}

				rptr = UNIQUE_PTR_MOVE(
					single_block_reader_ptr_type(
						new single_block_reader_type(stream,nextid,maxdecode)
					)
				);

				if ( rptr->numreads )
				{
					// std::cerr << "opened block containing " << rptr->numreads << " reads." << std::endl;
					return true;
				}
				else
				{
					rptr.reset();
					finished = true;
					return false;
				}
			}

			CompactFastQMultiBlockReader(
				stream_type & rstream,
				uint64_t rnextid = 0,
				uint64_t const rmaxdecode = std::numeric_limits<uint64_t>::max()
			)
			: stream(rstream), nextid(rnextid), maxdecode(rmaxdecode), finished(false)
			{
				openNextStream();
			}

			bool getNextPatternUnlocked(pattern_type & pattern)
			{
				while ( ! finished )
				{
					bool const ok = rptr->getNextPatternUnlocked(pattern);

					if ( ok )
						return true;
					else
						openNextStream();
				}

				return false;
			}

		};

		struct MultiFileCompactFastQDecoder
		{
			typedef MultiFileCompactFastQDecoder this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			::libmaus2::aio::ReorderConcatGenericInput<uint8_t> stream;
			CompactFastQMultiBlockReader< ::libmaus2::aio::ReorderConcatGenericInput<uint8_t> > reader;
			typedef CompactFastQMultiBlockReader< ::libmaus2::aio::ReorderConcatGenericInput<uint8_t> >::pattern_type pattern_type;

			MultiFileCompactFastQDecoder(std::vector<std::string> const & filenames)
			: stream(CompactFastQMultiBlockReaderBase::getFragments(filenames)), reader(stream)
			{

			}
			MultiFileCompactFastQDecoder(std::string const & filename)
			: stream(CompactFastQMultiBlockReaderBase::getFragments(filename)), reader(stream)
			{

			}

			bool getNextPatternUnlocked(pattern_type & pattern)
			{
				return reader.getNextPatternUnlocked(pattern);
			}
		};
	}
}
#endif
