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

#if ! defined(COMPACTFASTDECODER_HPP)
#define COMPACTFASTDECODER_HPP

#include <libmaus2/aio/ReorderConcatGenericInput.hpp>
#include <libmaus2/fastx/CompactFastDecoderBase.hpp>
#include <libmaus2/fastx/FastInterval.hpp>
#include <libmaus2/fastx/Pattern.hpp>
#include <libmaus2/util/unique_ptr.hpp>

#if ! defined(_WIN32)
#include <libmaus2/network/SocketFastReaderBase.hpp>
#endif

#include <fstream>

namespace libmaus2
{
	namespace fastx
	{
		struct CompactFastDecoder : public CompactFastDecoderBase
		{
			typedef CompactFastDecoder this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::fastx::Pattern pattern_type;

			typedef std::ifstream file_input_stream_type;
			typedef ::libmaus2::util::unique_ptr<file_input_stream_type>::type file_input_stream_ptr_type;	

			file_input_stream_ptr_type inptr;
			std::istream & istr;
			std::vector< ::libmaus2::fastx::FastInterval > index;
			
			static uint64_t getNonDataSize(std::istream & istr)
			{
			        return getIndexLength(istr) + 8 + getTermLength();
			}
			static uint64_t getNonDataSize(std::string const & filename)
			{
                                std::ifstream istr(filename.c_str(),std::ios::binary);
                                return getNonDataSize(istr);
			}
			static uint64_t getDataSize(std::string const & filename)
			{
			        uint64_t const filesize = ::libmaus2::util::GetFileSize::getFileSize(filename);
			        uint64_t const datasize = filesize - getNonDataSize(filename);
			        return datasize;
			}
			
			static bool checkTerminator(std::string const & filename)
			{
                                std::ifstream istr(filename.c_str(),std::ios::binary);
                                int64_t const nd = getNonDataSize(istr);
                                istr.seekg(0,std::ios::end);
                                istr.seekg(-nd,std::ios::cur);
                                uint32_t const v = decodeUTF8(istr);
                                return v == getTerminator();
			}
			
			#if ! defined(_WIN32)
			static void removeIndexAndTerm(std::string const & filename)
			{
			        uint64_t const datasize = getDataSize(filename);

			        if ( truncate ( filename.c_str(), datasize ) < 0 )
			        {
			                ::libmaus2::exception::LibMausException se;
			                se.getStream() << "truncate(" << filename << "," << datasize << ") failed" << std::endl;
			                se.finish();
			                throw se;
			        }
			}
			#endif
			
			static uint64_t getIndexLength(std::istream & istr)
			{			
				istr.seekg(0,std::ios::end);
				istr.seekg(-8,std::ios::cur);
				uint64_t indexlen = 0;
				
				for ( uint64_t i = 0; i < 8; ++i )
				{
					indexlen <<= 8;
					indexlen |= static_cast<uint64_t>(istr.get());
				}
				
				return indexlen;
			}

			static std::vector< ::libmaus2::fastx::FastInterval > loadIndex(std::istream & istr)
			{
				std::vector< ::libmaus2::fastx::FastInterval > index;
				
				uint64_t const indexlen = getIndexLength(istr);
				
				istr.seekg(0,std::ios::end);
				istr.seekg(-8,std::ios::cur);

				istr.seekg(-static_cast<int64_t>(indexlen),std::ios::cur);

				index = ::libmaus2::fastx::FastInterval::deserialiseVector(istr);
				
				istr.seekg(0,std::ios::beg);
			
				return index;
			}

			static std::vector< ::libmaus2::fastx::FastInterval > loadIndex(std::string const & filename)
			{
			        std::ifstream istr(filename.c_str(),std::ios::binary);
			        return loadIndex(istr);
			}

			static std::vector< ::libmaus2::fastx::FastInterval > loadIndex(std::vector < std::string > const & filenames)
			{                        
			        std::vector< ::libmaus2::fastx::FastInterval > index;
			        
			        for ( uint64_t i = 0; i < filenames.size(); ++i )
					::libmaus2::fastx::FastInterval::append(index,loadIndex(filenames[i]));

			        return index;
			}

