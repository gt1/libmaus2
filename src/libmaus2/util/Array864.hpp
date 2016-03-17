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
#if ! defined(LIBMAUS2_UTIL_ARRAY864)
#define LIBMAUS2_UTIL_ARRAY864

#include <libmaus2/rank/ERank222B.hpp>
#include <libmaus2/bitio/getBit.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/util/iterator.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>

namespace libmaus2
{
	namespace util
	{
		struct Array864Generator;

		/**
		 * array class storing a sequence of variable length numbers such that
		 * - numbers  < 256 are stored using 8 bits
		 * - numbers >= 256 are stored using 64 bits
		 * - a bit vector stores for each number in which category it is
		 * - a rank dictionary is used for category lookups
		 **/
		struct Array864
		{
			//! this type
			typedef Array864 this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			//! rank dictionary type
			typedef ::libmaus2::rank::ERank222B rank_type;
			//! writer type for bit vector used in rank dictionary
			typedef rank_type::writer_type writer_type;
			//! data type used for bit vector in rank dictionary
			typedef writer_type::data_type data_type;
			//! generator type for this array type
			typedef Array864Generator generator_type;
			//! const iterator type
			typedef libmaus2::util::ConstIterator<this_type,uint64_t> const_iterator;

			//! length of stored sequence
			uint64_t n;
			//! category bit vector
			::libmaus2::autoarray::AutoArray<data_type> B;
			//! rank dictionary
			::libmaus2::rank::ERank222B::unique_ptr_type R;
			//! subsequence stored using 8 bits per number
			::libmaus2::autoarray::AutoArray<uint8_t> A8;
			//! subsequence stored using 64 bits per number
			::libmaus2::autoarray::AutoArray<uint64_t> A64;

			/**
			 * @return estimated size of object in bytes
			 **/
			uint64_t byteSize() const
			{
				return
					sizeof(uint64_t)+
					B.byteSize()+
					R->byteSize()+
					A8.byteSize()+
					A64.byteSize();
			}

			/**
			 * @return const start iterator (inclusive)
			 **/
			const_iterator begin() const
			{
				return const_iterator(this,0);
			}
			/**
			 * @return const end iterator (exclusive)
			 **/
			const_iterator end() const
			{
				return const_iterator(this,n);
			}

			/**
			 * serialise this object to an output stream
			 *
			 * @param out output stream
			 **/
			void serialise(std::ostream & out) const;
			/**
			 * deserialise a number
			 *
			 * @param in input stream
			 * @return deserialised number
			 **/
			static uint64_t deserializeNumber(std::istream & in);
			/**
			 * constructor for empty array
			 **/
			Array864() : n(0) {}
			/**
			 * constructor from input stream
			 *
			 * @param in input stream
			 **/
			Array864(std::istream & in);

			/**
			 * load object from file fs and return it encapsulated in a unique pointer
			 *
			 * @param fs filename
			 * @return deserialised object as unique pointer
			 **/
			static unique_ptr_type load(std::string const & fs)
			{
				libmaus2::aio::InputStreamInstance CIS(fs);
				unique_ptr_type u(new this_type(CIS));
				return UNIQUE_PTR_MOVE(u);
			}

			/**
			 * load serialised object from InputStreamInstance CIS
			 *
			 * @param CIS input stream
			 * @return deserialised object as unique pointer
			 **/
			static unique_ptr_type load(std::istream & CIS)
			{
				unique_ptr_type u(new this_type(CIS));
				return UNIQUE_PTR_MOVE(u);
			}

			/**
			 * constructor from random access sequence
			 *
			 * @param a start iterator (inclusive)
			 * @param e end iterator (exclusive)
			 **/
			template<typename iterator>
			Array864(iterator a, iterator e)
			{
				n = e-a;

				if ( n )
				{
					B = ::libmaus2::autoarray::AutoArray<data_type>((n+63)/64);
					writer_type W(B.get());

					for ( iterator i = a; i != e; ++i )
						W.writeBit( *i < 256 );

					W.flush();

					::libmaus2::rank::ERank222B::unique_ptr_type tR(new ::libmaus2::rank::ERank222B(B.get(), B.size()*64));
					R = UNIQUE_PTR_MOVE(tR);

					uint64_t const n8 = R->rank1(n-1);
					uint64_t const n64 = R->rank0(n-1);

					A8 = ::libmaus2::autoarray::AutoArray<uint8_t>(n8,false);
					A64 = ::libmaus2::autoarray::AutoArray<uint64_t>(n64,false);

					uint64_t j = 0;
					for ( iterator i = a; i != e; ++i,++j )
						if ( *i < 256 )
							A8[ R->rank1(j)-1 ] = *i;
						else
							A64[ R->rank0(j)-1 ] = *i;

					#if 0
					j = 0;
					for ( iterator i = a; i != e; ++i, ++j )
						assert ( (*this)[j] == *i );
					#endif

					#if defined(ARRAY864DEBUG)
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads)
					#endif
					for ( int64_t i = 0; i < static_cast<int64_t>(n); ++i )
						assert ( (*this)[i] == a[i] );
					#endif
				}

			}

