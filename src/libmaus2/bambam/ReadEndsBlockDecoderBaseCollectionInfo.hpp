/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_READENDSBLOCKDECODERBASECOLLECTIONINFO_HPP)
#define LIBMAUS2_BAMBAM_READENDSBLOCKDECODERBASECOLLECTIONINFO_HPP

#include <libmaus2/bambam/ReadEndsBlockDecoderBaseCollectionInfoBase.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/parallel/PosixMutex.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct ReadEndsBlockDecoderBaseCollectionInfoLockedStream
		{
			typedef ReadEndsBlockDecoderBaseCollectionInfoLockedStream this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			libmaus2::parallel::ScopePosixMutex mutex;
			std::istream & istr;

			ReadEndsBlockDecoderBaseCollectionInfoLockedStream(libmaus2::parallel::PosixMutex & rmutex, std::istream & ristr) : mutex(rmutex), istr(ristr)
			{}

			public:
			static unique_ptr_type construct(libmaus2::parallel::PosixMutex & rmutex, std::istream & ristr)
			{
				unique_ptr_type tptr(new this_type(rmutex,ristr));
				return UNIQUE_PTR_MOVE(tptr);
			}

			std::istream & getStream()
			{
				return istr;
			}
		};

		struct ReadEndsBlockDecoderBaseCollectionInfoDataStreamProvider
		{
			virtual ~ReadEndsBlockDecoderBaseCollectionInfoDataStreamProvider() {}
			virtual ReadEndsBlockDecoderBaseCollectionInfoLockedStream::unique_ptr_type getDataStream() = 0;
		};

		struct ReadEndsBlockDecoderBaseCollectionInfoIndexStreamProvider
		{
			virtual ~ReadEndsBlockDecoderBaseCollectionInfoIndexStreamProvider() {}
			virtual ReadEndsBlockDecoderBaseCollectionInfoLockedStream::unique_ptr_type getIndexStream() = 0;
		};

		struct ReadEndsBlockDecoderBaseCollectionInfo
		:
			public ReadEndsBlockDecoderBaseCollectionInfoBase,
			public ReadEndsBlockDecoderBaseCollectionInfoDataStreamProvider,
			public ReadEndsBlockDecoderBaseCollectionInfoIndexStreamProvider
		{
			typedef ReadEndsBlockDecoderBaseCollectionInfo this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef libmaus2::aio::InputStream input_stream_type;

			// private:
			input_stream_type::shared_ptr_type datastr;
			input_stream_type::shared_ptr_type indexstr;

			libmaus2::parallel::PosixMutex datamutex;
			libmaus2::parallel::PosixMutex indexmutex;

			public:
			ReadEndsBlockDecoderBaseCollectionInfo() : ReadEndsBlockDecoderBaseCollectionInfoBase(), datastr(), indexstr()
			{}

			ReadEndsBlockDecoderBaseCollectionInfo(
				std::string const & rdatafilename,
				std::string const & rindexfilename,
				std::vector < uint64_t > const & rblockelcnt,
				std::vector < uint64_t > const & rindexoffset
			) :
			    ReadEndsBlockDecoderBaseCollectionInfoBase(rdatafilename,rindexfilename,rblockelcnt,rindexoffset),
			    datastr (libmaus2::aio::InputStreamFactoryContainer::constructShared(ReadEndsBlockDecoderBaseCollectionInfoBase::datafilename)),
			    indexstr(libmaus2::aio::InputStreamFactoryContainer::constructShared(ReadEndsBlockDecoderBaseCollectionInfoBase::indexfilename))
			{}

			ReadEndsBlockDecoderBaseCollectionInfo(ReadEndsBlockDecoderBaseCollectionInfoBase const & O)
			:
			    ReadEndsBlockDecoderBaseCollectionInfoBase(O),
			    datastr (libmaus2::aio::InputStreamFactoryContainer::constructShared(ReadEndsBlockDecoderBaseCollectionInfoBase::datafilename)),
			    indexstr(libmaus2::aio::InputStreamFactoryContainer::constructShared(ReadEndsBlockDecoderBaseCollectionInfoBase::indexfilename))
			{
			}

			ReadEndsBlockDecoderBaseCollectionInfoLockedStream::unique_ptr_type getDataStream()
			{
				ReadEndsBlockDecoderBaseCollectionInfoLockedStream::unique_ptr_type tptr(
					ReadEndsBlockDecoderBaseCollectionInfoLockedStream::construct(datamutex,*datastr)
				);

				return UNIQUE_PTR_MOVE(tptr);
			}

			ReadEndsBlockDecoderBaseCollectionInfoLockedStream::unique_ptr_type getIndexStream()
			{
				ReadEndsBlockDecoderBaseCollectionInfoLockedStream::unique_ptr_type tptr(
					ReadEndsBlockDecoderBaseCollectionInfoLockedStream::construct(indexmutex,*indexstr)
				);

				return UNIQUE_PTR_MOVE(tptr);
			}
		};

		std::ostream & operator<<(std::ostream & out, ReadEndsBlockDecoderBaseCollectionInfo const & O)
		{
			out << static_cast<ReadEndsBlockDecoderBaseCollectionInfoBase const &>(O);
			return out;
		}
	}
}
#endif
