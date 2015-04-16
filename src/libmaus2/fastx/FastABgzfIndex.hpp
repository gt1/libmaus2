/*
    libmaus2
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
#if ! defined(LIBMAUS_FASTX_FASTABGZFINDEX_HPP)
#define LIBMAUS_FASTX_FASTABGZFINDEX_HPP

#include <libmaus2/fastx/FastABgzfIndexEntry.hpp>
#include <libmaus2/fastx/FastABgzfDecoder.hpp>
#include <libmaus2/trie/TrieState.hpp>

namespace libmaus2
{
	namespace fastx
	{
		struct FastABgzfIndex
		{
			typedef FastABgzfIndex this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			uint64_t blocksize;
			std::vector< ::libmaus2::fastx::FastABgzfIndexEntry > entries;
			std::vector< std::string > shortnames;
			::libmaus2::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type LHTnofailure;
			
			static unique_ptr_type load(std::string const & filename)
			{
				libmaus2::aio::CheckedInputStream CIS(filename);
				unique_ptr_type tptr(new this_type(CIS));
				return UNIQUE_PTR_MOVE(tptr);
			}

			void init(std::istream & indexistr)
			{
				indexistr.seekg(0,std::ios::beg);
				blocksize = libmaus2::util::NumberSerialisation::deserialiseNumber(indexistr);
				
				indexistr.seekg(-8,std::ios::end);
				uint64_t imetaoffset = libmaus2::util::NumberSerialisation::deserialiseNumber(indexistr);
				indexistr.seekg(imetaoffset,std::ios::beg);
				uint64_t const numseq = libmaus2::util::NumberSerialisation::deserialiseNumber(indexistr);
				
				if ( numseq )
				{
					uint64_t const firstpos = libmaus2::util::NumberSerialisation::deserialiseNumber(indexistr);
					
					indexistr.seekg(firstpos,std::ios::beg);
					
					entries.resize(numseq);
					shortnames.resize(numseq);

					for ( uint64_t i = 0; i < numseq; ++i )
					{
						entries[i] = ::libmaus2::fastx::FastABgzfIndexEntry(indexistr);
						shortnames[i] = entries[i].shortname;
					}

					::libmaus2::trie::Trie<char> trienofailure;
					trienofailure.insertContainer(shortnames);
					::libmaus2::trie::LinearHashTrie<char,uint32_t>::unique_ptr_type tLHTnofailure(trienofailure.toLinearHashTrie<uint32_t>());
					LHTnofailure = UNIQUE_PTR_MOVE(tLHTnofailure);
				}
			}
			
			FastABgzfIndex(std::istream & in)
			{
				init(in);
			}
			
			int64_t getSequenceId(std::string const & name) const
			{
				return LHTnofailure->searchCompleteNoFailureZ(name.begin());
			}

			int64_t getSequenceId(char const * name) const
			{
				return LHTnofailure->searchCompleteNoFailureZ(name);
			}
			
			::libmaus2::fastx::FastABgzfIndexEntry const & operator[](int64_t const i) const
			{
				if ( i < 0 )
				{
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "::libmaus2::fastx::FastABgzfIndexEntry::operator[] called for non existant id" << std::endl;
					ex.finish();
					throw ex;
				}
				
				return entries[i];
			}
			
			FastABgzfDecoder::unique_ptr_type getStream(std::istream & in, uint64_t const id) const
			{
				FastABgzfDecoder::unique_ptr_type Tptr(new FastABgzfDecoder(in,(*this)[id],blocksize));
				return UNIQUE_PTR_MOVE(Tptr);
			}

			FastABgzfDecoder::unique_ptr_type getStream(std::string const & filename, uint64_t const id) const
			{
				FastABgzfDecoder::unique_ptr_type Tptr(new FastABgzfDecoder(filename,(*this)[id],blocksize));
				return UNIQUE_PTR_MOVE(Tptr);
			}
		};

		::std::ostream & operator<<(::std::ostream & out, ::libmaus2::fastx::FastABgzfIndex const & F);
	}
}
#endif
