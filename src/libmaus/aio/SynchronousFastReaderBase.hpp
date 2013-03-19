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


#if ! defined(SYNCHRONOUSFASTREADERBASE_HPP)
#define SYNCHRONOUSFASTREADERBASE_HPP

#include <libmaus/fastx/CharBuffer.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <cerrno>

namespace libmaus
{
	namespace aio
	{
		struct SynchronousFastReaderBase
		{
			typedef SynchronousFastReaderBase this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			std::vector<std::string> const filenames;
			
			typedef std::ifstream input_type;
			typedef ::libmaus::util::unique_ptr<input_type>::type input_type_ptr;
			typedef input_type_ptr fpt_first_type;
			typedef std::vector<std::string>::const_iterator fpt_second_type;
			typedef std::pair < fpt_first_type, fpt_second_type > file_pair_type;
			typedef ::libmaus::util::unique_ptr<file_pair_type>::type file_pair_ptr_type;
			
			typedef std::pair< fpt_second_type, uint64_t > file_offset_type;

			file_pair_ptr_type fpt;
			::libmaus::autoarray::AutoArray<uint8_t> B;
			uint8_t * const pa;
			uint8_t * pc;
			uint8_t * pe;
			uint64_t c;
			
			::libmaus::fastx::CharBuffer cb;
			
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
					input_type_ptr itp = UNIQUE_PTR_MOVE(input_type_ptr(new input_type(ita->c_str(),std::ios::binary)));
					
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
					return UNIQUE_PTR_MOVE(file_pair_ptr_type(new file_pair_type()));
				}
			}
			

			static std::vector<std::string> singleToList(std::string const & filename)
			{
				std::vector<std::string> filenames;
				filenames.push_back(filename);
				return filenames;
			}

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
			uint64_t readNumber2()
			{
				uint64_t const v0 = readNumber1();
				uint64_t const v1 = readNumber1();
				return (v0<<8)|v1;
			}
			uint64_t readNumber4()
			{
				uint64_t const v0 = readNumber2();
				uint64_t const v1 = readNumber2();
				return (v0<<16)|v1;
			}
			uint64_t readNumber8()
			{
				uint64_t const v0 = readNumber4();
				uint64_t const v1 = readNumber4();
				return (v0<<32)|v1;
			}

			public:
			SynchronousFastReaderBase(
				std::string const & rfilename, 
				unsigned int = 0, 
				unsigned int bufsize = 1024, 
				uint64_t const offset = 0)
			: 
				filenames(singleToList(rfilename)),
				fpt(UNIQUE_PTR_MOVE(setupInput(filenames,offset))),
				B(bufsize),
				pa(B.get()),
				pc(pa),
				pe(pc),
				c(0)
			{
			}

			SynchronousFastReaderBase(
				std::vector<std::string> const & rfilenames,
				unsigned int = 0, 
				unsigned int bufsize = 1024, 
				uint64_t const offset = 0)
			: 
				filenames(rfilenames),
				fpt(UNIQUE_PTR_MOVE(setupInput(filenames,offset))),
				B(bufsize),
				pa(B.get()),
				pc(pa),
				pe(pc),
				c(0)
			{
			}

			uint64_t getC() const
			{
				return c;
			}

			int get()
			{
				return getNextCharacter();
			}

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
							fpt->first = UNIQUE_PTR_MOVE(input_type_ptr(new input_type(fpt->second->c_str(),std::ios::binary)));
					}
				}
				
				c += 1;
				return *(pc++);
			}
			
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
			
			static std::vector < uint64_t > getLineOffsets(std::string const & filename, uint64_t const mod)
			{
				SynchronousFastReaderBase SFRB(filename);
				return SFRB.getLineOffsets(mod);
			}

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