			/**
			 * access operator
			 *
			 * @param i index of element to be accessed
			 * @return element at index i
			 **/
			uint64_t operator[](uint64_t const i) const
			{
				if ( i >= n )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Access of element " << i << " >= " << n << " in Array864::operator[]";
					se.finish();
					throw se;
				}

				if ( ::libmaus2::bitio::getBit(B.get(),i) )
					return A8[R->rank1(i)-1];
				else
					return A64[R->rank0(i)-1];
			}

			/**
			 * get i'th element
			 *
			 * @param i index of element to be accessed
			 * @return element at index i
			 **/
			uint64_t get(uint64_t const i) const
			{
				return (*this)[i];
			}
		};

		/**
		 * generator class for Array864
		 **/
		struct Array864Generator
		{
			//! count of numbers stored in 8 bits
			uint64_t const n8;
			//! count of numbers stored in 64 bits
			uint64_t const n64;
			//! n8+n64
			uint64_t const n;

			//! bit array writer type
			typedef Array864::writer_type writer_type;
			//! bit array data type
			typedef Array864::data_type data_type;

			/**
			 * get number of occurences not exceeding thres in histogram hist
			 *
			 * @param hist histogram
			 * @param thres threshold
			 * @return accumulation of values for keys not exceeding thres in histogram
			 **/
			static uint64_t getSmallerEqual(libmaus2::util::Histogram & hist, int64_t const thres)
			{
				std::map<int64_t,uint64_t> const H = hist.getByType<int64_t>();

				uint64_t c = 0;

				for ( std::map<int64_t,uint64_t>::const_iterator ita = H.begin(); ita != H.end(); ++ita )
					if ( ita->first <= thres )
						c += ita->second;

				return c;
			}

			//! generated array
			Array864::unique_ptr_type P;
			//! writer for bit vector
			writer_type::unique_ptr_type W;
			//! count of 8 bit numbers written
			uint64_t p8;
			//! count of 64 bit numbers written
			uint64_t p64;

			/**
			 * constructor from bit length histogram
			 *
			 * @param hist bit length histogram storing the number of bits necessary
			 *        to store each number in the sequence to be written by this generator
			 **/
			Array864Generator(libmaus2::util::Histogram & hist)
			:
				n8(getSmallerEqual(hist,8)), n64(getSmallerEqual(hist,64)-n8), n(n8+n64),
				P(Array864::unique_ptr_type(new Array864)),
				p8(0), p64(0)
			{
				P->B = ::libmaus2::autoarray::AutoArray<data_type>((n+63)/64);
				writer_type::unique_ptr_type twriter(new writer_type(P->B.begin()));
				W = UNIQUE_PTR_MOVE(twriter);
				P->A8 = ::libmaus2::autoarray::AutoArray<uint8_t>(n8);
				P->A64 = ::libmaus2::autoarray::AutoArray<uint64_t>(n64);
				P->n = n;
			}

			/**
			 * add number v to the end of the stored sequence
			 *
			 * @param v number to be added
			 **/
			void add(uint64_t const v)
			{
				if ( v < 256 )
				{
					W->writeBit(1);
					P->A8[p8++] = v;
				}
				else
				{
					W->writeBit(0);
					P->A64[p64++] = v;
				}
			}

			/**
			 * @return final array object
			 **/
			Array864::unique_ptr_type createFinal()
			{
				W->flush();
				::libmaus2::rank::ERank222B::unique_ptr_type tPR(new ::libmaus2::rank::ERank222B(P->B.get(), P->B.size()*64));
				P->R = UNIQUE_PTR_MOVE(tPR);
				return UNIQUE_PTR_MOVE(P);
			}
		};
	}
}
#endif
