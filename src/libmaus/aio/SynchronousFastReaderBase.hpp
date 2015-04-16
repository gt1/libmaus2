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


#if ! defined(SYNCHRONOUSFASTREADERBASE_HPP)
#define SYNCHRONOUSFASTREADERBASE_HPP

#include <libmaus/fastx/CharBuffer.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/aio/CheckedInputStream.hpp>
#include <cerrno>

namespace libmaus
{
	namespace aio
	{
		/**
		 * synchronous base class for FastA/FastQ input
		 **/
		struct SynchronousFastReaderBase
		{
			//! this type
			typedef SynchronousFastReaderBase this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			//! input file names
			std::vector<std::string> const filenames;
		
			//! input type	
			typedef std::ifstream input_type;
			//! input pointer type
			typedef ::libmaus::util::unique_ptr<input_type>::type input_type_ptr;
			//! file pointer first type
			typedef input_type_ptr fpt_first_type;
			//! file pointer second type
			typedef std::vector<std::string>::const_iterator fpt_second_type;
			//! file pointer type
			typedef std::pair < fpt_first_type, fpt_second_type > file_pair_type;
			//! pointer to file pointer type
			typedef ::libmaus::util::unique_ptr<file_pair_type>::type file_pair_ptr_type;
			//! offset in file type
			typedef std::pair< fpt_second_type, uint64_t > file_offset_type;

			//! current file pointer
			file_pair_ptr_type fpt;
			//! input buffer
			::libmaus::autoarray::AutoArray<uint8_t> B;
			//! input buffer start pointer
			uint8_t * const pa;
			//! input buffer current pointer
			uint8_t * pc;
			//! input buffer end pointer
			uint8_t * pe;
			//! number of symbols extracted
			uint64_t c;
			
			//! character buffer
			::libmaus::fastx::CharBuffer cb;
			
			/**
			 * get file pointer object for numerical offset in a list of files
			 *
			 * @param filenames list of files
			 * @param offset number of bytes to skip
			 * @return file offset pair
			 **/
			static file_offset_type getFileOffset(
				std::vector<std::string> const & filenames,
				uint64_t offset)			
			{
				std::vector<std::string>::const_iterator ita = filenames.begin();
				
				uint64_t l;
				while ( ita != filenames.end() && offset >= (l=::libmaus::util::GetFileSize::getFileSize(*ita)) )
					++ita, offset -= l;
			
				return file_offset_type(ita,offset);
			}
			
			/**
			 * set up input for a list of file names and an offset in bytes
			 *
			 * @param filenames list of input files
			 * @param offset start offset in bytes
			 * @return file pair pointer
			 **/
			static file_pair_ptr_type setupInput(
				std::vector<std::string> const & filenames,
				uint64_t offset)
			{
				std::vector<std::string>::const_iterator ita = filenames.begin();
				
				uint64_t l;
				while ( ita != filenames.end() && offset >= (l=::libmaus::util::GetFileSize::getFileSize(*ita)) )
					++ita, offset -= l;
				
				if ( ita != filenames.end() )
				{
					input_type_ptr itp(new input_type(ita->c_str(),std::ios::binary));
					
					if ( !itp->is_open() )
					{
						::libmaus::exception::LibMausException se;
						se.getStream() << "Failed to open file " << *ita << ": " << strerror(errno);
						se.finish();
						throw se;
					}
					
					if ( offset != 0 )
						itp->seekg(offset,std::ios::beg);
					
					file_pair_ptr_type fpt(new file_pair_type());
					fpt->first = UNIQUE_PTR_MOVE(itp);
					fpt->second = ita;
					return UNIQUE_PTR_MOVE(fpt);
				}
				else
				{
					file_pair_ptr_type fptr(new file_pair_type());
					return UNIQUE_PTR_MOVE(fptr);
				}
			}
			
			/**
			 * transform a single file name to a list of file names
			 **/
			static std::vector<std::string> singleToList(std::string const & filename)
			{
				std::vector<std::string> filenames;
				filenames.push_back(filename);
				return filenames;
			}

			/**
			 * read a one byte number and check for eof
			 *
			 * @return number
			 **/
			uint64_t readNumber1()
			{
				int const v = getNextCharacter();
				if ( v < 0 )
				{
					::libmaus::exception::LibMausException ex;
					ex.getStream() << "Failed to read number in ::libmaus::aio::SynchronousFastReaderBase::readNumber1().";
					ex.finish();
					throw ex;
				}
				return v;
			}
			/**
			 * read a two byte number and check for eof
			 *
			 * @return number
			 **/
			uint64_t readNumber2()
			{
				uint64_t const v0 = readNumber1();
				uint64_t const v1 = readNumber1();
				return (v0<<8)|v1;
			}
			/**
			 * read a four byte number and check for eof
			 *
			 * @return number
			 **/
			uint64_t readNumber4()
			{
				uint64_t const v0 = readNumber2();
				uint64_t const v1 = readNumber2();
				return (v0<<16)|v1;
			}
			/**
			 * read a eight byte number and check for eof
			 *
			 * @return number
			 **/
			uint64_t readNumber8()
			{
				uint64_t const v0 = readNumber4();
				uint64_t const v1 = readNumber4();
				return (v0<<32)|v1;
			}
			
