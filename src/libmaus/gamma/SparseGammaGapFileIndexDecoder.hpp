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
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPFILEINDEXDECODER_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPFILEINDEXDECODER_HPP

#include <libmaus/aio/CheckedInputStream.hpp>
#include <libmaus/gamma/SparseGammaGapFileIndexDecoderEntry.hpp>
#include <libmaus/util/GenericIntervalTree.hpp>
#include <libmaus/util/iterator.hpp>
#include <libmaus/util/NumberSerialisation.hpp>
#include <libmaus/parallel/OMPLock.hpp>

namespace libmaus
{
	namespace gamma
	{
		struct SparseGammaGapFileIndexDecoder;
		std::ostream & operator<<(std::ostream & out, SparseGammaGapFileIndexDecoder const & o);

		struct SparseGammaGapFileIndexDecoder
		{
			typedef SparseGammaGapFileIndexDecoder this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::gamma::SparseGammaGapFileIndexDecoderEntry value_type;

			typedef libmaus::util::ConstIterator<this_type,value_type> const_iterator;

			private:
			libmaus::aio::CheckedInputStream::unique_ptr_type CIS;
			std::istream & in;
			uint64_t maxkey;
			uint64_t numentries;
			mutable libmaus::parallel::OMPLock lock;
			
			void init()
			{
				in.clear();
				in.seekg(-2*static_cast<int64_t>(sizeof(uint64_t)),std::ios::end);
				maxkey = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				numentries = libmaus::util::NumberSerialisation::deserialiseNumber(in);
			}

			const_iterator begin() const
			{
				return const_iterator(this,0);
			}

			const_iterator end() const
			{
				return const_iterator(this,numentries);
			}

			public:
			SparseGammaGapFileIndexDecoder(std::istream & rin)
			: in(rin)
			{	
				init();
			}
			
			SparseGammaGapFileIndexDecoder(std::string const & filename)
			: CIS(new libmaus::aio::CheckedInputStream(filename)), in(*CIS)
			{
				init();
			}	
			
			value_type get(uint64_t const i) const
			{
				if ( i >= numentries )
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "SparseGammaGapFileIndexDecoder::get() out of range: " << i << " >= " << numentries << std::endl;
					se.finish();
					throw se;
				}
			
				libmaus::parallel::ScopeLock slock(lock);
				in.clear();
				in.seekg(-2*sizeof(uint64_t)-2*numentries*sizeof(uint64_t) + i*(2*sizeof(uint64_t)), std::ios::end);
				uint64_t const ikey = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				uint64_t const ibitoff = libmaus::util::NumberSerialisation::deserialiseNumber(in);
				return value_type(ikey,ibitoff);
			}
			
			value_type operator[](uint64_t const i) const
			{
				return get(i);
			}
			
			bool isEmpty() const
			{
				return numentries == 0;
			}
			
			uint64_t getMinKey() const
			{
				if ( isEmpty() )
				{
					libmaus::exception::LibMausException ex;
					ex.getStream() << "SparseGammaGapFileIndexDecoder::getMinKey(): index is empty" << std::endl;
					ex.finish();
					throw ex;
				}
				
				return get(0).ikey;
			}
			
			uint64_t getMaxKey() const
			{
				return maxkey;
			}
			
			uint64_t getBlockIndex(uint64_t const ikey) const
			{
				// std::cerr << "getBlockIndex(" << ikey << ")" << std::endl;
			
				if ( ! numentries )
					return numentries;
				else if ( ikey < getMinKey() )
					return 0;
				else if ( ikey > getMaxKey() )
					return numentries-1;
							
				const_iterator ita = std::lower_bound(begin(),end(),value_type(ikey));
				
				uint64_t const index = ita - begin();
				
				if ( (ita != end()) && (*ita).ikey == ikey )
				{
					return index;
				}
				else
				{
					assert ( index > 0 );
					return index-1;
				}
			}
			
			uint64_t size() const
			{
				return numentries;
			}
	
			friend std::ostream & operator<<(std::ostream & out, SparseGammaGapFileIndexDecoder const & o);
		};
		
		inline std::ostream & operator<<(std::ostream & out, SparseGammaGapFileIndexDecoder const & O)
		{
			for ( SparseGammaGapFileIndexDecoder::const_iterator ita = O.begin(); ita != O.end(); ++ita )
				out << *ita << std::endl;

			return out;
		}
	}
}
#endif
