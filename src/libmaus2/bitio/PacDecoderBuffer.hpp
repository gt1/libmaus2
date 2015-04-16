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

#if ! defined(LIBMAUS2_AIO_PACDECODERBUFFER_HPP)
#define LIBMAUS2_AIO_PACDECODERBUFFER_HPP

#include <istream>
#include <fstream>
#include <ios>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/aio/CheckedInputStream.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/bitio/Ctz.hpp>
#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/bitio/ArrayDecode.hpp>

namespace libmaus2
{
	namespace bitio
	{
		struct PacDecoderBuffer : public ::std::streambuf
		{
			private:
			::libmaus2::aio::CheckedInputStream stream;
			
			// log of word size we are using
			static unsigned int const loglog = 3;
			// bits per word in memory
			static unsigned int const bitsperentity = (1u << loglog);
			
			uint64_t const b;
			uint64_t const n;
			uint64_t const alignmult;

			uint64_t const buffersize;
			::libmaus2::autoarray::AutoArray<uint8_t> C;
			::libmaus2::autoarray::AutoArray<char> buffer;
			
			uint64_t symsread;

			bool addterm;

			PacDecoderBuffer(PacDecoderBuffer const &);
			PacDecoderBuffer & operator=(PacDecoderBuffer&);
			
			static uint64_t getNumberOfSymbols(::libmaus2::aio::CheckedInputStream & stream)
			{
				stream.seekg(-1,std::ios::end);
				uint64_t const databytes = stream.tellg();
				
				if ( ! databytes )
					return 0;
				
				// number of symbols in last data byte
				int64_t const last = stream.get();
				
				stream.seekg(0);
				stream.clear();
				
				return (4 * (databytes-1))+last;
			}
			
			public:
			PacDecoderBuffer(
				std::string const & filename, 
				::std::size_t rbuffersize,
				bool const raddterm = false
			)
			: stream(filename),
			  b(2),
			  n(getNumberOfSymbols(stream)),
			  // smallest multiple aligning to bitsperentity bits
			  alignmult(1ull << (loglog-::libmaus2::bitio::Ctz::ctz(b))),
			  // make buffersize multiple of alignmult
			  buffersize(((rbuffersize + alignmult-1)/alignmult)*alignmult),
			  C((buffersize*b+7)/8),
			  buffer(buffersize,false),
			  symsread(0),
			  addterm(raddterm)
			{
				setg(buffer.end(), buffer.end(), buffer.end());	
			}

			private:
			// gptr as unsigned pointer
			uint8_t const * uptr() const
			{
				return reinterpret_cast<uint8_t const *>(gptr());
			}
			
			::std::streampos seekpos(::std::streampos sp, ::std::ios_base::openmode which = ::std::ios_base::in | ::std::ios_base::out)
			{
				if ( which & ::std::ios_base::in )
				{
					int64_t const cur = symsread-(egptr()-gptr());
					int64_t const curlow = cur - static_cast<int64_t>(gptr()-eback());
					int64_t const curhigh = cur + static_cast<int64_t>(egptr()-gptr());
					
					// call relative seek, if target is in range
					if ( sp >= curlow && sp <= curhigh )
						return seekoff(static_cast<int64_t>(sp) - cur, ::std::ios_base::cur, which);

					// target is out of range, we really need to seek
					uint64_t tsymsread = (sp / buffersize)*buffersize;
					
					symsread = tsymsread;
					stream.clear();
					stream.seekg( (symsread * b) / 8 );
					setg(buffer.end(),buffer.end(),buffer.end());
					underflow();
					setg(eback(),gptr() + (static_cast<int64_t>(sp)-static_cast<int64_t>(tsymsread)), egptr());
				
					return sp;
				}
				
				return -1;
			}
			
			::std::streampos seekoff(::std::streamoff off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which = ::std::ios_base::in | ::std::ios_base::out)
			{
				if ( which & ::std::ios_base::in )
				{
					int64_t abstarget = 0;
					int64_t const cur = symsread - (egptr()-gptr());
					
					if ( way == ::std::ios_base::cur )
						abstarget = cur + off;
					else if ( way == ::std::ios_base::beg )
						abstarget = off;
					else // if ( way == ::std::ios_base::end )
						abstarget = n + (addterm?1:0) + off;
						
					if ( abstarget - cur == 0 )
					{
						return abstarget;
					}
					else if ( (abstarget - cur) > 0 && (abstarget - cur) <= (egptr()-gptr()) )
					{
						setg(eback(),gptr()+(abstarget-cur),egptr());
						return abstarget;
					}
					else if ( (abstarget - cur) < 0 && (cur-abstarget) <= (gptr()-eback()) )
					{
						setg(eback(),gptr()-(cur-abstarget),egptr());
						return abstarget;
					}
					else
					{
						return seekpos(abstarget,which);
					}
				}
				
				return -1;
			}
			
			int_type underflow()
			{
				// if there is still data, then return it
				if ( gptr() < egptr() )
					return static_cast<int_type>(*uptr());

				assert ( gptr() == egptr() );

				uint64_t const symsleft = ((n+(addterm?1:0))-symsread);
				
				if ( symsleft == 0 )
					return traits_type::eof();

				uint64_t const lasttermblock = addterm ? ((symsleft <= buffersize)?1:0) : 0;
				uint64_t const symstoread = std::min(symsleft-lasttermblock,buffersize);
				
				uint64_t const wordstoread = (symstoread * b + (bitsperentity-1))/bitsperentity;
				uint64_t const bytestoread = wordstoread * (bitsperentity/8);
				
				// load packed data into memory
				stream.read ( reinterpret_cast<char *>(C.begin()) , bytestoread );
				if ( stream.gcount() != static_cast<int64_t>(bytestoread) )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "PacDecoderBuffer::underflow() failed to read " << bytestoread << " bytes." << std::endl;
					se.finish();
					throw se;
				}
			
				// decode array
				::libmaus2::bitio::ArrayDecode::decodeArray(
					C.begin(), reinterpret_cast<uint8_t *>(buffer.begin()), symstoread, 2
				);
				
				if ( addterm )
					for ( uint64_t i = 0; i < symstoread; ++i )
						buffer[i] += 1;
				if ( lasttermblock )
					buffer[symstoread] = 0;
				
				setg(buffer.begin(),buffer.begin(),buffer.begin()+symstoread+lasttermblock);

				symsread += symstoread+lasttermblock;
				
				return static_cast<int_type>(*uptr());
			}
		};

		template<bool _addterm>
		struct PacDecoderWrapperTemplate : public PacDecoderBuffer, public ::std::istream
		{
			static bool const addterm = _addterm;
			
			typedef PacDecoderWrapperTemplate<_addterm> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			PacDecoderWrapperTemplate(std::string const & filename, uint64_t const buffersize = 64*1024)
			: PacDecoderBuffer(filename,buffersize,addterm), ::std::istream(this)
			{
				
			}

			static int getSymbolAtPosition(std::string const & filename, uint64_t const offset)
			{
				this_type I(filename);
				I.seekg(offset);
				return I.get();
			}

			static uint64_t getFileSize(std::string const & filename)
			{
				this_type I(filename);
				I.seekg(0,std::ios::end);
				return I.tellg();
			}
		};
		
		typedef PacDecoderWrapperTemplate<false> PacDecoderWrapper;
		typedef PacDecoderWrapperTemplate<true> PacDecoderTermWrapper;
	}
}
#endif
