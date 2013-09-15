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

#if ! defined(LF_HPP)
#define LF_HPP

#include <memory>
#include <libmaus/rank/ERank222B.hpp>
#include <libmaus/bitio/CompactArray.hpp>

#include <libmaus/wavelet/toWaveletTreeBits.hpp>
#include <libmaus/wavelet/WaveletTree.hpp>
#include <libmaus/huffman/huffman.hpp>
#include <libmaus/math/bitsPerNum.hpp>
#include <libmaus/rank/CacheLineRank.hpp>
#include <libmaus/rank/ImpCacheLineRank.hpp>
#include <libmaus/wavelet/ExternalWaveletGenerator.hpp>
#include <libmaus/huffman/RLDecoder.hpp>
#include <libmaus/wavelet/ImpWaveletTree.hpp>
#include <libmaus/wavelet/ImpExternalWaveletGenerator.hpp>
#include <libmaus/wavelet/ImpHuffmanWaveletTree.hpp>
#include <libmaus/rl/RLIndex.hpp>

namespace libmaus
{
	namespace lf
	{
		template<typename _wt_type>
		struct LFBase
		{
			typedef _wt_type wt_type;
			typedef typename wt_type::unique_ptr_type wt_ptr_type;
			wt_ptr_type W;
			::libmaus::autoarray::AutoArray<uint64_t> D;

			uint64_t serialize(std::ostream & out)
			{
				uint64_t s = 0;
				s += W->serialize(out);
				return s;
			}
				
			static ::libmaus::autoarray::AutoArray<uint64_t> computeD(wt_type const * W)
			{
				::libmaus::autoarray::AutoArray<uint64_t> D( (1ull << W->getB()) + 1 , true );
				
				for ( uint64_t i = 0; i < (1ull << W->getB()); ++i )
					D[i] = W->rank(i, W->getN()-1);

				// output character frequencies
				#if 0
				for ( uint64_t i = 0; i < (1ull << W->getB()); ++i )
					if ( D[i] )
						std::cerr << i << " => " << D[i] << std::endl;
				#endif

				{ uint64_t c = 0; for ( uint64_t i = 0; i < ((1ull << W->getB())+1); ++i ) { uint64_t t = D[i]; D[i] = c; c += t; } }


				return D;
			}
			
			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const syms = (1ull<< W->getB());
				for ( unsigned int i = 0; i < syms; ++i )
					if ( D[syms-i-1] <= r )
						return syms-i-1;
				return 0;
			}
			
			uint64_t phi(uint64_t r) const
			{
				#if 0
				uint64_t const sym = W->sortedSymbol(r);
				#else		
				uint64_t const sym = sortedSymbol(r);
				#endif
				
				r -= D[sym];
				return W->select(sym,r);
			}
			
			uint64_t getN() const
			{
				return W->getN();
			}
			uint64_t getB() const
			{
				return W->getB();
			}
			
			uint64_t deserialize(std::istream & istr)
			{
				uint64_t s = 0;
				wt_ptr_type tW( new wt_type (istr,s) );
				W = UNIQUE_PTR_MOVE(tW);
				D = computeD(W.get());
				std::cerr << "LF: " << s << " bytes = " << s*8 << " bits" << " = " << (s+(1024*1024-1))/(1024*1024) << " mb " << std::endl;
				return s;
			}

			LFBase( std::istream & istr )
			{
				deserialize(istr);
			}

			LFBase( std::istream & istr, uint64_t & s )
			{
				s += deserialize(istr);
			}
			
			LFBase( wt_ptr_type & rW )
			: W(UNIQUE_PTR_MOVE(rW)), D(computeD(W.get()))
			{
				// std::cerr << "moved " << W.get() << std::endl;
			}
			
			LFBase( bitio::CompactArray::unique_ptr_type & ABWT )
			{
				// compute wavelet tree bits
				::libmaus::autoarray::AutoArray<uint64_t> AW = ::libmaus::wavelet::toWaveletTreeBitsParallel ( ABWT.get() );
				wt_ptr_type tW( new wt_type( AW, ABWT->n, ABWT->getB()) );
				W = UNIQUE_PTR_MOVE(tW);
				
				D = computeD(W.get());
				
				ABWT.reset(0);
			}
			LFBase( bitio::CompactArray::unique_ptr_type & ABWT, ::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type /* ahnode */ )
			{
				// compute wavelet tree bits
				::libmaus::autoarray::AutoArray<uint64_t> AW = ::libmaus::wavelet::toWaveletTreeBitsParallel ( ABWT.get() );
				wt_ptr_type tW( new wt_type ( AW, ABWT->n, ABWT->getB()) );
				W = UNIQUE_PTR_MOVE(tW);
				
				D = computeD(W.get());
				
				ABWT.reset(0);
			}
			
			uint64_t operator()(uint64_t const r) const
			{
				std::pair< typename wt_type::symbol_type,uint64_t> const is = W->inverseSelect(r);
				return D[is.first] + is.second;
			}

			uint64_t operator[](uint64_t pos) const
			{
				return (*W)[pos];
			}

			private:
			uint64_t rankm1(uint64_t const k, uint64_t const sp) const { return sp ? W->rank(k,sp-1) : 0; }
			
