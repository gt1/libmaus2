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

#if ! defined(HASHCREATIONBASE_HPP)
#define HASHCREATIONBASE_HPP

#include <libmaus2/fastx/MultiWordDNABitBuffer.hpp>
#include <libmaus2/fastx/SingleWordDNABitBuffer.hpp>
#include <libmaus2/fastx/acgtnMap.hpp>

namespace libmaus2
{
	namespace clustering
	{
		struct HashCreationBase
		{
			typedef ::libmaus2::fastx::MultiWordDNABitBuffer<31> multi_word_buffer_type;

			static ::libmaus2::autoarray::AutoArray<uint8_t> createSymMap()
			{
				::libmaus2::autoarray::AutoArray<uint8_t> S(256);

				S['a'] = S['A'] = ::libmaus2::fastx::mapChar('A');
				S['c'] = S['C'] = ::libmaus2::fastx::mapChar('C');
				S['g'] = S['G'] = ::libmaus2::fastx::mapChar('G');
				S['t'] = S['T'] = ::libmaus2::fastx::mapChar('T');

				return S;
			}

			static ::libmaus2::autoarray::AutoArray<uint8_t> createRevSymMap()
			{
				::libmaus2::autoarray::AutoArray<uint8_t> S(256);

				S['a'] = S['A'] = ::libmaus2::fastx::mapChar('T');
				S['c'] = S['C'] = ::libmaus2::fastx::mapChar('G');
				S['g'] = S['G'] = ::libmaus2::fastx::mapChar('C');
				S['t'] = S['T'] = ::libmaus2::fastx::mapChar('A');

				return S;
			}

			static ::libmaus2::autoarray::AutoArray<unsigned int> createErrorMap()
			{
				::libmaus2::autoarray::AutoArray<unsigned int> S(256);
				std::fill ( S.get(), S.get()+256, 1 );

				S['a'] = S['A'] = 0;
				S['c'] = S['C'] = 0;
				S['g'] = S['G'] = 0;
				S['t'] = S['T'] = 0;

				return S;
			}

			template<typename buffer_type>
			static std::string decodeBuffer(buffer_type const & reve)
			{
				std::string rstring = reve.toString();
				for ( unsigned int i = 0; i < rstring.size(); ++i )
				{
					switch ( rstring[i] )
					{
						case '0': rstring[i] = 'A'; break;
						case '1': rstring[i] = 'C'; break;
						case '2': rstring[i] = 'G'; break;
						case '3': rstring[i] = 'T'; break;
					}
				}
				return rstring;
			}

			static std::string decodeWord(uint64_t const word, unsigned int const k)
			{
				::libmaus2::fastx::SingleWordDNABitBuffer B(k);
				B.buffer = word;

				std::string rstring = B.toString();
				for ( unsigned int i = 0; i < rstring.size(); ++i )
				{
					switch ( rstring[i] )
					{
						case '0': rstring[i] = 'A'; break;
						case '1': rstring[i] = 'C'; break;
						case '2': rstring[i] = 'G'; break;
						case '3': rstring[i] = 'T'; break;
					}
				}
				return rstring;
			}

			template<typename buffer_type>
			static std::string decodeReverseBuffer(buffer_type const & reve)
			{
				std::string rstring = decodeBuffer(reve);
				std::reverse(rstring.begin(),rstring.end());
				for ( unsigned int i = 0; i < rstring.size(); ++i )
					rstring[i] =
						::libmaus2::fastx::remapChar(
							::libmaus2::fastx::invertN(::libmaus2::fastx::mapChar(rstring[i]))
						);
				return rstring;
			}

			static uint64_t getThreadNum()
			{
				#if defined(_OPENMP)
				uint64_t const threadnum = omp_get_thread_num();
				#else
				uint64_t const threadnum = 0;
				#endif

				return threadnum;
			}

			#if 0
			struct AnnotatedArray
			{
				uint64_t * const ptr;
				uint64_t const wordsperobject;

				AnnotatedArray(uint64_t * const rptr, uint64_t rwordsperobject)
				: ptr(rptr), wordsperobject(rwordsperobject)
				{}
			};
			#endif

