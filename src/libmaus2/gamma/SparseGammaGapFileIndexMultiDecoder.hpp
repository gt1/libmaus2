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
#if ! defined(LIBMAUS_GAMMA_SPARSEGAMMAGAPFILEINDEXMULTIDECODER_HPP)
#define LIBMAUS_GAMMA_SPARSEGAMMAGAPFILEINDEXMULTIDECODER_HPP

#include <libmaus2/gamma/SparseGammaGapFileIndexDecoder.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct SparseGammaGapFileIndexMultiDecoder
		{
			typedef SparseGammaGapFileIndexMultiDecoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef SparseGammaGapFileIndexDecoder single_decoder_type;
			typedef single_decoder_type::value_type value_type;
			typedef single_decoder_type::unique_ptr_type single_decoder_ptr_type;

			private:
			std::vector<std::string> filenames;
			libmaus2::autoarray::AutoArray<single_decoder_ptr_type> decs;
			
			libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > H;
			libmaus2::util::IntervalTree::unique_ptr_type I;
			libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > BC;
			libmaus2::util::IntervalTree::unique_ptr_type B;
			uint64_t numentries;

			static libmaus2::autoarray::AutoArray<single_decoder_ptr_type> openDecoders(
				std::vector<std::string> & filenames
			)
			{
				libmaus2::autoarray::AutoArray<single_decoder_ptr_type> decs(filenames.size());
				uint64_t numdecs = 0;
				
				for ( uint64_t i = 0; i < decs.size(); ++i )
				{
					single_decoder_ptr_type Tdec(new single_decoder_type(filenames[i]));
					
					if ( ! Tdec->isEmpty() )
					{
						filenames[numdecs] = filenames[i];
						decs[numdecs] = UNIQUE_PTR_MOVE(Tdec);
						numdecs += 1;
					}
				}
				
				if ( numdecs != filenames.size() )
				{
					while ( filenames.size() > numdecs )
						filenames.pop_back();
						
					libmaus2::autoarray::AutoArray<single_decoder_ptr_type> tdecs(numdecs);
					for ( uint64_t i = 0; i < numdecs; ++i )
						tdecs[i] = UNIQUE_PTR_MOVE(decs[i]);
						
					decs = tdecs;
				}

				return decs;
			}

			public:
			typedef libmaus2::util::ConstIterator<this_type,value_type> const_iterator;

			const_iterator begin() const
			{
				return const_iterator(this,0);
			}

			const_iterator end() const
			{
				return const_iterator(this,numentries);
			}
			
			std::vector<std::string> const & getFileNames() const
			{
				return filenames;
			}
									
			SparseGammaGapFileIndexMultiDecoder(std::vector<std::string> const & rfilenames) 
			: filenames(rfilenames), decs(openDecoders(filenames)), H(filenames.size()), BC(filenames.size())
			{
				if ( BC.size() )
					BC[0].first = 0;
					
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					H[i].first = decs[i]->getMinKey();
					
					if ( i > 0 )
						BC[i].first = BC[i-1].second;

					BC[i].second = BC[i].first + decs[i]->size();
				}
				for ( uint64_t i = 0; i+1 < filenames.size(); ++i )
				{
					H[i].second = H[i+1].first;
				}
				if ( filenames.size() )
				{
					H[0].first           = 0;
					H[H.size()-1].second = std::numeric_limits<uint64_t>::max();
				}
				
				if ( H.size() )
				{
					libmaus2::util::IntervalTree::unique_ptr_type tI(new libmaus2::util::IntervalTree(H,0,H.size()));
					I = UNIQUE_PTR_MOVE(tI);
				}

				if ( BC.size() )
				{
					libmaus2::util::IntervalTree::unique_ptr_type tB(new libmaus2::util::IntervalTree(BC,0,BC.size()));
					B = UNIQUE_PTR_MOVE(tB);
				}
				
				numentries = BC.size() ? BC[BC.size()-1].second : 0;
			}
			
			single_decoder_type & getSingleDecoder(uint64_t const i)
			{
				if ( i >= decs.size() )
				{
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "SparseGammaGapFileIndexMultiDecoder::getSingleDecoder(): index out of range" << std::endl;
					ex.finish();
					throw ex;
				}
				
				return *(decs[i]);
			}
			
			std::pair<uint64_t,uint64_t> getBlockIndex(uint64_t const ikey)
			{
				if ( ! H.size() )
				{
					return std::pair<uint64_t,uint64_t>(0,0);
				}
				else if ( ikey >= H[H.size()-1].second )
				{
					return std::pair<uint64_t,uint64_t>(H.size(),0);
				}
				else
				{
					uint64_t const findex = I->find(ikey);
					assert ( ikey >= H[findex].first );
					assert ( ikey  < H[findex].second );
					return std::pair<uint64_t,uint64_t>(findex,decs[findex]->getBlockIndex(ikey));
				}
			}
			
			value_type get(uint64_t const i) const
			{
				if ( i >= numentries )
				{
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "SparseGammaGapFileIndexMultiDecoder::get(): index out of range" << std::endl;
					ex.finish();
					throw ex;
				}
				
				uint64_t const fileptr = B->find(i);
				uint64_t const blockptr = i - BC[fileptr].first;
				
				return decs[fileptr]->get(blockptr);
			}
			
			value_type get(uint64_t const fileptr, uint64_t const blockptr) const
			{
				if ( fileptr >= decs.size() || blockptr >= decs[fileptr]->size() )
				{				
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "SparseGammaGapFileIndexMultiDecoder::get(): index out of range" << std::endl;
					ex.finish();
					throw ex;
				}
				
				return decs[fileptr]->get(blockptr);
			}
			
			bool hasPrevKey(uint64_t const ikey) const
			{
				if ( ! numentries )
					return false;
				else
				{
					uint64_t const minkey = get(0).ikey;
					
					if ( ikey <= minkey )
						return false;
					else
						return true;
				}
			}
			
			bool isEmpty() const
			{
				return numentries == 0;
			}
			
			bool hasMaxKey() const
			{
				return numentries;
			}
			
			uint64_t getMaxKey() const
			{
				assert ( hasMaxKey() );
				return decs[decs.size()-1]->getMaxKey();
			}
			
			uint64_t getMinKey() const
			{
				assert ( !isEmpty() );
				return get(0).ikey;
			}
						
			bool hasKeysLargerEqual(uint64_t const ikey) const
			{
				return hasMaxKey() && (ikey <= getMaxKey());
			}
			
			struct SparseGammaGapFileIndexMultiDecoderBlockCountAccessor
			{
				SparseGammaGapFileIndexMultiDecoder const * owner;
				
				SparseGammaGapFileIndexMultiDecoderBlockCountAccessor(SparseGammaGapFileIndexMultiDecoder const * rowner) : owner(rowner) {}
				
				uint64_t get(uint64_t const ikey) const
				{
					SparseGammaGapFileIndexMultiDecoder::const_iterator itc =
						std::lower_bound(
							owner->begin(),
							owner->end(),
							ikey	
					);
					
					return itc - owner->begin();
				}			
			};
			
			SparseGammaGapFileIndexMultiDecoderBlockCountAccessor getBlockCountAccessor() const
			{
				return SparseGammaGapFileIndexMultiDecoderBlockCountAccessor(this);
			}
			
			struct SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined
			{
				typedef SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined this_type;
				
				SparseGammaGapFileIndexMultiDecoderBlockCountAccessor accessora;
				SparseGammaGapFileIndexMultiDecoderBlockCountAccessor accessorb;
				uint64_t range;

				typedef libmaus2::util::ConstIterator<this_type,value_type> const_iterator;

				const_iterator begin() const
				{
					return const_iterator(this,0);
				}

				const_iterator end() const
				{
					return const_iterator(this,range);
				}
				
				uint64_t totalBlocks() const
				{
					return
						accessora.owner->numentries + 
						accessorb.owner->numentries;
				}
				
				SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined(
					SparseGammaGapFileIndexMultiDecoderBlockCountAccessor raccessora,
					SparseGammaGapFileIndexMultiDecoderBlockCountAccessor raccessorb,
					uint64_t const rrange
				) : accessora(raccessora), accessorb(raccessorb), range(rrange)
				{
				
				}

				SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined(
					SparseGammaGapFileIndexMultiDecoder const & rA,
					SparseGammaGapFileIndexMultiDecoder const & rB,
					uint64_t const rrange
				) : accessora(rA.getBlockCountAccessor()), accessorb(rB.getBlockCountAccessor()), range(rrange)
				{
				
				}
								
				uint64_t get(uint64_t const ikey) const
				{
					return accessora.get(ikey) + accessorb.get(ikey);
				}
			};
			
			static SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined getCombinedAccessor(
				SparseGammaGapFileIndexMultiDecoder const & A,
				SparseGammaGapFileIndexMultiDecoder const & B,
				uint64_t const range)
			{
				return SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined(A,B,range);
			}
			
			private:
			static std::vector<uint64_t> getSplitKeysInternal(
				SparseGammaGapFileIndexMultiDecoder const & A,
				SparseGammaGapFileIndexMultiDecoder const & B,
				uint64_t const range,
				uint64_t const numkeys
			)
			{
				SparseGammaGapFileIndexMultiDecoderBlockCountAccessorCombined CA = 
					getCombinedAccessor(A,B,range);
										
				uint64_t const totalblocks = CA.totalBlocks();
				uint64_t const blocksperkey = (totalblocks + numkeys-1)/numkeys;
				std::vector<uint64_t> keys;
				
				for ( uint64_t i = 0; i < numkeys; ++i )
				{
					uint64_t const tblock = std::min(i * blocksperkey, totalblocks);
					uint64_t const ikey = std::lower_bound(CA.begin(),CA.end(),tblock) - CA.begin();
					if ( (! keys.size()) || (ikey != keys.back()) )
						keys.push_back(ikey);
				}
				
				return keys;
			}

			public:
			static std::vector<uint64_t> getSplitKeys(
				this_type & indexa,
				this_type & indexb,
				uint64_t const numkeys,
				uint64_t & rmaxv
			)
			{			
				// is a empty?
				bool const aempty = indexa.isEmpty();
				// is b empty?
				bool const bempty = indexb.isEmpty();				
				// maximum key in a (or 0 for none)
				uint64_t const maxa = aempty ? 0 : indexa.getMaxKey();
				// maximum key in b (or 0 for none)
				uint64_t const maxb = bempty ? 0 : indexb.getMaxKey();
				// maximum key over both input arrays
				uint64_t const maxv = std::max(maxa,maxb);
				// copy maximum value
				rmaxv = maxv;

				return getSplitKeysInternal(indexa,indexb,maxv+1,numkeys);
			}

			static std::vector<uint64_t> getSplitKeys(
				std::vector<std::string> const & fna,
				std::vector<std::string> const & fnb,
				uint64_t const numkeys,
				uint64_t & rmaxv
			)
			{
				this_type indexa(fna);
				this_type indexb(fnb);
				return getSplitKeys(indexa,indexb,numkeys,rmaxv);
			}

			// khigh is exclusive
			bool hasKeyInRange(uint64_t const klow, uint64_t const khigh)
			{
				return !(isEmpty() || (getMinKey() >= khigh) || (getMaxKey() < klow));
			}
			
			bool hasPrevKey(uint64_t const ikey)
			{
				return (!isEmpty()) && getMinKey() < ikey;
			}
		};	
	}
}
#endif
