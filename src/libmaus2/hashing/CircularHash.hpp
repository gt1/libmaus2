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

#if ! defined(LIBMAUS_HASHING_CIRCULARHASH_HPP)
#define LIBMAUS_HASHING_CIRCULARHASH_HPP

#include <libmaus2/bitbtree/bitbtree.hpp>
#include <libmaus2/bambam/BamAlignmentExpungeCallback.hpp>

namespace libmaus2
{
	namespace hashing
	{
		template<typename _overflow_type>
		struct CircularHash
		{
			typedef _overflow_type overflow_type;
			typedef CircularHash<overflow_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef uint32_t pos_type;
			typedef uint32_t entry_size_type;
			typedef uint32_t hash_type;
			
			overflow_type & overflow;
			
			unsigned int const tablesizelog;
			uint64_t const tablesize;
			uint64_t const tablemask;
			
			unsigned int const entrysize;
			uint64_t const bsize;
			uint64_t const bmask;
			
			// hash table
			libmaus2::autoarray::AutoArray<pos_type> H;
			// circular buffer
			libmaus2::autoarray::AutoArray<uint8_t> B;
			// rank/select/next1 data structure
			libmaus2::bitbtree::BitBTree<8,8> R;
			// current insert pointer
			pos_type ipos;
			
			libmaus2::bambam::BamAlignmentExpungeCallback * expungecallback;
			
			uint64_t hashexpunge;
			uint64_t wrapexpunge;
			
			std::ostream & printCounters(std::ostream & out) const
			{
				out << "hashexpunge=" << hashexpunge << " wrapexpunge=" << wrapexpunge << std::endl;
				return out;
			}
			
			static hash_type unused()
			{
				return std::numeric_limits<hash_type>::max();
			}
			
			static uint64_t checkBSize(uint64_t const bsize)
			{
				if ( bsize-1 <= static_cast<uint64_t>(std::numeric_limits<pos_type>::max()) )
					return bsize;
				else
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "CircularHash: table size is too big." << std::endl;
					se.finish();
					throw se;
				}
			}
			
			CircularHash(
				overflow_type & roverflow,
				unsigned int const rtablesizelog = 8,
				unsigned int const rentrysize = 220
			)
			:
			  overflow(roverflow),
			  tablesizelog(rtablesizelog),
			  tablesize(1ull << tablesizelog),
			  tablemask(tablesize-1),
			  entrysize(rentrysize),
			  bsize(checkBSize(libmaus2::math::nextTwoPow(entrysize * tablesize))),
			  bmask(bsize-1),
			  H(tablesize,false),
			  B(bsize,false),
			  R(B.size(),false),
			  ipos(0),
			  expungecallback(0),
			  hashexpunge(0),
			  wrapexpunge(0)
			{
				std::fill(H.begin(),H.end(),unused());
			}
			
			hash_type getHashForPos(pos_type const elpos) const
			{
				hash_type hv = 0;
				for ( unsigned int i = 0; i < sizeof(hash_type); ++i )
					hv |= static_cast<hash_type>(B[(elpos+i)&bmask]) << (8*i);
				return hv;	
			}

			void eraseEntry(pos_type const elpos, hash_type const hv)
			{
				R.setBitQuick(elpos,false);
				H[hv & tablemask] = unused();
			}
			
			void eraseEntry(hash_type const hv)
			{
				eraseEntry(H[hv&tablemask],hv);
			}
			
			bool overflowEntry(
				pos_type const elpos,
				hash_type const hv
			)
			{
				// assert ( getHashForPos(elpos) == hv );
				
				// position of element length
				pos_type const lenpos = (elpos + sizeof(hash_type)) & bmask;
				
				// decode length of entry
				entry_size_type len = 0;
				for ( unsigned int i = 0; i < sizeof(entry_size_type); ++i )
					len |= (static_cast<entry_size_type>(B[(lenpos+i)&bmask]) << (i*8));
				
				// position of data
				pos_type const datapos = (lenpos + sizeof(entry_size_type)) & bmask;

				if ( overflow.needFlush(len,true /* first/full */) )
					return false;
					
				assert ( ! overflow.needFlush(len,true /* first */) );

				// overflow.prepare(len);
				
				// can we write without wrap around?
				if ( datapos + len <= B.size() )
				{											
					overflow.write(B.begin()+datapos,len,true,len);

					if ( expungecallback )
						expungecallback->expunged(B.begin()+datapos,len);
				}
				else
				{
					pos_type const flen = B.size()-datapos;
					pos_type const slen = len-flen;

					overflow.write(B.begin()+datapos,flen,true,len);
					overflow.write(B.begin(),slen,false,len);

					if ( expungecallback )
						expungecallback->expunged(B.begin()+datapos,flen,B.begin(),slen);
				}
			
				// assert ( R[elpos] );	
				eraseEntry(elpos,hv);
				
				return true;
			}