			SynchronousFastReaderBase(SynchronousFastReaderBase const &);
			SynchronousFastReaderBase & operator=(SynchronousFastReaderBase const &);

			public:
			/**
			 * constructor
			 *
			 * @param rfilename input file name
			 * @param bufsize input buffer size
			 * @param offset read start offset
			 **/
			SynchronousFastReaderBase(
				std::string const & rfilename, 
				unsigned int = 0, 
				unsigned int bufsize = 1024, 
				uint64_t const offset = 0)
			: 
				filenames(singleToList(rfilename)),
				fpt(setupInput(filenames,offset)),
				B(bufsize),
				pa(B.get()),
				pc(pa),
				pe(pc),
				c(0),
				cb()
			{
			}

			/**
			 * constructor
			 *
			 * @param rfilenames list of input file name
			 * @param bufsize input buffer size
			 * @param offset read start offset
			 **/
			SynchronousFastReaderBase(
				std::vector<std::string> const & rfilenames,
				unsigned int = 0, 
				unsigned int bufsize = 1024, 
				uint64_t const offset = 0)
			: 
				filenames(rfilenames),
				fpt(setupInput(filenames,offset)),
				B(bufsize),
				pa(B.get()),
				pc(pa),
				pe(pc),
				c(0),
				cb()
			{
			}

			/**
			 * @return number of bytes read
			 **/
			uint64_t getC() const
			{
				return c;
			}

			/**
			 * @return next character or -1 for EOF
			 **/
			int get()
			{
				return getNextCharacter();
			}

			/**
			 * @return next character or -1 for EOF
			 **/
			int getNextCharacter()
			{
				while ( pc == pe )
				{
					/* no input stream? */
					if ( !(fpt->first.get()) )
						return -1;
					
					fpt->first->read ( reinterpret_cast<char *>(pa), B.size() );
					
					if (  fpt->first->gcount() )
					{
						pc = pa;
						pe = pc+fpt->first->gcount();
					}
					else
					{
						fpt->second++;
						fpt->first.reset();
						if ( fpt->second != filenames.end() )
						{
							input_type_ptr tfptfirst(new input_type(fpt->second->c_str(),std::ios::binary));
							fpt->first = UNIQUE_PTR_MOVE(tfptfirst);
						}
					}
				}
				
				c += 1;
				return *(pc++);
			}
			
			/**
			 * get line as character array
			 *
			 * @return pair of character pointer and line length
			 **/
			std::pair < char const *, uint64_t > getLineRaw()
			{
				int c;
				cb.reset();
				while ( (c=getNextCharacter()) >= 0 && c != '\n' )
					cb.bufferPush(c);
				
				if ( cb.length == 0 && c == -1 )
					return std::pair<char const *, uint64_t>(reinterpret_cast<char const *>(0),0);
				else
					return std::pair<char const *, uint64_t>(cb.buffer,cb.length);
			}
			
			/**
			 * get line as string object
			 *
			 * @param s string object to store the line
			 * @return true iff line was read (false for eof)
			 **/
			bool getLine(std::string & s)
			{
				std::pair < char const *, uint64_t > P = getLineRaw();
				
				if ( P.first )
				{
					s = std::string(P.first,P.first+P.second);
					return true;
				}
				else
				{
					return false;
				}
			}
			
			/**
			 * get line offsets for line number modulo mod equaling zero
			 *
			 * @param filename input file name
			 * @param mod modulo value
			 * @return line offsets in filename
			 **/
			static std::vector < uint64_t > getLineOffsets(std::string const & filename, uint64_t const mod)
			{
				SynchronousFastReaderBase SFRB(filename);
				return SFRB.getLineOffsets(mod);
			}

			/**
			 * get line offsets for line number modulo mod equaling zero
			 *
			 * @param mod modulo value
			 * @return line offsets
			 **/
			std::vector < uint64_t > getLineOffsets(uint64_t const mod)
			{
				std::vector<uint64_t> V;
				
				int ch = 0;
				uint64_t l = 0;

				V.push_back(0);
				l++;

				while ( (ch=getNextCharacter()) >= 0 )
					if ( ch == '\n' )
						if ( (l++ % mod) == 0 )
						{
							V.push_back(c-1);
							// std::cerr << "Pushed " << V.back() << std::endl;
						}
				
				return V;
			}
		};
	}
}
#endif