			public:
			uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + rankm1(k,sp); }

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = W->getN();
				
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
					sp = step(query[m-i-1],sp),
					ep = step(query[m-i-1],ep);
			}

			uint64_t zeroPosRank() const
			{
				return W->getN() ? W->select ( W->rmq(0,W->getN()), 0) : 0;

				#if 0
				// find rank of position 0 (i.e. search terminating symbol)
				uint64_t r = 0;
				uint64_t const n = W->getN();
				
				for ( uint64_t i = 0; i < n; ++i )
				{
					if ( (i & ( 32*1024*1024-1)) == 0 )
						std::cerr << "(" << (static_cast<double>(i)/n) << ")";
				
					if ( ! (*W)[i] )
					{
						r = i;
						break;
					}
				}
				return r;
				#endif
			}
		};

		struct LF : LFBase< ::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, uint64_t > >
		{
			typedef LFBase< ::libmaus::wavelet::WaveletTree< ::libmaus::rank::ERank222B, uint64_t > > base_type;
			
			typedef base_type::wt_type wt_type;
			typedef base_type::wt_ptr_type wt_ptr_type;
			typedef LF this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			LF( std::istream & istr ) : base_type ( istr ) {}
			LF( std::istream & istr, uint64_t & s ) : base_type(istr,s) {}
			LF( wt_ptr_type & rW ) : base_type(rW) {}
			LF( bitio::CompactArray::unique_ptr_type & ABWT ) : base_type(ABWT) {}
			LF( bitio::CompactArray::unique_ptr_type & ABWT, ::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type ahnode )
			: base_type(ABWT,ahnode) {}			
		};

		struct QuickLF : LFBase< ::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, uint64_t > >
		{
			typedef LFBase< ::libmaus::wavelet::QuickWaveletTree< ::libmaus::rank::ERank222B, uint64_t > > base_type;

			typedef base_type::wt_type wt_type;
			typedef base_type::wt_ptr_type wt_ptr_type;
			typedef QuickLF this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
						
			QuickLF( std::istream & istr ) : base_type ( istr ) {}
			QuickLF( std::istream & istr, uint64_t & s ) : base_type(istr,s) {}
			QuickLF( wt_ptr_type & rW ) : base_type(rW) {}
			QuickLF( bitio::CompactArray::unique_ptr_type & ABWT ) : base_type(ABWT) {}
			QuickLF( bitio::CompactArray::unique_ptr_type & ABWT, ::libmaus::util::shared_ptr < huffman::HuffmanTreeNode >::type ahnode )
			: base_type(ABWT,ahnode) {}			
		};

		struct MultiRankLF
		{
			typedef ::libmaus::rank::ERank222B rank_type;
			typedef rank_type::writer_type writer_type;
			typedef writer_type::data_type data_type;
			typedef writer_type::unique_ptr_type writer_ptr_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;			
			typedef ::libmaus::autoarray::AutoArray<data_type> bit_vector_type;
			typedef bit_vector_type::unique_ptr_type bit_vector_ptr_type;
			static uint64_t const bitsperword = 8*sizeof(data_type);
			
			typedef MultiRankLF this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			uint64_t n;
			::libmaus::autoarray::AutoArray < bit_vector_ptr_type > bit_vectors;
			::libmaus::autoarray::AutoArray < rank_ptr_type > rank_dictionaries;
			::libmaus::autoarray::AutoArray < uint64_t > D;

			uint64_t serialize(std::ostream & out) const
			{
				uint64_t s = 0;
				
				s += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,n);
				s += ::libmaus::serialize::Serialize<uint64_t>::serialize(out,bit_vectors.getN());

				for ( uint64_t i = 0; i < bit_vectors.getN(); ++i )
					s += bit_vectors[i]->serialize(out);
					
				s += D.serialize(out);

				return s;
			}

			static uint64_t deserializeNumber(std::istream & in, uint64_t & s)
			{
				uint64_t num;
				s += ::libmaus::serialize::Serialize<uint64_t>::deserialize(in,&num);
				return num;
			}
			static uint64_t deserializeNumber(std::istream & in)
			{
				uint64_t s = 0;
				return deserializeNumber(in,s);
			}

			static ::libmaus::autoarray::AutoArray < bit_vector_ptr_type > deserializeBitVectors(std::istream & in, uint64_t & s)
			{
				
				uint64_t numbitvectors = deserializeNumber(in,s);

				::libmaus::autoarray::AutoArray < bit_vector_ptr_type > bit_vectors(numbitvectors);

				for ( uint64_t i = 0; i < bit_vectors.getN(); ++i )
					s += bit_vectors[i]->deserialize(in);

				return bit_vectors;
			}
			static ::libmaus::autoarray::AutoArray < bit_vector_ptr_type > deserializeBitVectors(std::istream & in)
			{
				uint64_t s = 0;
				return deserializeBitVectors(in,s);
			}
			
			static ::libmaus::autoarray::AutoArray < uint64_t > deserializeArray(std::istream & in, uint64_t & s)
			{
				::libmaus::autoarray::AutoArray < uint64_t > A;
				s += A.deserialize(in);
				return A;
			}

			static ::libmaus::autoarray::AutoArray < uint64_t > deserializeArray(std::istream & in)
			{
				uint64_t s = 0;
				return deserializeArray(in,s);
			}
			
			MultiRankLF (std::istream & in, uint64_t & s)
			: n(deserializeNumber(in,s)), bit_vectors ( deserializeBitVectors(in,s) ), rank_dictionaries(bit_vectors.getN()), D( deserializeArray(in,s) )
			{
				for ( uint64_t j = 0; j < bit_vectors.getN(); ++j )
				{
					rank_ptr_type trank_dictionariesj (
                                                        new rank_type (
                                                                bit_vectors[j]->get(),
                                                                bit_vectors[j]->getN() * bitsperword
                                                        )
                                                );
					rank_dictionaries[j] = UNIQUE_PTR_MOVE(trank_dictionariesj);
				}
			
			}
			MultiRankLF (std::istream & in)
			: n(deserializeNumber(in)), bit_vectors ( deserializeBitVectors(in) ), rank_dictionaries(bit_vectors.getN()), D( deserializeArray(in) )
			{
				for ( uint64_t j = 0; j < bit_vectors.getN(); ++j )
				{
					rank_ptr_type trank_dictionariesj (
                                                        new rank_type (
                                                                bit_vectors[j]->get(),
                                                                bit_vectors[j]->getN() * bitsperword
                                                        )
                                                );
					rank_dictionaries[j] = UNIQUE_PTR_MOVE(trank_dictionariesj);
				}
			
			}
			
			template<typename iterator>
			MultiRankLF ( iterator BWT, uint64_t const rn, uint64_t rmaxval = 0 )
			: n(rn)
			{
				if ( n )
				{
					uint64_t maxval = rmaxval;
					for ( uint64_t i = 0; i < n; ++i )
						maxval = std::max ( maxval, static_cast<uint64_t>(BWT[i]) );
					
					bit_vectors = ::libmaus::autoarray::AutoArray < bit_vector_ptr_type >(maxval+1);
					rank_dictionaries = ::libmaus::autoarray::AutoArray < rank_ptr_type >(maxval+1);
					
					::libmaus::autoarray::AutoArray < writer_ptr_type > writers(maxval+1);
					
					for ( uint64_t i = 0; i < maxval+1; ++i )
					{
						// add one word for correct rankm1 at border
						bit_vector_ptr_type tbit_vectorsi (
                                                                new bit_vector_type(
                                                                        ( ( n + bitsperword - 1 ) / bitsperword + 1 )
                                                                )
                                                        );
						bit_vectors[i] = UNIQUE_PTR_MOVE(tbit_vectorsi);
						writer_ptr_type twritersi(
                                                        new writer_type(
                                                                bit_vectors[i]->get()
                                                        )
                                                );
						writers[i] = UNIQUE_PTR_MOVE(twritersi);
					}
					
					for ( uint64_t i = 0; i < n; ++i )
					{
						uint64_t const v = BWT[i];
						
						for ( uint64_t j = 0; j < maxval+1; ++j )
							writers[j]->writeBit ( v == j );
					}

					D = ::libmaus::autoarray::AutoArray < uint64_t > (maxval+1);

					for ( uint64_t j = 0; j < maxval+1; ++j )
					{
						writers[j]->flush();
						rank_ptr_type trank_dictionariesj (
                                                                new rank_type (
                                                                        bit_vectors[j]->get(),
                                                                        bit_vectors[j]->getN() * bitsperword
                                                                )
                                                        );
						rank_dictionaries[j] = UNIQUE_PTR_MOVE(trank_dictionariesj);
						
						D[j] = rank_dictionaries[j] -> rank1 (n - 1);
					}					

					{ uint64_t c = 0; for ( uint64_t i = 0; i < D.getN(); ++i ) { uint64_t t = D[i]; D[i] = c; c += t; } }
				}
			}
			
			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const syms = bit_vectors.getN() + 1;
				for ( unsigned int i = 0; i < syms; ++i )
					if ( D[syms-i-1] <= r )
						return syms-i-1;
				return 0;
			}
			
			uint64_t phi(uint64_t r) const
			{
				uint64_t const sym = sortedSymbol(r);
				r -= D[sym];
				return rank_dictionaries[sym]->select1(r);
			}
			
			uint64_t getN() const
			{
				return n;
			}
			
			uint64_t getB() const
			{
				if ( bit_vectors.getN() )
					return ::libmaus::math::bitsPerNum(bit_vectors.getN()-1);
				else
					return 0;
			}

			uint64_t operator[](uint64_t pos) const
			{
				for ( uint64_t i = 0; i < bit_vectors.getN(); ++i )
					if ( ::libmaus::bitio::getBit ( bit_vectors[i]->get() , pos ) )
						return i;
				
				throw std::runtime_error("Undefined position in MultiRankLF::operator[]");
			}
			
			uint64_t operator()(uint64_t const r) const
			{
				return step((*this)[r],r);
			}


			private:
			// uint64_t rankm1(uint64_t const k, uint64_t const sp) const { return sp ? rank_dictionaries[k]->rank1(sp-1) : 0; }
			inline uint64_t rankm1(uint64_t const k, uint64_t const sp) const 
			{
				#if 0
				bool ok = rank_dictionaries[k]->rankm1(sp) == (sp ? rank_dictionaries[k]->rank1(sp-1) : 0);
				if ( ! ok )
				{
					std::cerr << "Failure for k=" << k << " sp=" << sp <<
						" exptected " << (sp ? rank_dictionaries[k]->rank1(sp-1) : 0) <<
						" got " << rank_dictionaries[k]->rankm1(sp);
				}
				return (sp ? rank_dictionaries[k]->rank1(sp-1) : 0);
				#else
				return rank_dictionaries[k]->rankm1(sp);
				#endif
			}
			
			public:
			inline uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + rankm1(k,sp); }

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = n;
				
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
					sp = step(query[m-i-1],sp),
					ep = step(query[m-i-1],ep);
			}

			uint64_t zeroPosRank() const
			{
				if ( ! n )
					return 0;
					
				for ( uint64_t i = 0; i < bit_vectors.getN(); ++i )
					if ( rank_dictionaries[i]->rank1(n-1) )
						return rank_dictionaries[i]->select1(0);
						
				libmaus::exception::LibMausException se;
				se.getStream() << "MultiRankLF::zeroPosRank(): failed to find rank, inconsistent data." << std::endl;
				se.finish();
				throw se;
			}
		};

		struct MultiRankCacheLF
		{
			typedef ::libmaus::rank::CacheLineRank8 rank_type;
			typedef rank_type::WriteContext writer_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;			
			
			typedef MultiRankCacheLF this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			uint64_t n;
			::libmaus::autoarray::AutoArray < rank_ptr_type > rank_dictionaries;
			::libmaus::autoarray::AutoArray < uint64_t > D;

			template<typename iterator>
			MultiRankCacheLF ( iterator BWT, uint64_t const rn, uint64_t const rmaxval = 0)
			: n(rn)
			{
				if ( n )
				{
					uint64_t maxval = rmaxval;
					for ( uint64_t i = 0; i < n; ++i )
						maxval = std::max ( maxval, static_cast<uint64_t>(BWT[i]) );
						
					rank_dictionaries = ::libmaus::autoarray::AutoArray < rank_ptr_type >(maxval+1);
					
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
					{
						rank_ptr_type trank_dictionariesi(new rank_type(n+1));
						rank_dictionaries[i] = UNIQUE_PTR_MOVE(trank_dictionariesi);
						writer_type writer = rank_dictionaries[i]->getWriteContext();
						
						for ( uint64_t j = 0; j < n; ++j )
							writer.writeBit(BWT[j] == i);
						// write additional bit to make rankm1 defined for n
						writer.writeBit(0);
						
						writer.flush();
					}
					
					D = ::libmaus::autoarray::AutoArray < uint64_t >(rank_dictionaries.size()+1);
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
						D [ i ] = rank_dictionaries[i]->rank1(n-1);
					D.prefixSums();
				}
			}
			
			uint64_t getN() const
			{
				return n;
			}
			
			private:
			
			public:
			inline uint64_t rankm1(uint64_t const k, uint64_t const sp) const { return rank_dictionaries[k]->rankm1(sp); }
			inline uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + rankm1(k,sp); }
			
			uint64_t operator[](uint64_t const i) const
			{
				for ( uint64_t j = 0; j < rank_dictionaries.size(); ++j )
					if ( (*(rank_dictionaries[j]))[i] )
						return j;
				return rank_dictionaries.size();
			}

			uint64_t operator()(uint64_t const r) const
			{
				return step((*this)[r],r);
			}

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = n;
				
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
					sp = step(query[m-i-1],sp),
					ep = step(query[m-i-1],ep);
			}

			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const syms = rank_dictionaries.size();
				for ( unsigned int i = 0; i < syms; ++i )
					if ( D[syms-i-1] <= r )
						return syms-i-1;
				return 0;
			}
			
			uint64_t phi(uint64_t r) const
			{
				uint64_t const sym = sortedSymbol(r);
				r -= D[sym];
				return rank_dictionaries[sym]->select1(r);
			}
		};

		struct ImpCacheLineRankLF
		{
			typedef ::libmaus::rank::ImpCacheLineRank rank_type;
			typedef rank_type::WriteContext writer_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;			
			
			typedef ImpCacheLineRankLF this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			uint64_t n;
			::libmaus::autoarray::AutoArray < rank_ptr_type > rank_dictionaries;
			::libmaus::autoarray::AutoArray < uint64_t > D;

			static unique_ptr_type constructFromRL(std::string const & filename, uint64_t const maxval, uint64_t * const zcnt = 0)
			{
				::libmaus::huffman::RLDecoder decoder(std::vector<std::string>(1,filename));
				return unique_ptr_type(new this_type(decoder,maxval,zcnt));
			}

			static unique_ptr_type constructFromRL(std::vector<std::string> const & filenames, uint64_t const maxval, uint64_t * const zcnt = 0)
			{
				::libmaus::huffman::RLDecoder decoder(filenames);
				return unique_ptr_type(new this_type(decoder,maxval,zcnt));
			}

			ImpCacheLineRankLF (::libmaus::huffman::RLDecoder & decoder, uint64_t const maxval, uint64_t * const zcnt = 0)
			: n(decoder.getN())
			{
				if ( n )
				{
					rank_dictionaries = ::libmaus::autoarray::AutoArray < rank_ptr_type >(maxval+1);
					::libmaus::autoarray::AutoArray<writer_type> writers(rank_dictionaries.size());
					
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
					{
						rank_ptr_type trank_dictionariesi(new rank_type(n+1));
						rank_dictionaries[i] = UNIQUE_PTR_MOVE(trank_dictionariesi);
						writers[i] = rank_dictionaries[i]->getWriteContext();
					}
					
					for ( uint64_t i = 0; i < n; ++i )
					{
						uint64_t const sym = decoder.decode();
						
						for ( uint64_t j = 0; j < rank_dictionaries.size(); ++j )
							writers[j].writeBit(sym == j);
					}
					
					for ( uint64_t j = 0; j < rank_dictionaries.size(); ++j )
					{
						writers[j].writeBit(0);
						writers[j].flush();
					}
					
					D = ::libmaus::autoarray::AutoArray < uint64_t >(rank_dictionaries.size()+1);
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
						D [ i ] = rank_dictionaries[i]->rank1(n-1);
					D.prefixSums();
					
					if ( zcnt )
						*zcnt = D[0];
				}
			}

			template<typename iterator>
			ImpCacheLineRankLF ( iterator BWT, uint64_t const rn, uint64_t const rmaxval = 0, uint64_t * const zcnt = 0)
			: n(rn)
			{
				if ( n )
				{
					uint64_t maxval = rmaxval;
					for ( uint64_t i = 0; i < n; ++i )
						maxval = std::max ( maxval, static_cast<uint64_t>(BWT[i]) );
						
					rank_dictionaries = ::libmaus::autoarray::AutoArray < rank_ptr_type >(maxval+1);
					
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
					{
						rank_ptr_type trank_dictionariesi(new rank_type(n+1));
						rank_dictionaries[i] = UNIQUE_PTR_MOVE(trank_dictionariesi);
						writer_type writer = rank_dictionaries[i]->getWriteContext();
						
						for ( uint64_t j = 0; j < n; ++j )
							writer.writeBit(BWT[j] == i);
						// write additional bit to make rankm1 defined for n
						writer.writeBit(0);
						
						writer.flush();
					}
					
					D = ::libmaus::autoarray::AutoArray < uint64_t >(rank_dictionaries.size()+1);
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
						D [ i ] = rank_dictionaries[i]->rank1(n-1);
					D.prefixSums();
					
					if ( zcnt )
						*zcnt = 0;
				}
			}
			
			uint64_t getN() const
			{
				return n;
			}
			
			private:
			
			public:
			inline uint64_t rankm1(uint64_t const k, uint64_t const sp) const { return rank_dictionaries[k]->rankm1(sp); }
			inline uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + rankm1(k,sp); }
			
			uint64_t operator[](uint64_t const i) const
			{
				for ( uint64_t j = 0; j < rank_dictionaries.size(); ++j )
					if ( (*(rank_dictionaries[j]))[i] )
						return j;
				return rank_dictionaries.size();
			}

			uint64_t operator()(uint64_t const r) const
			{
				return step((*this)[r],r);
			}

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = n;
				
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
					sp = step(query[m-i-1],sp),
					ep = step(query[m-i-1],ep);
			}

			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const syms = rank_dictionaries.size();
				for ( unsigned int i = 0; i < syms; ++i )
					if ( D[syms-i-1] <= r )
						return syms-i-1;
				return 0;
			}
			
			uint64_t phi(uint64_t r) const
			{
				uint64_t const sym = sortedSymbol(r);
				r -= D[sym];
				return rank_dictionaries[sym]->select1(r);
			}
		};

		struct ImpCacheLineRankLFNonZero
		{
			typedef ::libmaus::rank::ImpCacheLineRank rank_type;
			typedef rank_type::WriteContext writer_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;			
			
			typedef ImpCacheLineRankLFNonZero this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			uint64_t n;
			::libmaus::autoarray::AutoArray < rank_ptr_type > rank_dictionaries;
			::libmaus::autoarray::AutoArray < uint64_t > D;

			static unique_ptr_type constructFromRL(std::string const & filename, uint64_t const maxval, uint64_t * const zcnt = 0)
			{
				::libmaus::huffman::RLDecoder decoder(std::vector<std::string>(1,filename));
				return unique_ptr_type(new this_type(decoder,maxval,zcnt));
			}

			static unique_ptr_type constructFromRL(std::vector<std::string> const & filenames, uint64_t const maxval, uint64_t * const zcnt = 0)
			{
				::libmaus::huffman::RLDecoder decoder(filenames);
				return unique_ptr_type(new this_type(decoder,maxval,zcnt));
			}

			ImpCacheLineRankLFNonZero (::libmaus::huffman::RLDecoder & decoder, uint64_t const maxval, uint64_t * const zcnt = 0)
			: n(decoder.getN())
			{
				if ( n )
				{
					rank_dictionaries = ::libmaus::autoarray::AutoArray < rank_ptr_type >(maxval);
					::libmaus::autoarray::AutoArray<writer_type> writers(rank_dictionaries.size());
					
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
					{
						rank_ptr_type trank_dictionariesi(new rank_type(n+1));
						rank_dictionaries[i] = UNIQUE_PTR_MOVE(trank_dictionariesi);
						writers[i] = rank_dictionaries[i]->getWriteContext();
					}
					
					uint64_t zshift = 0;
					for ( uint64_t i = 0; i < n; ++i )
					{
						uint64_t const sym = decoder.decode();
						
						for ( uint64_t j = 0; j < rank_dictionaries.size(); ++j )
							writers[j].writeBit(sym && ((sym-1) == j));
						
						if ( ! sym )
							zshift++;
					}
					
					for ( uint64_t j = 0; j < rank_dictionaries.size(); ++j )
					{
						writers[j].writeBit(0);
						writers[j].flush();
					}
					
					D = ::libmaus::autoarray::AutoArray < uint64_t >(rank_dictionaries.size()+1);
					for ( uint64_t i = 0; i < rank_dictionaries.size(); ++i )
						D [ i ] = rank_dictionaries[i]->rank1(n-1);
					D.prefixSums();
					
					for ( uint64_t i = 0; i < D.size(); ++i )
						D[i] += zshift;
						
					if ( zcnt )
						*zcnt = zshift;
				}
			}

			uint64_t getN() const
			{
				return n;
			}
			
			public:
			inline uint64_t rankm1(uint64_t const k, uint64_t const sp) const
			{
				return rank_dictionaries[k-1]->rankm1(sp);
			}
			inline uint64_t step(uint64_t const k, uint64_t const sp) const 
			{
				assert ( k );
				return D[k-1] + rankm1(k,sp); 
			}
		};

		struct ImpWTLF
		{
			typedef ::libmaus::wavelet::ImpWaveletTree wt_type;
			typedef wt_type::unique_ptr_type wt_ptr_type;
			
			typedef ImpWTLF this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			uint64_t n;
			wt_ptr_type W;
			::libmaus::autoarray::AutoArray < uint64_t > D;
			
			void serialise(std::ostream & out)
			{
				::libmaus::util::NumberSerialisation::serialiseNumber(out,n);
				W->serialise(out);
			}
			
			ImpWTLF(std::istream & in)
			: n(::libmaus::util::NumberSerialisation::deserialiseNumber(in)),
			  W (new wt_type(in))
			{				
				if ( n )
				{
					D = ::libmaus::autoarray::AutoArray < uint64_t >((1ull<<W->getB())+1);
					for ( uint64_t i = 0; i < (1ull<<W->getB()); ++i )
						D [ i ] = W->rank(i,n-1);
					D.prefixSums();
				}
			}

			template<typename iterator>
			ImpWTLF ( iterator BWT, uint64_t const rn, ::libmaus::util::TempFileNameGenerator & rtmpgen, uint64_t const rmaxval = 0)
			: n(rn)
			{
				if ( n )
				{	
					uint64_t maxval = rmaxval;
					for ( uint64_t i = 0; i < n; ++i )
						maxval = std::max ( maxval, static_cast<uint64_t>(BWT[i]) );
					uint64_t const b = ::libmaus::math::numbits(maxval);

					::libmaus::wavelet::ImpExternalWaveletGenerator IEWG(b,rtmpgen);
					for ( uint64_t i = 0; i < n; ++i )
						IEWG.putSymbol(BWT[i]);
					std::string const tmpfilename = rtmpgen.getFileName();
					IEWG.createFinalStream(tmpfilename);
					
					std::ifstream istr(tmpfilename.c_str(),std::ios::binary);
					wt_ptr_type tW(new wt_type(istr));
					W = UNIQUE_PTR_MOVE(tW);
					istr.close();
					remove ( tmpfilename.c_str() );
					
					D = ::libmaus::autoarray::AutoArray < uint64_t >((1ull<<W->getB())+1);
					for ( uint64_t i = 0; i < (1ull<<W->getB()); ++i )
						D [ i ] = W->rank(i,n-1);
					D.prefixSums();
				}
			}

			static unique_ptr_type constructFromRL(std::string const & filename, uint64_t const maxval, std::string const & tmpprefix )
			{
				::libmaus::util::TempFileNameGenerator tmpgen(tmpprefix,2);
				::libmaus::huffman::RLDecoder decoder(std::vector<std::string>(1,filename));
				return unique_ptr_type(new this_type(decoder,::libmaus::math::numbits(maxval),tmpgen));
			}

			static unique_ptr_type constructFromRL(std::vector<std::string> const & filenames, uint64_t const maxval, std::string const & tmpprefix )
			{
				::libmaus::util::TempFileNameGenerator tmpgen(tmpprefix,2);
				::libmaus::huffman::RLDecoder decoder(filenames);
				return unique_ptr_type(new this_type(decoder,::libmaus::math::numbits(maxval),tmpgen));
			}

			ImpWTLF (::libmaus::huffman::RLDecoder & decoder, uint64_t const b, ::libmaus::util::TempFileNameGenerator & rtmpgen)
			: n(decoder.getN())
			{
				if ( n )
				{	
					::libmaus::wavelet::ImpExternalWaveletGenerator IEWG(b,rtmpgen);
					for ( uint64_t i = 0; i < n; ++i )
						IEWG.putSymbol(decoder.decode());
					std::string const tmpfilename = rtmpgen.getFileName();
					IEWG.createFinalStream(tmpfilename);
					
					std::ifstream istr(tmpfilename.c_str(),std::ios::binary);
					wt_ptr_type tW(new wt_type(istr));
					W = UNIQUE_PTR_MOVE(tW);
					istr.close();
					remove ( tmpfilename.c_str() );
					
					D = ::libmaus::autoarray::AutoArray < uint64_t >((1ull<<W->getB())+1);
					for ( uint64_t i = 0; i < (1ull<<W->getB()); ++i )
						D [ i ] = W->rank(i,n-1);
					D.prefixSums();
				}
			}
			
			uint64_t getN() const
			{
				return n;
			}
			
			private:
			
			public:
			inline uint64_t rankm1(uint64_t const k, uint64_t const sp) const { return W->rankm1(k,sp); }
			inline uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + rankm1(k,sp); }
			
			uint64_t operator[](uint64_t const i) const
			{
				return (*W)[i];
			}

			uint64_t operator()(uint64_t const r) const
			{
				std::pair<uint64_t,uint64_t> const P = W->inverseSelect(r);
				return D[P.first] + P.second;
			}

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = n;
				
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
					sp = step(query[m-i-1],sp),
					ep = step(query[m-i-1],ep);
			}

			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const syms = (1ull<<W->getB());
				for ( unsigned int i = 0; i < syms; ++i )
					if ( D[syms-i-1] <= r )
						return syms-i-1;
				return 0;
			}
			
			uint64_t phi(uint64_t r) const
			{
				uint64_t const sym = sortedSymbol(r);
				r -= D[sym];
				return W->select(sym,r);
			}
		};

		struct ImpHuffmanWaveletLF
		{
			typedef ImpHuffmanWaveletLF this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef ::libmaus::wavelet::ImpHuffmanWaveletTree wt_type;
			typedef wt_type::unique_ptr_type wt_ptr_type;
		
			wt_ptr_type const W;
			uint64_t const n;
			uint64_t const n0;
			::libmaus::autoarray::AutoArray<uint64_t> D;
			
			uint64_t byteSize() const
			{
				return 
					W->byteSize()+
					2*sizeof(uint64_t)+
					D.byteSize();
			}
			
			uint64_t getN() const
			{
				return n;
			}
			
			::libmaus::autoarray::AutoArray<int64_t> getSymbols() const
			{
				::libmaus::autoarray::AutoArray<int64_t> symbols = W->sroot->symbolArray();
				std::sort(symbols.begin(),symbols.end());
				return symbols;
			}
			
			uint64_t getSymbolThres() const
			{
				::libmaus::autoarray::AutoArray<int64_t> const syms = getSymbols();
				if ( syms.size() )
					return syms[syms.size()-1]+1;
				else
					return 0;
			}

			::libmaus::autoarray::AutoArray<uint64_t> computeD() const
			{
				::libmaus::autoarray::AutoArray<int64_t> symbols = W->sroot->symbolArray();
				int64_t maxsym = symbols.size() ? symbols[0] : -1;
				int64_t minsym = maxsym;
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					maxsym = std::max(maxsym,symbols[i]);
					minsym = std::min(minsym,symbols[i]);
				}
				#if 0
				std::cerr << "minsym: " << maxsym << std::endl;
				std::cerr << "maxsym: " << maxsym << std::endl;
				#endif
				
				assert ( minsym >= 0 );
				
				::libmaus::autoarray::AutoArray<uint64_t> D(maxsym+1);
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					int64_t const sym = symbols[i];
					D [ sym ] = n ? W->rank(sym,n-1) : 0;
					#if 0
					std::cerr << "D[" << sym << "]=" << D[sym] << std::endl;
					#endif
				}
				D.prefixSums();	

				return D;		
			}
			
			static unique_ptr_type loadSequential(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				if ( ! istr.is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ImpHuffmanWaveletLF::load() failed to open file " << filename << std::endl;
					se.finish();
					throw se;
				}
				
				unique_ptr_type ptr ( new ImpHuffmanWaveletLF ( istr ) );
				
				if ( ! istr )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "ImpHuffmanWaveletLF::load() failed to read file " << filename << std::endl;
					se.finish();
					throw se;					
				}
				
				return UNIQUE_PTR_MOVE(ptr);
			}

			static unique_ptr_type load(std::string const & filename)
			{
				unique_ptr_type ptr(new this_type(filename));
				return UNIQUE_PTR_MOVE(ptr);
			}

			ImpHuffmanWaveletLF(std::istream & in)
			: W(new wt_type(in)), n(W->n), n0((n && W->haveSymbol(0)) ? W->rank(0,n-1) : 0), D(computeD())
			{
			}

			ImpHuffmanWaveletLF(std::string const & filename)
			: W(wt_type::load(filename)), n(W->n), n0((n && W->haveSymbol(0)) ? W->rank(0,n-1) : 0), D(computeD())
			{
			}
			
			uint64_t operator()(uint64_t const r) const
			{
				std::pair< int64_t,uint64_t> const is = W->inverseSelect(r);
				return D[is.first] + is.second;
			}

			uint64_t operator[](uint64_t pos) const
			{
				return (*W)[pos];
			}

			std::pair<int64_t,uint64_t> extendedLF(uint64_t const r) const
			{
				std::pair< int64_t,uint64_t> const is = W->inverseSelect(r);
				return std::pair<int64_t,uint64_t>(is.first, D[is.first] + is.second);
			}

			public:
			uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + W->rankm(k,sp); }
			std::pair<uint64_t,uint64_t> step(uint64_t const k, uint64_t const sp, uint64_t const ep) const 
			{
				return W->rankm(k,sp,ep,D.get());
			}
			std::pair<uint64_t,uint64_t> step(uint64_t const k, std::pair<uint64_t,uint64_t> const & P) const 
			{
				return W->rankm(k,P.first,P.second,D.get());
			}

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = n;
						
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
				{
					int64_t const sym = query[m-i-1];
					std::pair<uint64_t,uint64_t> const P = step(sym,sp,ep);
					sp = P.first;
					ep = P.second;
				}
			}
			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, std::pair<uint64_t,uint64_t> & P) const
			{
				P = std::pair<uint64_t,uint64_t>(0,n);
						
				for ( uint64_t i = 0; i < m && P.first != P.second; ++i )
				{
					int64_t const sym = query[m-i-1];
					P = step(sym,P);
				}
			}

			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const syms = D.size();
				for ( unsigned int i = 0; i < syms; ++i )
					if ( D[syms-i-1] <= r )
						return syms-i-1;
				return 0;
			}
			
			uint64_t phi(uint64_t r) const
			{
				uint64_t const sym = sortedSymbol(r);
				r -= D[sym];
				return W->select(sym,r);
			}
		};

		template<typename _rlindex_type>
		struct RLIndexLFTemplate
		{
			typedef _rlindex_type rlindex_type;
			typedef RLIndexLFTemplate<rlindex_type> this_type;
			typedef typename ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typedef rlindex_type wt_type;
			typedef typename wt_type::unique_ptr_type wt_ptr_type;
		
			wt_ptr_type const W;
			uint64_t const n;
			uint64_t const n0;
			::libmaus::autoarray::AutoArray<uint64_t> D;
			
			uint64_t getN() const
			{
				return n;
			}
			
			::libmaus::autoarray::AutoArray<int64_t> getSymbols() const
			{
				::libmaus::autoarray::AutoArray<int64_t> symbols = W->symbolArray();
				std::sort(symbols.begin(),symbols.end());
				return symbols;
			}
			
			uint64_t getSymbolThres() const
			{
				::libmaus::autoarray::AutoArray<int64_t> const syms = getSymbols();
				if ( syms.size() )
					return syms[syms.size()-1]+1;
				else
					return 0;
			}

			::libmaus::autoarray::AutoArray<uint64_t> computeD() const
			{
				::libmaus::autoarray::AutoArray<int64_t> symbols = W->symbolArray();
				int64_t maxsym = symbols.size() ? symbols[0] : -1;
				int64_t minsym = maxsym;
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					maxsym = std::max(maxsym,symbols[i]);
					minsym = std::min(minsym,symbols[i]);
				}
				#if 0
				std::cerr << "minsym: " << maxsym << std::endl;
				std::cerr << "maxsym: " << maxsym << std::endl;
				#endif
				
				assert ( minsym >= 0 );
				
				::libmaus::autoarray::AutoArray<uint64_t> D(maxsym+1);
				for ( uint64_t i = 0; i < symbols.size(); ++i )
				{
					int64_t const sym = symbols[i];
					D [ sym ] = n ? W->rank(sym,n-1) : 0;
					#if 0
					std::cerr << "D[" << sym << "]=" << D[sym] << std::endl;
					#endif
				}
				D.prefixSums();	

				return D;		
			}
			
			static unique_ptr_type loadSequential(std::string const & filename)
			{
				std::ifstream istr(filename.c_str(),std::ios::binary);
				if ( ! istr.is_open() )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "RLIndexLFTemplate::load() failed to open file " << filename << std::endl;
					se.finish();
					throw se;
				}
				
				unique_ptr_type ptr ( new this_type ( istr ) );
				
				if ( ! istr )
				{
					::libmaus::exception::LibMausException se;
					se.getStream() << "RLIndexLFTemplate::load() failed to read file " << filename << std::endl;
					se.finish();
					throw se;					
				}
				
				return UNIQUE_PTR_MOVE(ptr);
			}

			static unique_ptr_type load(std::string const & filename)
			{
				return UNIQUE_PTR_MOVE(unique_ptr_type(new this_type(filename)));
			}

			RLIndexLFTemplate(std::istream & in)
			: W(new wt_type(in)), n(W->n), n0(n ? W->rank(0,n-1) : 0), D(computeD())
			{
			}

			RLIndexLFTemplate(std::string const & filename)
			: W(wt_type::load(filename)), n(W->n), n0(n ? W->rank(0,n-1) : 0), D(computeD())
			{
			}
			
			uint64_t operator()(uint64_t const r) const
			{
				std::pair< int64_t,uint64_t> const is = W->inverseSelect(r);
				return D[is.first] + is.second;
			}

			uint64_t operator[](uint64_t pos) const
			{
				return (*W)[pos];
			}

			std::pair<int64_t,uint64_t> extendedLF(uint64_t const r) const
			{
				std::pair< int64_t,uint64_t> const is = W->inverseSelect(r);
				return std::pair<int64_t,uint64_t>(is.first, D[is.first] + is.second);
			}

			public:
			uint64_t step(uint64_t const k, uint64_t const sp) const { return D[k] + W->rankm(k,sp); }
			std::pair<uint64_t,uint64_t> step(uint64_t const k, uint64_t const sp, uint64_t const ep) const 
			{
				return W->rankm(k,sp,ep,D.get());
			}
			std::pair<uint64_t,uint64_t> step(uint64_t const k, std::pair<uint64_t,uint64_t> const & P) const 
			{
				return W->rankm(k,P.first,P.second,D.get());
			}

			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, uint64_t & sp, uint64_t & ep) const
			{
				sp = 0, ep = n;
						
				for ( uint64_t i = 0; i < m && sp != ep; ++i )
				{
					int64_t const sym = query[m-i-1];
					std::pair<uint64_t,uint64_t> const P = step(sym,sp,ep);
					sp = P.first;
					ep = P.second;
				}
			}
			template<typename iterator>	
			inline void search(iterator query, uint64_t const m, std::pair<uint64_t,uint64_t> & P) const
			{
				P = std::pair<uint64_t,uint64_t>(0,n);
						
				for ( uint64_t i = 0; i < m && P.first != P.second; ++i )
				{
					int64_t const sym = query[m-i-1];
					P = step(sym,P);
				}
			}

			uint64_t sortedSymbol(uint64_t r) const
			{
				uint64_t const syms = D.size();
				for ( unsigned int i = 0; i < syms; ++i )
					if ( D[syms-i-1] <= r )
						return syms-i-1;
				return 0;
			}
			
			uint64_t phi(uint64_t r) const
			{
				uint64_t const sym = sortedSymbol(r);
				r -= D[sym];
				return W->select(sym,r);
			}
		};

		typedef RLIndexLFTemplate< ::libmaus::rl::RLIndex > RLIndexLF;
		typedef RLIndexLFTemplate< ::libmaus::rl::RLSimpleIndex > RLSimpleIndexLF;
	}
}
#endif