			static ::libmaus2::fastx::FastInterval loadIndexSingleInterval(std::vector < std::string > const & filenames)
			{
				std::vector< ::libmaus2::fastx::FastInterval > const index = loadIndex(filenames);
				return ::libmaus2::fastx::FastInterval::merge(index.begin(),index.end());
			}

			static ::libmaus2::aio::FileFragment getDataFragment(std::string const & filename)
			{
			        return ::libmaus2::aio::FileFragment ( filename, 0, getDataSize(filename) );
			}
			
			static std::vector < ::libmaus2::aio::FileFragment > getDataFragments(std::vector < std::string > const & filenames)
			{
			        std::vector < ::libmaus2::aio::FileFragment > fragments;
			        for ( uint64_t i = 0; i < filenames.size(); ++i )
			                fragments.push_back(getDataFragment(filenames[i]));

                                // add terminator for last fragment			                
                                if ( fragments.size() )
                                        fragments.back().len += getTermLength();
                                
                                return fragments;
			}

			CompactFastDecoder(std::string const & filename)
			: CompactFastDecoderBase(), 
			  inptr(new file_input_stream_type(filename.c_str(),std::ios::binary)), 
			  istr(*inptr), index(loadIndex(istr))
			{}

			CompactFastDecoder(std::istringstream & rin) : CompactFastDecoderBase(), istr(rin), index(loadIndex(istr)) {}

			bool getNextPatternUnlocked(pattern_type & pattern)
			{
				return decode(pattern,istr);
			}
			
			std::ostream & printIndex(std::ostream & out) const
			{
				for ( uint64_t i = 0; i < index.size(); ++i )
					out << index[i] << std::endl;
				return out;	
			}
		};

		struct CompactFastConcatDecoder : public CompactFastDecoderBase
		{
			typedef CompactFastConcatDecoder this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::fastx::Pattern pattern_type;
			
			typedef ::libmaus2::aio::ReorderConcatGenericInput<uint8_t> input_type;
			typedef input_type::unique_ptr_type input_ptr_type;
			
			typedef CompactFastDecoderBase base_type;

			std::vector < ::libmaus2::aio::FileFragment > fragments;
			::libmaus2::fastx::FastInterval const FI;
			input_ptr_type pistr;
			input_type & istr;
			
			CompactFastConcatDecoder(
			        std::vector<std::string> const & filenames, 
			        uint64_t const bufsize = 64*1024
                        )
			: fragments(CompactFastDecoder::getDataFragments(filenames)),
			  FI(0,std::numeric_limits<uint64_t>::max(),0,std::numeric_limits<uint64_t>::max(),std::numeric_limits<uint64_t>::max(),0/*minlen*/,0/*maxlen*/),
			  pistr(new input_type(fragments,bufsize,std::numeric_limits<uint64_t>::max()/* limit */,0/* offset */)),
			  istr(*pistr)
			{
			
			}

			CompactFastConcatDecoder(
			        std::vector<std::string> const & filenames,
			        ::libmaus2::fastx::FastInterval const & rFI,
			        uint64_t const bufsize = 64*1024
                        )
			: fragments(CompactFastDecoder::getDataFragments(filenames)), 
			  FI(rFI),
			  pistr(new input_type(fragments,bufsize,FI.fileoffsethigh-FI.fileoffset/* limit */,FI.fileoffset/* offset */)),
			  istr(*pistr)
			{
			        nextid = FI.low;
			}
			
			static ::std::vector< ::libmaus2::fastx::FastInterval> parallelIndex(::std::vector< ::std::string > const & filenames, uint64_t /* frags */)
			{
				return CompactFastDecoder::loadIndex(filenames);
			}

			template<typename strip_type>
                        static std::vector < ::libmaus2::fastx::FastInterval > computeCommonNameAlignedFrags(
                        	std::vector<std::string> const & filenames, uint64_t const numfrags, uint64_t const /* mod */, strip_type & /* strip */
			)
			{
				return parallelIndex(filenames,numfrags);
			}

			bool getNextPatternUnlocked(pattern_type & pattern)
			{
			        if ( nextid == FI.high )
			                return false;
                                else
        				return decode(pattern,istr);
			}

			int64_t skipPattern()
			{
			        return base_type::skipPattern(istr);
			}

			int64_t skipPattern(uint64_t & codelen)
			{
			        return base_type::skipPattern(istr,codelen);
			}

