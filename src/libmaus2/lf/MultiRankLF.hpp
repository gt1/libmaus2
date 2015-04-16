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
#if ! defined(LIBMAUS_LF_MULTIRANKLF_HPP)
#define LIBMAUS_LF_MULTIRANKLF_HPP

#include <libmaus/bitio/getBit.hpp>
#include <libmaus/math/bitsPerNum.hpp>
#include <libmaus/rank/ERank222B.hpp>

namespace libmaus
{
	namespace lf
	{
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
	}
}
#endif
