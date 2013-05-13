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
#if ! defined(LIBMAUS_UTIL_IMPCOMPACTNUMBERARRAY_HPP)
#define LIBMAUS_UTIL_IMPCOMPACTNUMBERARRAY_HPP

#include <libmaus/math/bitsPerNum.hpp>
#include <libmaus/math/Log.hpp>
#include <libmaus/bitio/CompactArray.hpp>
#include <libmaus/util/Histogram.hpp>
#include <libmaus/huffman/huffman.hpp>
#include <libmaus/util/MemTempFileContainer.hpp>
#include <libmaus/wavelet/ImpExternalWaveletGeneratorHuffman.hpp>
#include <libmaus/wavelet/ImpHuffmanWaveletTree.hpp>
#include <libmaus/util/iterator.hpp>

namespace libmaus
{
	namespace util
	{
		struct ImpCompactNumberArrayGenerator;

		struct ImpCompactNumberArray
		{
			typedef ImpCompactNumberArray this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			libmaus::wavelet::ImpHuffmanWaveletTree::unique_ptr_type IHWT;
			libmaus::autoarray::AutoArray< ::libmaus::bitio::CompactArray::unique_ptr_type > C;	
			
			typedef libmaus::util::ConstIterator<this_type,uint64_t> const_iterator;
			
			friend struct ImpCompactNumberArrayGenerator;
			
			uint64_t byteSize() const
			{
				uint64_t s =
					IHWT->byteSize() +
					C.byteSize();
					
				for ( uint64_t i = 0; i < C.size(); ++i )
					if ( C[i] )
						s += C[i]->byteSize();
					
				return s;
			}
			
			private:
			ImpCompactNumberArray() {}
			
			public:
			template<typename stream_type>
			void serialise(stream_type & out) const
			{
				IHWT->serialise(out);
				
				if ( IHWT->getN() )
					for ( int s = IHWT->enctable.minsym; s <= IHWT->enctable.maxsym; ++s )
						if ( s > 1 && IHWT->rank(s,IHWT->getN()-1) )
						{
							assert ( C[s].get() );
							C[s]->serialize(out);
						}
				
				out.flush();
			}
			
			template<typename stream_type>
			void deserialise(stream_type & in)
			{
				IHWT = UNIQUE_PTR_MOVE(libmaus::wavelet::ImpHuffmanWaveletTree::unique_ptr_type(new libmaus::wavelet::ImpHuffmanWaveletTree(in)));
				C = libmaus::autoarray::AutoArray< ::libmaus::bitio::CompactArray::unique_ptr_type >(IHWT->enctable.maxsym+1);

				if ( IHWT->getN() )
					for ( int s = IHWT->enctable.minsym; s <= IHWT->enctable.maxsym; ++s )
						if ( s > 1 && IHWT->rank(s,IHWT->getN()-1) )
						{
							C[s] = UNIQUE_PTR_MOVE(
								::libmaus::bitio::CompactArray::unique_ptr_type(
									new ::libmaus::bitio::CompactArray(in)));
						}
			}
			
			template<typename stream_type>
			static unique_ptr_type load(stream_type & in)
			{
				unique_ptr_type P = UNIQUE_PTR_MOVE(unique_ptr_type(new this_type));
				P->deserialise(in);
				return UNIQUE_PTR_MOVE(P);
			}
			
			static unique_ptr_type loadFile(std::string const & filename)
			{
				libmaus::aio::CheckedInputStream CIS(filename);
				return UNIQUE_PTR_MOVE(load(CIS));
			}
			
			uint64_t operator[](uint64_t const i) const
			{
				return get(i);
			}
			
			uint64_t size() const
			{
				return IHWT->n;
			}
			
			uint64_t get(uint64_t const i) const
			{
				std::pair<int64_t,uint64_t> const P = IHWT->inverseSelect(i);
				
				if ( P.first < 2 )
					return P.first;
				else
						return (1ull << (P.first-1)) | C[P.first]->get(P.second);			
			}
			
			const_iterator begin() const
			{
				return const_iterator(this,0);
			}
			const_iterator end() const
			{
				return const_iterator(this,IHWT->n);
			}
		};

		struct ImpCompactNumberArrayGenerator
		{
			typedef ImpCompactNumberArrayGenerator this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			private:
			std::map<int64_t,uint64_t> const probs;
			libmaus::huffman::HuffmanTreeNode::shared_ptr_type HN;
			libmaus::huffman::EncodeTable<1> E;
			libmaus::util::MemTempFileContainer MTFC;
			libmaus::wavelet::ImpExternalWaveletGeneratorHuffman IEWGH;
			ImpCompactNumberArray::unique_ptr_type ICNA;
			::libmaus::autoarray::AutoArray<uint64_t> D;

			public:
			ImpCompactNumberArrayGenerator(libmaus::util::Histogram & hist)
			: probs(hist.getByType<int64_t>()), HN(libmaus::huffman::HuffmanBase::createTree(probs)), 
			  E(HN.get()), IEWGH(HN.get(), MTFC), ICNA(new ImpCompactNumberArray), D(E.maxsym+1)
			{
				ICNA->C = libmaus::autoarray::AutoArray< ::libmaus::bitio::CompactArray::unique_ptr_type >(E.maxsym+1);

				for ( int i = E.minsym; i <= E.maxsym; ++i )
					if ( i > 1 && E.checkSymbol(i) )
					{
						uint64_t const numsyms = probs.find(i)->second;
						ICNA->C[i] = UNIQUE_PTR_MOVE(
							::libmaus::bitio::CompactArray::unique_ptr_type(
								new ::libmaus::bitio::CompactArray(numsyms,i-1)
							)
						);
					}
			}
			
			void add(uint64_t const v)
			{
				unsigned int const b = libmaus::math::bitsPerNum(v);

				IEWGH.putSymbol(b);
				if ( b > 1 )
				{
					uint64_t const m = v & libmaus::math::lowbits(b-1);
					ICNA->C[b]->set(D[b]++,m);
				}	
			}
			
			ImpCompactNumberArray::unique_ptr_type createFinal()
			{
				std::ostringstream hwtostr;
				IEWGH.createFinalStream(hwtostr);
				
				std::istringstream hwtistr(hwtostr.str());
				ICNA->IHWT = UNIQUE_PTR_MOVE(
					libmaus::wavelet::ImpHuffmanWaveletTree::unique_ptr_type(
						new libmaus::wavelet::ImpHuffmanWaveletTree(hwtistr)
					)
				);
				
				return UNIQUE_PTR_MOVE(ICNA);		
			}
			
			template<typename iterator_in>
			ImpCompactNumberArray::unique_ptr_type construct(iterator_in it_in, uint64_t const n)
			{
				for ( uint64_t i = 0; i < n; ++i )
					add(*(it_in++));
				
				return UNIQUE_PTR_MOVE(createFinal());
			}

			template<typename iterator_in>
			static ImpCompactNumberArray::unique_ptr_type constructFromArray(iterator_in it_in, uint64_t const n)
			{
				libmaus::util::Histogram hist;
				for ( iterator_in it_a = it_in; it_a != it_in+n; ++it_a )
					hist ( libmaus::math::bitsPerNum(*it_a) );

				ImpCompactNumberArrayGenerator gen(hist);
				
				return UNIQUE_PTR_MOVE(gen.construct(it_in,n));
			}
		};
	}
}
#endif
