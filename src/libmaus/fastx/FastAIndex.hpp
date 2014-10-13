/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_FASTX_FASTAINDEX_HPP)
#define LIBMAUS_FASTX_FASTAINDEX_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/fastx/FastAIndexEntry.hpp>
#include <libmaus/util/stringFunctions.hpp>
#include <libmaus/exception/LibMausException.hpp>

#include <vector>
#include <sstream>

namespace libmaus
{
	namespace fastx
	{
		struct FastAIndex
		{
			typedef FastAIndex this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
					
			std::vector<libmaus::fastx::FastAIndexEntry> sequences;
			std::map<std::string,uint64_t> shortNameToId;
			
			FastAIndex() : sequences()
			{
			
			}
			
			uint64_t getSequenceIdByName(std::string const & s) const
			{
				std::string const shortname = computeShortName(s);
				
				if ( shortNameToId.find(shortname) != shortNameToId.end() )
					return shortNameToId.find(shortname)->second;
					
				libmaus::exception::LibMausException lme;
				lme.getStream() << "libmaus::fastx::FastAIndex::getSequenceIdByName(" << s << "): sequence is not in database\n";
				lme.finish();
				throw lme;
			}
			
			static std::string computeShortName(std::string const & s)
			{
				uint64_t i = 0;
				while ( 
					i < s.size() && 
					(!isspace(static_cast<unsigned char>(s[i])))
				)
					++i;
					
				return s.substr(0,i);
			}
			
			static uint64_t parseNumber(std::string const & s)
			{
				std::istringstream istr(s);
				uint64_t n;
				istr >> n;
				
				if ( ! istr )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "FastAIndexEntry::parseNumber(): cannot parse " << s << " as number" << std::endl;
					lme.finish();
					throw lme;
				}
				
				return n;
			}
			
			FastAIndex(std::istream & in) : sequences()
			{
				while ( in )
				{
					std::string line;
					std::getline(in,line);
					
					if ( line.size() )
					{
						std::deque<std::string> tokens = libmaus::util::stringFunctions::tokenize<std::string>(line,std::string("\t"));
						
						if ( tokens.size() >= 5 )
						{
							shortNameToId[computeShortName(tokens[0])] = sequences.size();
							
							sequences.push_back(
								libmaus::fastx::FastAIndexEntry(
									tokens[0],
									parseNumber(tokens[1]),
									parseNumber(tokens[2]),
									parseNumber(tokens[3]),
									parseNumber(tokens[4])
								)
							);
						}
					}
				}
			}
			
			static unique_ptr_type load(std::string const & filename)
			{
				libmaus::aio::CheckedInputStream CIS(filename);
				unique_ptr_type tptr(new this_type(CIS));
				return UNIQUE_PTR_MOVE(tptr);
			}
			
			libmaus::autoarray::AutoArray<char> readSequence(std::istream & in, int64_t const seqid) const
			{
				if ( seqid < 0 || seqid >= static_cast<int64_t>(sequences.size()) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "FastAIndexEntry::readSequence(): sequence id " << seqid << " is out of range" << std::endl;
					lme.finish();
					throw lme;				
				}
				
				libmaus::fastx::FastAIndexEntry const entry = sequences.at(seqid);
				uint64_t const lineskip = entry.bytesperline-entry.basesperline;
			
				libmaus::autoarray::AutoArray<char> A(entry.length,false);
				char * cur = A.begin();
				
				uint64_t todo = entry.length;
				
				in.clear();
				in.seekg(entry.offset,std::ios::beg);
				
				while ( todo )
				{
					uint64_t const re = std::min(todo,entry.basesperline);
					
					in.read(cur,re);
					
					if ( in.gcount() != static_cast<int64_t>(re) )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "FastAIndexEntry::readSequence(): failed to read sequence " << entry.name << std::endl;
						lme.finish();
						throw lme;					
					}

					cur += re;
					todo -= re;
					
					if ( todo )
					{
						in.ignore(lineskip);

						if ( in.gcount() != static_cast<int64_t>(lineskip) )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "FastAIndexEntry::readSequence(): failed to read sequence " << entry.name << std::endl;
							lme.finish();
							throw lme;				
						}	
					}				
				}
				
				return A;
			}

			libmaus::autoarray::AutoArray<char> readSequenceRange(std::istream & in, int64_t const seqid, int64_t const low, int64_t const high) const
			{
				if ( seqid < 0 || seqid >= static_cast<int64_t>(sequences.size()) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "FastAIndexEntry::readSequenceRange(): sequence id " << seqid << " is out of range" << std::endl;
					lme.finish();
					throw lme;				
				}
				
				libmaus::fastx::FastAIndexEntry const entry = sequences.at(seqid);

				if ( low < 0 || high > static_cast<int64_t>(entry.length) || low > high )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "FastAIndexEntry::readSequenceRange(): invalid range" << std::endl;
					lme.finish();
					throw lme;				
				}

				uint64_t const lineskip = entry.bytesperline-entry.basesperline;

				uint64_t todo = high-low;
				libmaus::autoarray::AutoArray<char> A(todo,false);
				char * cur = A.begin();
				
				// seek to position on sequence
				in.clear();
				in.seekg(entry.offset + ( low / entry.basesperline ) * entry.bytesperline + (low % entry.basesperline), std::ios::beg);
				
				// if we are not at a line start, then read to (at most) end of line
				if ( (low % entry.basesperline) != 0 )
				{
					uint64_t const re = std::min(todo,entry.basesperline - (low % entry.basesperline));

					in.read(cur,re);
					
					if ( in.gcount() != static_cast<int64_t>(re) )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "FastAIndexEntry::readSequenceRange(): failed to read sequence " << entry.name << std::endl;
						lme.finish();
						throw lme;					
					}

					cur += re;
					todo -= re;
					
					if ( todo )
					{
						in.ignore(lineskip);

						if ( in.gcount() != static_cast<int64_t>(lineskip) )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "FastAIndexEntry::readSequenceRange(): failed to read sequence " << entry.name << std::endl;
							lme.finish();
							throw lme;				
						}	
					}				
				}
				
				// read rest
				while ( todo )
				{
					uint64_t const re = std::min(todo,entry.basesperline);
					
					in.read(cur,re);
					
					if ( in.gcount() != static_cast<int64_t>(re) )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "FastAIndexEntry::readSequenceRange(): failed to read sequence " << entry.name << std::endl;
						lme.finish();
						throw lme;					
					}

					cur += re;
					todo -= re;
					
					if ( todo )
					{
						in.ignore(lineskip);

						if ( in.gcount() != static_cast<int64_t>(lineskip) )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "FastAIndexEntry::readSequenceRange(): failed to read sequence " << entry.name << std::endl;
							lme.finish();
							throw lme;				
						}	
					}				
				}
				
				assert ( cur == A.end() );
				
				return A;
			}
		};
		
		std::ostream & operator<<(std::ostream & out, FastAIndex const & index);
	}
}
#endif
