/*
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
*/
#if ! defined(LIBMAUS_AIO_CIRCULARWRAPPER_HPP)
#define LIBMAUS_AIO_CIRCULARWRAPPER_HPP

#include <libmaus/aio/CircularBuffer.hpp>
#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/bitio/CompactDecoderBuffer.hpp>
#include <libmaus/bitio/PacDecoderBuffer.hpp>
#include <libmaus/util/unique_ptr.hpp>
#include <libmaus/util/shared_ptr.hpp>

namespace libmaus
{
	namespace aio
	{
		/**
		 * wrapper class which makes wrapped byte oriented file or stream circular
		 **/
		struct CircularWrapper : public CircularBuffer, public ::std::istream
		{
			typedef CircularWrapper this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			/**
			 * constructor from filename
			 * @param filename name of file
			 * @param offset in file
			 * @param buffersize size of streambuf object
			 * @param pushbacksize size of push back buffer
			 **/
			CircularWrapper(
				std::string const & filename, 
				uint64_t const offset = 0,
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: CircularBuffer(filename,offset,buffersize,pushbackspace), ::std::istream(this)
			{
				
			}
			/**
			 * constructor istream object
			 * @param rin input stream
			 * @param offset in file
			 * @param buffersize size of streambuf object
			 * @param pushbacksize size of push back buffer
			 **/
			CircularWrapper(
				std::istream & rin, 
				uint64_t const offset = 0,
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: CircularBuffer(rin,offset,buffersize,pushbackspace), ::std::istream(this)
			{
				
			}
			
			/**
			 * return read position in absolute stream
			 **/
			uint64_t tellg() const
			{
				return CircularBuffer::tellg();
			}
		};

		/**
		 * wrapper class which makes wrapped utf-8 code based file or stream circular
		 **/
		struct Utf8CircularWrapper : public Utf8CircularBuffer, public ::std::wistream
		{
			typedef Utf8CircularWrapper this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			/**
			 * constructor from filename
			 * @param filename name of file
			 * @param offset in file
			 * @param buffersize size of streambuf object
			 * @param pushbacksize size of push back buffer
			 **/
			Utf8CircularWrapper(
				std::string const & filename, 
				uint64_t const offset = 0,
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: Utf8CircularBuffer(filename,offset,buffersize,pushbackspace), ::std::wistream(this)
			{
				
			}
			/**
			 * constructor istream object
			 * @param rin input stream
			 * @param offset in file
			 * @param buffersize size of streambuf object
			 * @param pushbacksize size of push back buffer
			 **/
			Utf8CircularWrapper(
				std::wistream & rin, 
				uint64_t const offset = 0,
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: Utf8CircularBuffer(rin,offset,buffersize,pushbackspace), ::std::wistream(this)
			{
				
			}
			/**
			 * return symbol read position in absolute stream
			 **/
			uint64_t tellg() const
			{
				return Utf8CircularBuffer::tellg();
			}
		};

		/**
		 * wrapper class which makes wrapped byte oriented file or stream circular and reversed
		 **/
		struct CircularReverseWrapper : public CircularReverseBuffer, public ::std::istream
		{
			typedef CircularReverseWrapper this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			/**
			 * constructor from filename
			 * @param filename name of file
			 * @param offset in file
			 * @param buffersize size of streambuf object
			 * @param pushbacksize size of push back buffer
			 **/
			CircularReverseWrapper(
				std::string const & filename, 
				uint64_t const offset = 0,
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: CircularReverseBuffer(filename,offset,buffersize,pushbackspace), ::std::istream(this)
			{
				
			}
			/**
			 * constructor istream object
			 * @param rin input stream
			 * @param offset in file
			 * @param buffersize size of streambuf object
			 * @param pushbacksize size of push back buffer
			 **/
			CircularReverseWrapper(
				std::istream & rin, 
				uint64_t const offset = 0,
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: CircularReverseBuffer(rin,offset,buffersize,pushbackspace), ::std::istream(this)
			{
				
			}
			/**
			 * return symbol read position in absolute stream
			 **/
			uint64_t tellg() const
			{
				return CircularReverseBuffer::tellg();
			}
		};

		/**
		 * wrapper class which makes wrapped utf-8 code based file or stream circular and reversed
		 **/
		struct Utf8CircularReverseWrapper : public Utf8CircularReverseBuffer, public ::std::wistream
		{
			typedef Utf8CircularReverseWrapper this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;

			/**
			 * constructor from filename
			 * @param filename name of file
			 * @param offset in file
			 * @param buffersize size of streambuf object
			 * @param pushbacksize size of push back buffer
			 **/
			Utf8CircularReverseWrapper(
				std::string const & filename, 
				uint64_t const offset = 0,
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: Utf8CircularReverseBuffer(filename,offset,buffersize,pushbackspace), ::std::wistream(this)
			{
				
			}
			/**
			 * constructor istream object
			 * @param rin input stream
			 * @param offset in file
			 * @param buffersize size of streambuf object
			 * @param pushbacksize size of push back buffer
			 **/
			Utf8CircularReverseWrapper(
				std::wistream & rin, 
				uint64_t const offset = 0,
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: Utf8CircularReverseBuffer(rin,offset,buffersize,pushbackspace), ::std::wistream(this)
			{
				
			}
			/**
			 * return symbol read position in absolute stream
			 **/
			uint64_t tellg() const
			{
				return Utf8CircularReverseBuffer::tellg();
			}
		};
		
		/**
		 * class wrapping an object of type libmaus::aio::CheckedInputStream
		 **/
		struct CheckedInputStreamWrapper
		{
			//! wrapped object
			::libmaus::aio::CheckedInputStream stream;
			
			/**
			 * constructor from file name
			 *
			 * @param filename file name
			 **/
			CheckedInputStreamWrapper(std::string const & filename)
			: stream(filename)
			{
			
			}
		};

		/**
		 * class wrapping an object of type Utf8DecoderWrapper
		 **/
		struct Utf8DecoderWrapperWrapper
		{
			//! wrapped object
			::libmaus::util::Utf8DecoderWrapper stream;
			
			/**
			 * constructor from file name
			 *
			 * @param filename file name
			 **/
			Utf8DecoderWrapperWrapper(std::string const & filename) : stream(filename)
			{
			
			}
		};
		
		/**
		 * class wrapping an object of type libmaus::bitio::CompactDecoderWrapper
		 **/
		struct CompactDecoderWrapperWrapper
		{
			//! wrapped object
			::libmaus::bitio::CompactDecoderWrapper stream;
			
			/**
			 * constructor from file name
			 *
			 * @param filename file name
			 **/
			CompactDecoderWrapperWrapper(std::string const & filename)
			: stream(filename)
			{
			
			}
		};

		/**
		 * class wrapping an object of type ::libmaus::bitio::PacDecoderWrapper
		 **/
		struct PacDecoderWrapperWrapper
		{
			//! wrapped object
			::libmaus::bitio::PacDecoderWrapper stream;
			
			/**
			 * constructor from file name
			 *
			 * @param filename file name
			 **/
			PacDecoderWrapperWrapper(std::string const & filename)
			: stream(filename)
			{
			
			}
		};

		/**
		 * class wrapping an object of type ::libmaus::bitio::PacDecoderTermWrapper
		 **/
		struct PacDecoderTermWrapperWrapper
		{
			//! wrapped object
			::libmaus::bitio::PacDecoderTermWrapper stream;
			
			/**
			 * constructor from file name
			 *
			 * @param filename file name
			 **/
			PacDecoderTermWrapperWrapper(std::string const & filename)
			: stream(filename)
			{
			
			}
		};
		
		/**
		 * class for instantiating a circular input stream
		 **/
		struct CheckedCircularWrapper : public CheckedInputStreamWrapper, public CircularWrapper
		{
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param offset initial file offset
			 * @param buffersize size of streambuf object
			 * @param pushbackspace size of push back buffer
			 **/
			CheckedCircularWrapper(
				std::string const & filename, 
				uint64_t const offset, 
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: CheckedInputStreamWrapper(filename), CircularWrapper(CheckedInputStreamWrapper::stream,offset,buffersize,pushbackspace)
			{
			
			}
		};

		/**
		 * class for instantiating a circular compact input stream
		 **/
		struct CompactCircularWrapper : public CompactDecoderWrapperWrapper, public CircularWrapper
		{
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param offset initial file offset
			 * @param buffersize size of streambuf object
			 * @param pushbackspace size of push back buffer
			 **/
			CompactCircularWrapper(
				std::string const & filename, 
				uint64_t const offset, 
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: CompactDecoderWrapperWrapper(filename), CircularWrapper(CompactDecoderWrapperWrapper::stream,offset,buffersize,pushbackspace)
			{
			
			}
		};

		/**
		 * class for instantiating a circular pac input stream
		 **/
		struct PacCircularWrapper : public PacDecoderWrapperWrapper, public CircularWrapper
		{
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param offset initial file offset
			 * @param buffersize size of streambuf object
			 * @param pushbackspace size of push back buffer
			 **/
			PacCircularWrapper(
				std::string const & filename, 
				uint64_t const offset, 
				uint64_t const buffersize = 64*1024, 
				uint64_t const pushbackspace = 64
			)
			: PacDecoderWrapperWrapper(filename), CircularWrapper(PacDecoderWrapperWrapper::stream,offset,buffersize,pushbackspace)
			{
			
			}
		};

		/**
		 * class for instantiating a circular terminated pac input stream
		 **/
		struct PacTermCircularWrapper : public PacDecoderTermWrapperWrapper, public CircularWrapper
		{
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param offset initial file offset
			 * @param buffersize size of streambuf object
			 * @param pushbackspace size of push back buffer
			 **/
			PacTermCircularWrapper(std::string const & filename, uint64_t const offset, uint64_t const buffersize = 64*1024, uint64_t const pushbackspace = 64)
			: PacDecoderTermWrapperWrapper(filename), CircularWrapper(PacDecoderTermWrapperWrapper::stream,offset,buffersize,pushbackspace)
			{
			
			}
		};

		/**
		 * class for instantiating a circular utf-8 coded input stream
		 **/
		struct Utf8CircularWrapperWrapper : public Utf8DecoderWrapperWrapper, public Utf8CircularWrapper
		{
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param offset initial file offset
			 * @param buffersize size of streambuf object
			 * @param pushbackspace size of push back buffer
			 **/
			Utf8CircularWrapperWrapper(std::string const & filename, uint64_t const offset, uint64_t const buffersize = 64*1024, uint64_t const pushbackspace = 64)
			: Utf8DecoderWrapperWrapper(filename), Utf8CircularWrapper(Utf8DecoderWrapperWrapper::stream,offset,buffersize,pushbackspace)
			{
			
			}
		};

		/**
		 * class for instantiating a circular reversed input stream
		 **/
		struct CheckedCircularReverseWrapper : public CheckedInputStreamWrapper, public CircularReverseWrapper
		{
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param offset initial file offset
			 * @param buffersize size of streambuf object
			 * @param pushbackspace size of push back buffer
			 **/
			CheckedCircularReverseWrapper(std::string const & filename, uint64_t const offset, uint64_t const buffersize = 64*1024, uint64_t const pushbackspace = 64)
			: CheckedInputStreamWrapper(filename), CircularReverseWrapper(CheckedInputStreamWrapper::stream,offset,buffersize,pushbackspace)
			{
			
			}
		};

		/**
		 * class for instantiating a circular reversed compact input stream
		 **/
		struct CompactCircularReverseWrapper : public CompactDecoderWrapperWrapper, public CircularReverseWrapper
		{
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param offset initial file offset
			 * @param buffersize size of streambuf object
			 * @param pushbackspace size of push back buffer
			 **/
			CompactCircularReverseWrapper(std::string const & filename, uint64_t const offset, uint64_t const buffersize = 64*1024, uint64_t const pushbackspace = 64)
			: CompactDecoderWrapperWrapper(filename), CircularReverseWrapper(CompactDecoderWrapperWrapper::stream,offset,buffersize,pushbackspace)
			{
			
			}
		};

		/**
		 * class for instantiating a circular reversed pac input stream
		 **/
		struct PacCircularReverseWrapper : public PacDecoderWrapperWrapper, public CircularReverseWrapper
		{
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param offset initial file offset
			 * @param buffersize size of streambuf object
			 * @param pushbackspace size of push back buffer
			 **/
			PacCircularReverseWrapper(std::string const & filename, uint64_t const offset, uint64_t const buffersize = 64*1024, uint64_t const pushbackspace = 64)
			: PacDecoderWrapperWrapper(filename), CircularReverseWrapper(PacDecoderWrapperWrapper::stream,offset,buffersize,pushbackspace)
			{
			
			}
		};

		/**
		 * class for instantiating a circular reversed terminated pac input stream
		 **/
		struct PacTermCircularReverseWrapper : public PacDecoderTermWrapperWrapper, public CircularReverseWrapper
		{
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param offset initial file offset
			 * @param buffersize size of streambuf object
			 * @param pushbackspace size of push back buffer
			 **/
			PacTermCircularReverseWrapper(std::string const & filename, uint64_t const offset, uint64_t const buffersize = 64*1024, uint64_t const pushbackspace = 64)
			: PacDecoderTermWrapperWrapper(filename), CircularReverseWrapper(PacDecoderTermWrapperWrapper::stream,offset,buffersize,pushbackspace)
			{
			
			}
		};

		/**
		 * class for instantiating a circular reversed utf-8 coded input stream
		 **/
		struct Utf8CircularReverseWrapperWrapper : public Utf8DecoderWrapperWrapper, public Utf8CircularReverseWrapper
		{
			/**
			 * constructor
			 *
			 * @param filename file name
			 * @param offset initial file offset
			 * @param buffersize size of streambuf object
			 * @param pushbackspace size of push back buffer
			 **/
			Utf8CircularReverseWrapperWrapper(std::string const & filename, uint64_t const offset, uint64_t const buffersize = 64*1024, uint64_t const pushbackspace = 64)
			: Utf8DecoderWrapperWrapper(filename), Utf8CircularReverseWrapper(Utf8DecoderWrapperWrapper::stream,offset,buffersize,pushbackspace)
			{
			
			}
		};
	}
}
#endif