			bool overflowEntry(pos_type const elpos)
			{
				// assert ( R[elpos] );
				return overflowEntry(elpos,getHashForPos(elpos));
			}
			
			bool flush()
			{
				while ( R.hasNext1() )
					if ( ! overflowEntry(R.next1(0)) )
						return false;
				
				return true;
			}
			
			uint64_t nextOneSpace(uint64_t & next1) const
			{
				// assert ( R.hasNext1() );
				next1 = R.next1(ipos);
				// assert ( R[next1] );
				
				if ( next1 < ipos )
					return (B.size()-ipos + next1);
				else
					return next1-ipos;
			}
			
			bool hasEntry(hash_type const hv) const
			{
				return H[hv&tablemask] != unused() && getHashForPos(H[hv&tablemask]) == hv;
			}
			
			uint64_t getEntryPos(hash_type const hv) const
			{
				assert ( hasEntry(hv) );
				return H[hv&tablemask];
			}
			
			std::pair<pos_type,entry_size_type> getEntry(hash_type const hv) const
			{
				pos_type const rp = getEntryPos(hv);
				
				pos_type lenpos = (rp + sizeof(hash_type)) & bmask;
				entry_size_type len = 0;
				for ( unsigned int i = 0; i < sizeof(entry_size_type); ++i, lenpos = (lenpos+1) & bmask )
					len |= static_cast<entry_size_type>(B[lenpos]) << (8*i);

				return
					std::pair<pos_type,entry_size_type>(
						(rp + sizeof(hash_type) + sizeof(entry_size_type)) & bmask,
						len );
			}
			
			uint8_t const * getEntryData(
				std::pair<pos_type,entry_size_type> const & P, libmaus2::autoarray::AutoArray<uint8_t> & T
			) const
			{
				if ( P.first + P.second <= B.size() )
					return B.begin() + P.first;
				else
				{
					if ( T.size() < P.second )
						T = libmaus2::autoarray::AutoArray<uint8_t>(P.second,false);
					
					uint64_t p = P.first & bmask;
					for ( uint64_t i = 0; i < P.second; ++i, p = (p+1)&bmask )
						T[i] = B[p];
					
					return T.begin();
				}
			}
			
			bool putEntry(
				uint8_t const * data,
				entry_size_type const & datasize,
				hash_type const hv
			)
			{
				// if we have a hash collision, then write out the entry causing the collision
				if ( H[hv&tablemask] != unused() )
				{
					// position of element
					pos_type const elpos = H[hv&tablemask];
					// remove element
					if ( ! overflowEntry(elpos,hv) )
						return false;
						
					hashexpunge += 1;
				}
				
				// assert ( H[hv&tablemask] == unused() );
				
				static entry_size_type const headersize = sizeof(hash_type) + sizeof(entry_size_type);
				
				// remove entries until we have sufficient space
				uint64_t next1;
				while ( R.hasNext1() && nextOneSpace(next1) < headersize+datasize )
				{
					// assert ( R[next1] );
					if ( ! overflowEntry(next1) )
						return false;
						
					wrapexpunge += 1;
				}
				
				pos_type const inspos = ipos;
				// assert ( inspos < B.size() );
				R.setBitQuick(inspos,true);
				H[hv&tablemask] = inspos;
				// assert ( R[inspos] );

				// write hash value
				for ( unsigned int i = 0; i < sizeof(hash_type); ++i, ipos=((ipos+1)&bmask) )
				{
					// assert ( i == 0 || ! R[ipos] );
					B[ipos] = (hv >> (8*i)) & 0xFF;
				}
				// entry length
				for ( unsigned int i = 0; i < sizeof(entry_size_type); ++i, ipos=((ipos+1)&bmask) )
				{
					// assert ( ! R[ipos] );
					B[ipos] = (datasize >> (8*i)) & 0xFF;
				}
				
				if ( ipos + datasize <= B.size() )
				{
					std::copy(data,data+datasize,B.begin()+ipos);
					ipos = (ipos+datasize)&bmask;
				}
				else
				{
					// data
					for ( unsigned int i = 0; i < datasize; ++i, ipos=((ipos+1)&bmask) )
					{
						// assert ( ! R[ipos] );
						B[ipos] = data[i];
					}
				}
				
				return true;
			}

			/**
			 * set expunge callback
			 *
			 * @param rrexpungecallback callback
			 **/
			void setExpungeCallback(libmaus2::bambam::BamAlignmentExpungeCallback * rexpungecallback)
			{
				expungecallback = rexpungecallback;
			}
		};
	}
}
#endif
