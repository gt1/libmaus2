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

#if ! defined(COMPACTDECODER_HPP)
#define COMPACTDECODER_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/GetLength.hpp>

namespace libmaus2
{
	namespace aio
	{
		/**
		 * class for decoding a sequence of 4 bit numbers encoded
		 * by the CompactEncoder4 class from a sequence of files
		 **/
		template < typename filename_container_type = std::vector<std::string> >
		struct CompactDecoder4 : public ::libmaus2::aio::GetLength
		{
			//! this type
			typedef CompactDecoder4<filename_container_type> this_type;
			//! unique pointer type
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			private:
			//! input stream type
			typedef libmaus2::aio::InputStreamInstance input_stream_type;
			//! input stream unique pointer type
			typedef ::libmaus2::util::unique_ptr<input_stream_type>::type input_stream_pointer_type;
			
			//! buffer for undecoded input data
			::libmaus2::autoarray::AutoArray<uint8_t> decodeBuffer;
			//! buffer for decoded input data
			::libmaus2::autoarray::AutoArray<uint8_t> buffer;
			//! current pointer in decoded data
			uint8_t * pc;
			//! end pointer for decoded data
			uint8_t * pe;
			
			//! local filename vector
			filename_container_type const localfilenames;
			//! reference to list of filenames
			filename_container_type const & filenames;
			//! const iterator type for file name vector
			typename filename_container_type::const_iterator fita;

			//! total length of decoded sequence
			uint64_t const n;
			
			//! current input file
			input_stream_pointer_type istr;
			//! number of symbols to be skipped
			uint64_t skip;
			//! number of unread symbols in current file
			uint64_t fileunread;
				
			/**
			 * open next file in list
			 *
			 * @return if next file was opened successfully
			 **/
			bool openNextFile()
			{
				while ( fita != filenames.end() && skip >= getLength(*fita) )
				{
					skip -= getLength(*fita);
					fita++;
				}

				if ( fita == filenames.end() )
					return false;
					
				assert ( fita != filenames.end() );
				assert ( skip < getLength(*fita) );
				
				istr = input_stream_pointer_type(new input_stream_type(*fita));
				fita++;
				fileunread = ::libmaus2::util::NumberSerialisation::deserialiseNumber(*istr);
				assert ( istr->is_open() );
				
				if ( skip/2 )
				{
					istr->seekg(skip/2, std::ios::cur);
					fileunread -= 2*(skip/2);
					skip -= 2*(skip/2);
				}

				assert ( fileunread );
				assert ( skip < 2 );
				
				return true;
			}

			/**
			 * fill the input buffer
			 *
			 * @return true iff data was successfully put in buffer
			 **/
			bool fillBuffer()
			{
				while ( pc == pe )
				{
					if ( ! fileunread )
					{
						bool const ok = openNextFile();
						if ( ! ok )
							return false;
					}

					uint64_t const toreadbytes = std::min(decodeBuffer.size(), (fileunread+1) / 2 );
					uint64_t const toreadsyms  = std::min( 2*toreadbytes, fileunread );
					istr->read(reinterpret_cast<char *>(decodeBuffer.get()), toreadbytes);
					
					if ( istr->gcount() != static_cast<int64_t>(toreadbytes) )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "CompactDecoder4::fillBuffer failed to fill buffer." << std::endl;
						se.finish();
						throw se;
					}
					
					for ( uint64_t i = 0; i < toreadbytes; ++i )
					{
						buffer[2*i+0] = decodeBuffer[i] & ((1u<<4)-1);
						buffer[2*i+1] = decodeBuffer[i] >> 4;
					}
					
					pc = buffer.begin();
					pe = pc + toreadsyms;
					fileunread -= toreadsyms;
					
					assert ( pc != pe );
					
					if ( skip )
					{
						assert ( skip == 1 );
						skip = 0;
						pc++;
					}
				}
				
				return true;
			}

			
			public:
			/**
			 * constructor from single filename
			 *
			 * @param filename name of file
			 * @param roffset initial reading offset
			 **/
			CompactDecoder4(std::string const & filename, uint64_t const roffset = 0)
			: 
			  decodeBuffer(4096,false), buffer(2*decodeBuffer.size(),false), pc(buffer.begin()), pe(buffer.begin()),
			  localfilenames(1,filename),
			  filenames(localfilenames),
			  fita(filenames.begin()),
			  n(getLength(filenames.begin(),filenames.end()) >= roffset ? getLength(filenames.begin(),filenames.end())-roffset : 0),
			  skip(roffset),
			  fileunread(0)
			{
			
			}

			/**
			 * constructor from a list of file names
			 *
			 * @param rfilenames list of file names
			 * @param roffset initial reading offset
			 **/
			CompactDecoder4(filename_container_type const & rfilenames, uint64_t const roffset = 0)
			: 
			  decodeBuffer(4096,false), buffer(2*decodeBuffer.size(),false), pc(buffer.begin()), pe(buffer.begin()),
			  localfilenames(),
			  filenames(rfilenames),
			  fita(filenames.begin()),
			  n(getLength(filenames.begin(),filenames.end()) >= roffset ? getLength(filenames.begin(),filenames.end())-roffset : 0),
			  skip(roffset),
			  fileunread(0)
			{
			
			}
			
			/**
			 * @return next symbol or -1 for eof
			 **/
			int get()
			{
				if ( pc == pe )
				{
					fillBuffer();
					if ( pc == pe )
						return -1;
				}
				return *(pc++);
			}

			/**
			 * read a complete file to memory
			 *
			 * @param filename file name
			 * @param offset start reading after offset symbols
			 * @return data in file
			 **/
			static ::libmaus2::autoarray::AutoArray<uint8_t> readFile(std::string const & filename, uint64_t const offset = 0)
			{
				CompactDecoder4 CD(filename,offset);
				::libmaus2::autoarray::AutoArray<uint8_t> D(CD.n);
				for ( uint64_t i = 0; i < D.size(); ++i )
					D[i] = CD.get();
				return D;
			}
			
			/**
			 * compute a list of segments split points (points where the file has value 0)
			 *
			 * @param V list of file names
			 * @param segments number of split points to be computed
			 * @return list of split points
			 **/
			static std::vector<uint64_t> getSplitPoints(filename_container_type const & V, uint64_t const segments)
			{
				uint64_t const n = getLength(V.begin(),V.end());
				std::vector < uint64_t > points;
				for ( uint64_t i = 0; i < segments; ++i )
				{
					uint64_t const scanstart = i * ((n+segments-1)/segments);
					// std::cerr << "scanstart " << scanstart << std::endl;
					this_type CD(V,scanstart);
					uint64_t offset = 0;
					while ( (scanstart+offset < n) && (CD.get() != 0) )
						offset++;
					points.push_back(scanstart + offset);
					// std::cerr << "offset " << offset << std::endl;
				}
				
				for ( uint64_t i = 0; i < points.size(); ++i )
					if ( points[i] != n )
					{
						this_type CD(V,points[i]);
						assert ( CD.get() == 0 );
					}
					
				points.push_back(n);
					
				return points;
			}

		};
	}
}
#endif
