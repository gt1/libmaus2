/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_READENDSBLOCKDECODERBASECOLLECTIONINFO_HPP)
#define LIBMAUS_BAMBAM_READENDSBLOCKDECODERBASECOLLECTIONINFO_HPP

#include <libmaus/bambam/ReadEndsBlockDecoderBaseCollectionInfoBase.hpp>
#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/aio/PosixFdInputStream.hpp>
#include <libmaus/parallel/PosixMutex.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct ReadEndsBlockDecoderBaseCollectionInfoLockedStream
		{
			typedef ReadEndsBlockDecoderBaseCollectionInfoLockedStream this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			private:			
			libmaus::parallel::ScopePosixMutex mutex;
			std::istream & istr;
			
			ReadEndsBlockDecoderBaseCollectionInfoLockedStream(libmaus::parallel::PosixMutex & rmutex, std::istream & ristr) : mutex(rmutex), istr(ristr)
			{}
			
			public:
			static unique_ptr_type construct(libmaus::parallel::PosixMutex & rmutex, std::istream & ristr)
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
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;			
			
			typedef libmaus::aio::PosixFdInputStream input_stream_type;
			// typedef libmaus::aio::CheckedInputStream input_stream_type;
			
			// private:		
			input_stream_type::shared_ptr_type datastr;
			input_stream_type::shared_ptr_type indexstr;
			
			libmaus::parallel::PosixMutex datamutex;
			libmaus::parallel::PosixMutex indexmutex;
			
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
			    datastr(new input_stream_type(ReadEndsBlockDecoderBaseCollectionInfoBase::datafilename)),
			    indexstr(new input_stream_type(ReadEndsBlockDecoderBaseCollectionInfoBase::indexfilename))
			{}
			
			ReadEndsBlockDecoderBaseCollectionInfo(ReadEndsBlockDecoderBaseCollectionInfoBase const & O)
			: 
			    ReadEndsBlockDecoderBaseCollectionInfoBase(O), 
			    datastr(new input_stream_type(ReadEndsBlockDecoderBaseCollectionInfoBase::datafilename)),
			    indexstr(new input_stream_type(ReadEndsBlockDecoderBaseCollectionInfoBase::indexfilename))
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
	}
}
#endif