			#if 0
			struct ArrayProjector
			{
				template<typename type>
				static void swap(type & A, unsigned int i, unsigned int j)
				{
					uint64_t offi = A.wordsperobject*i;
					uint64_t offj = A.wordsperobject*j;

					for ( unsigned int z = 0; z < A.wordsperobject; ++z )
						std::swap(A.ptr[offi++],A.ptr[offj++]);
				}

				template<typename type>
				static bool comp(type & A, unsigned int i, unsigned int j)
				{
					uint64_t offi = A.wordsperobject*i;
					uint64_t offj = A.wordsperobject*j;

					for ( unsigned int z = 0; z < A.wordsperobject; ++z, ++offi, ++offj )
					{
						if ( A.ptr[offi] < A.ptr[offj] )
							return true;
						else if ( A.ptr[offi] > A.ptr[offj] )
							return false;
					}

					return false;
				}
			};
			#endif

			template<unsigned int len>
			struct ShortArray
			{
				uint64_t A[len];

				bool operator< ( ShortArray<len> const & o ) const
				{
					for ( unsigned int i = 0; i < len; ++i )
					{
						if ( A[i] < o.A[i] )
							return true;
						else if ( A[i] > o.A[i] )
							return false;
					}

					return false;
				}
			};

			static void sortShortArray(uint64_t * const ptr, uint64_t const n, unsigned int const wordsperobject)
			{
				switch ( wordsperobject )
				{
					case 0:
						break;
					case 1:
						std::sort(ptr,ptr+n);
						break;
					case 2:
					{
						ShortArray<2> * S = reinterpret_cast< ShortArray<2> * >(ptr);
						std::sort ( S, S + n );
						break;
					}
					case 3:
					{
						ShortArray<3> * S = reinterpret_cast< ShortArray<3> * >(ptr);
						std::sort ( S, S + n );
						break;
					}
					case 4:
					{
						ShortArray<4> * S = reinterpret_cast< ShortArray<4> * >(ptr);
						std::sort ( S, S + n );
						break;
					}
					case 5:
					{
						ShortArray<5> * S = reinterpret_cast< ShortArray<5> * >(ptr);
						std::sort ( S, S + n );
						break;
					}
					default:
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Cannot handle " << wordsperobject
							<< " words per object in sortShortArray.";
						se.finish();
						throw se;
					}
				}
			}

			template < typename array_type >
			struct ShiftComparator
			{
				unsigned int const shift;

				ShiftComparator(unsigned int const rshift)
				: shift(rshift) {}

				inline bool operator()(array_type const & a, array_type const & b) const
				{
					return (a.A[0]>>shift) < (b.A[0]>>shift);
				}
			};

			static void sortShortStable(
				uint64_t * const ptr,
				uint64_t const n,
				unsigned int const wordsperobject,
				unsigned int const hashshift
			)
			{

				switch ( wordsperobject )
				{
					case 0:
						break;
					case 1:
					{
						ShortArray<1> * S = reinterpret_cast< ShortArray<1> * >(ptr);
						ShiftComparator< ShortArray<1> > comp(hashshift);
						::std::stable_sort(S,S+n,comp);
						break;
					}
					case 2:
					{
						ShortArray<2> * S = reinterpret_cast< ShortArray<2> * >(ptr);
						ShiftComparator< ShortArray<2> > comp(hashshift);
						::std::stable_sort ( S, S + n,comp);
						break;
					}
					case 3:
					{
						ShortArray<3> * S = reinterpret_cast< ShortArray<3> * >(ptr);
						ShiftComparator< ShortArray<3> > comp(hashshift);
						::std::stable_sort ( S, S + n,comp);
						break;
					}
					case 4:
					{
						ShortArray<4> * S = reinterpret_cast< ShortArray<4> * >(ptr);
						ShiftComparator< ShortArray<4> > comp(hashshift);
						::std::stable_sort ( S, S + n,comp);
						break;
					}
					case 5:
					{
						ShortArray<5> * S = reinterpret_cast< ShortArray<5> * >(ptr);
						ShiftComparator< ShortArray<5> > comp(hashshift);
						::std::stable_sort ( S, S + n, comp);
						break;
					}
					default:
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "Cannot handle " << wordsperobject
							<< " words per object in sortShortStable.";
						se.finish();
						throw se;
					}
				}
			}
		};
	}
}
#endif