			static ::libmaus2::aio::FileFragment getDataFragment(std::string const & filename)
			{
			        return CompactFastDecoder::getDataFragment(filename);
			}
			
			static std::vector < ::libmaus2::aio::FileFragment > getDataFragments(std::vector < std::string > const & filenames)
			{
			        return CompactFastDecoder::getDataFragments(filenames);
			}
                };
                
                struct CompactFastConcatRandomAccessAdapter
                {
                	std::vector<std::string> const filenames;                	
			
			std::vector < ::libmaus2::aio::FileFragment > const fragments;
			::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > const fragmentintervals;
			::libmaus2::util::IntervalTree::unique_ptr_type fragmenttree;

			std::vector < libmaus2::fastx::FastInterval > const index;
			::libmaus2::util::IntervalTree::unique_ptr_type const indextree;
			
			CompactFastConcatRandomAccessAdapter(std::vector<std::string> const & rfilenames)
			: 
				filenames(rfilenames),
				fragments(CompactFastDecoder::getDataFragments(filenames)),
				fragmentintervals(::libmaus2::aio::FileFragment::toIntervalVector(fragments)),
				fragmenttree(::libmaus2::aio::FileFragment::toIntervalTree(fragments)),
				index(CompactFastDecoder::loadIndex(filenames)),
				indextree(libmaus2::fastx::FastInterval::toIntervalTree(index))
			{
			
			}

			uint64_t operator()(uint64_t i, libmaus2::autoarray::AutoArray<uint8_t> & D) const
			{
				assert ( index.size() );
				assert ( i < index.rbegin()->high );
				
				uint64_t const ii = indextree->find(i);
				
				assert ( i >= index[ii].low );
				assert ( i  < index[ii].high );
				
				i -= index[ii].low;
				
				assert ( fragmentintervals.size() );
				assert ( index[ii].fileoffset < fragmentintervals[fragmentintervals.size()-1].second );
				uint64_t fi = fragmenttree->find(index[ii].fileoffset);
				
				assert ( index[ii].fileoffset >= fragmentintervals[fi].first );
				assert ( index[ii].fileoffset  < fragmentintervals[fi].second );
				
				uint64_t const ioffset = index[ii].fileoffset - fragmentintervals[fi].first;
				
				libmaus2::aio::CheckedInputStream CIS(fragments[fi].filename);
				CIS.seekg(fragments[fi].offset + ioffset);
				
				for ( ; i ; --i )
					CompactFastDecoderBase::skipPattern(CIS);
					
				uint64_t const len = CompactFastDecoderBase::decodeSimple(CIS,D);
				
				for ( uint64_t j = 0; j < len; ++j )
					D[j] = libmaus2::fastx::remapChar(D[j]);
				
				return len;
			}
			
			std::string operator[](uint64_t const i) const
			{
				libmaus2::autoarray::AutoArray<uint8_t> D;
				uint64_t const len = (*this)(i,D);
				return std::string(D.begin(),D.begin()+len);
			}
		};

                #if ! defined(_WIN32)
		struct CompactFastSocketDecoder : public CompactFastDecoderBase
		{
			typedef CompactFastSocketDecoder this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::fastx::Pattern pattern_type;
			
			typedef ::libmaus2::network::SocketBase socket_type;
			typedef ::libmaus2::network::SocketFastReaderBase input_type;
			typedef input_type::unique_ptr_type input_ptr_type;

			::std::vector < ::libmaus2::fastx::FastInterval > const & index;
			::libmaus2::fastx::FastInterval const & FI;
			input_ptr_type pistr;
			input_type & istr;
			
			CompactFastSocketDecoder(
			        socket_type * const socket,
        			::std::vector < ::libmaus2::fastx::FastInterval > const & rindex,
        			uint64_t const i,
			        uint64_t const bufsize = 64*1024
                        )
			: index(rindex),
			  FI(index[i]),
			  pistr(new input_type(socket,bufsize)),
			  istr(*pistr)
			{
			        socket->writeMessage<uint64_t>(0,&i,1);
			        nextid = FI.low;
			}

			bool getNextPatternUnlocked(pattern_type & pattern)
			{
			        if ( nextid == FI.high )
			                return false;
                                else
        				return decode(pattern,istr);
			}
                };
                #endif
	}
}
#endif
