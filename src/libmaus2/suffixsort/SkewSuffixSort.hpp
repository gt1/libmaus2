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

#if ! defined(SKEWSUFFIXSORT_HPP)
#define SKEWSUFFIXSORT_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/aio/OutputBuffer8.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <limits>

namespace libmaus2
{
	namespace suffixsort
	{
		template<typename elem_type>
		inline void saToPhi
			(
			std::string const & inputfilename,
			std::string const & outputfilename
			)
		{
			uint64_t const fs =
				::libmaus2::util::GetFileSize::getFileSize(inputfilename);
			assert ( fs % sizeof(elem_type) == 0 );
			uint64_t const n = fs/sizeof(elem_type);

			::libmaus2::autoarray::AutoArray<elem_type> Phi(n,false);

			::libmaus2::aio::SynchronousGenericInput<elem_type> SA(inputfilename,64*1024);

			if ( n )
			{
				elem_type sa0, sa1;

				bool saok = SA.getNext(sa1);
				assert ( saok );

				Phi[sa1] = n;

				for ( uint64_t i = 1; i < n; ++i )
				{
					sa0 = sa1;
					saok = SA.getNext(sa1);
					assert ( saok );

					Phi [ sa1 ] = sa0;
				}
			}

			::libmaus2::aio::SynchronousGenericOutput<elem_type>::writeArray(Phi,outputfilename);
		}

		// compute plcp	array
		template<typename key_iterator, typename elem_type, typename lcp_data_type>
		inline void plcpPhi(
			key_iterator t,
			uint64_t const n,
			std::string const & phiname,
			std::string const & plcpname)
		{
			uint64_t l = 0;

			::libmaus2::aio::SynchronousGenericInput<elem_type> PHI(phiname,64*1024);
			::libmaus2::aio::SynchronousGenericOutput<lcp_data_type> PLCP(plcpname,64*1024);

			key_iterator const tn = t+n;

			for ( size_t i = 0; i < n; ++i )
			{
				elem_type j;
				bool const phiok = PHI.getNext(j);
				assert ( phiok );

				key_iterator ti = t+i+l;
				key_iterator tj = t+j+l;

				if ( j < i ) while ( (ti != tn) && (*ti == *tj) ) ++l, ++ti, ++tj;
				else         while ( (tj != tn) && (*ti == *tj) ) ++l, ++ti, ++tj;

				PLCP.put(l);

				if ( l >= 1 )
					l -= 1;
				else
					l = 0;
			}

			PLCP.flush();
		}

		// compute plcp	array
		template<typename key_iterator, typename elem_type, typename lcp_data_type>
		inline void plcpPhiNoZero(
			key_iterator t,
			uint64_t const n,
			std::string const & phiname,
			std::string const & plcpname)
		{
			uint64_t l = 0;

			::libmaus2::aio::SynchronousGenericInput<elem_type> PHI(phiname,64*1024);
			::libmaus2::aio::SynchronousGenericOutput<lcp_data_type> PLCP(plcpname,64*1024);

			key_iterator const tn = t+n;

			for ( size_t i = 0; i < n; ++i )
			{
				elem_type j;
				bool const phiok = PHI.getNext(j);
				assert ( phiok );

				key_iterator ti = t+i+l;
				key_iterator tj = t+j+l;

				if ( j < i ) while ( (ti != tn) && (*ti == *tj) && (*ti != 0) ) ++l, ++ti, ++tj;
				else         while ( (tj != tn) && (*ti == *tj) && (*ti != 0) ) ++l, ++ti, ++tj;

				PLCP.put(l);

				if ( l >= 1 )
					l -= 1;
				else
					l = 0;
			}

			PLCP.flush();
		}

		template<typename elem_type>
		inline void saToISA(std::string const & safilename, std::string const & isafilename)
		{
			::libmaus2::aio::SynchronousGenericInput<elem_type> SA(safilename,64*1024);

			uint64_t const fs =
				::libmaus2::util::GetFileSize::getFileSize(safilename);
			assert ( fs % sizeof(elem_type) == 0 );
			uint64_t const n = fs/sizeof(elem_type);

			::libmaus2::autoarray::AutoArray<elem_type> AISA(n,false);

			for ( uint64_t i = 0; i < n; ++i )
			{
				elem_type v;
				bool const saok = SA.getNext(v);
				assert ( saok );

				AISA [ v ] = i;
			}

			::libmaus2::aio::SynchronousGenericOutput<elem_type>::writeArray(AISA,isafilename);
		}

		// compute lcp array using plcp array
		template<typename elem_type, typename lcp_data_type>
		inline void lcpByPlcp(
			std::string const & isafilename,
			std::string const & plcpfilename,
			std::string const & lcpfilename)
		{
			uint64_t const fs =
				::libmaus2::util::GetFileSize::getFileSize(isafilename);
			assert ( fs % sizeof(elem_type) == 0 );
			uint64_t const n = fs/sizeof(elem_type);

			assert ( n == ::libmaus2::util::GetFileSize::getFileSize(plcpfilename)/sizeof(lcp_data_type) );

			::libmaus2::autoarray::AutoArray<lcp_data_type> LCP(n,false);

			::libmaus2::aio::SynchronousGenericInput<elem_type> ISA(isafilename,64*1024);
			::libmaus2::aio::SynchronousGenericInput<lcp_data_type> PLCP(plcpfilename,64*1024);

			for ( uint64_t i = 0; i < n; ++i )
			{
				elem_type visa = 0;
				bool isaok = ISA.getNext(visa);
				lcp_data_type vplcp = 0;
				bool plcpok = PLCP.getNext(vplcp);

				assert ( isaok );
				assert ( plcpok );

				LCP[visa] = vplcp;
			}

			::libmaus2::aio::SynchronousGenericOutput<lcp_data_type> OLCP(lcpfilename,64*1024);

			for ( uint64_t i = 0; i < n; ++i )
				OLCP.put(LCP[i]);

			OLCP.flush();
		}

		template<typename key_type, typename elem_type, size_t offset>
		struct OffsetArray
		{
			key_type const * const y;
			elem_type const n;

			OffsetArray(
				key_type const * const ry,
				elem_type const rn) : y(ry), n(rn) {}

			key_type operator[](size_t i) const {
				i += offset;

				if ( i < n )
					return y[i];
				else
					return 0;
			}
		};

		template<typename key_type, typename elem_type>
		struct Comp
		{
			OffsetArray<key_type,elem_type,0> y;
			OffsetArray<elem_type,elem_type,1> s1;
			OffsetArray<elem_type,elem_type,2> s2;

			Comp(
				key_type const * const ry,
				elem_type const * const rs,
				elem_type const n
			) : y(ry,n), s1(rs,n), s2(rs,n) {}

			bool operator()(elem_type i, elem_type j) const
			{
				if ( (i % 3) == 0 )
				{
					if ( y[i] != y[j] )
						return y[i] < y[j];
					else
						return s1[i] < s1[j];
				}
				else
				{
					if ( y[i] != y[j] )
						return y[i] < y[j];
					else if ( y[i+1] != y[j+1] )
						return y[i+1] < y[j+1];
					else
						return s2[i] < s2[j];
				}
			}
		};

		// multiple character comparison with offset
		template<typename elem_type>
		struct LexicalWideStringOffsetComparator
		{
			elem_type const * const text;
			size_t const offset;
			size_t const n;

			LexicalWideStringOffsetComparator(
				elem_type const * const rtext,
				size_t const roffset,
				size_t const rn
			)
			: text(rtext), offset(roffset), n(rn)
			{
			}

			inline bool operator()(size_t const & pa, size_t const & pb) const
			{
				if ( pa == pb )
					return false;

				size_t aoff = pa+offset;
				size_t boff = pb+offset;

				for ( ; (aoff < n) && (boff < n); ++aoff, ++boff )
				{
					elem_type const va = text[aoff];
					elem_type const vb = text[boff];

					if ( va < vb )
						return true;
					if ( vb < va )
						return false;
				}

				if ( aoff == n )
					return true;
				else
					return false;
			}
		};




		template<typename key_type, typename elem_type, size_t offset>
		struct OffsetBucketSort
		{
			// bucket sort with offset
			// memory:
			//      	(maxelem-minelem)+1*sizeof(elem_type) + P01len * sizeof(elem_type)
			static void offsetBucketSort(
				key_type const * const y,
				elem_type const n,
				elem_type * const P01,
				elem_type const P01len,
				key_type const minelem,
				key_type const maxelem
				)
			{
				// accessor
				OffsetArray<key_type,elem_type,offset> O(y,n);
				// allocate counting array
				::libmaus2::autoarray::AutoArray<elem_type> Abuckcnt(static_cast<elem_type>(maxelem-minelem)+1,true); elem_type * const buckcnt = Abuckcnt.get();
				// count
				for ( elem_type i = 0; i < P01len; ++i ) buckcnt[O[P01[i]]-minelem]++;
				// cumulate to obtain bucket start indices
				elem_type sum = 0; for ( elem_type i = 0; i < static_cast<elem_type>(maxelem-minelem)+1; ++i ) { elem_type const cur = buckcnt[i]; buckcnt[i] = sum; sum += cur; }
				// temporary array
				::libmaus2::autoarray::AutoArray<elem_type> AtP01(P01len); elem_type * const tP01 = AtP01.get();
				// sort
				for ( elem_type i = 0; i < P01len; ++i )
					tP01[ buckcnt[O[P01[i]]-minelem]++ ] = P01[i];
				// copy back
				std::copy(tP01,tP01+P01len,P01);
			}

			// bucket sort with offset
			static void offsetBucketSort(
				key_type const * const y,
				elem_type const n,
				elem_type * const P01,
				elem_type const P01len,
				key_type const maxelem
				)
			{
				// accessor
				OffsetArray<key_type,elem_type,offset> O(y,n);
				// allocate counting array
				::libmaus2::autoarray::AutoArray<elem_type> Abuckcnt(static_cast<elem_type>(maxelem)+1,true); elem_type * const buckcnt = Abuckcnt.get();
				// count
				for ( elem_type i = 0; i < P01len; ++i ) buckcnt[O[P01[i]]]++;
				// cumulate to obtain bucket start indices
				elem_type sum = 0; for ( elem_type i = 0; i < static_cast<elem_type>(maxelem)+1; ++i ) { elem_type const cur = buckcnt[i]; buckcnt[i] = sum; sum += cur; }
				// temporary array
				::libmaus2::autoarray::AutoArray<elem_type> AtP01(P01len); elem_type * const tP01 = AtP01.get();
				// sort
				for ( elem_type i = 0; i < P01len; ++i )
					tP01[ buckcnt[O[P01[i]]]++ ] = P01[i];
				// copy back
				std::copy(tP01,tP01+P01len,P01);
			}

			// bucket sort with offset
			static void offsetBucketSort(
				key_type const * const y,
				elem_type const n,
				elem_type * const P01,
				elem_type const P01len
				)
			{
				// accessor
				OffsetArray<key_type,elem_type,offset> O(y,n);
				// compute maximum element
				key_type maxelem = std::numeric_limits<key_type>::min(); for ( elem_type i = 0; i < P01len; ++i ) maxelem = std::max(maxelem,O[P01[i]]);
				//
				offsetBucketSort(y,n,P01,P01len,maxelem);
			}
		};

		template<typename key_type, typename elem_type>
		struct SkewSuffixSort
		{
			template<typename iterator>
			static ::libmaus2::autoarray::AutoArray<elem_type> skewSuffixSortRef(
				iterator const & ry, elem_type const n
			)
			{
				::libmaus2::autoarray::AutoArray<key_type> y(n);
				for ( uint64_t i = 0; i < n; ++i )
					y[i] = ry[i];
				::libmaus2::autoarray::AutoArray<elem_type> Ap(n);
				skewSuffixSort ( y.get(), n, Ap.get() );

				return Ap;
			}

			static void skewSuffixSort(
				key_type const * const y,
				elem_type const n,
				elem_type * p
			)
			{
				key_type K = std::numeric_limits<key_type>::min();
				for ( size_t i = 0; i < n; ++i )
					K = std::max(K,y[i]);
				skewSuffixSort(y,n,p,K);
			}

			static  void skewSuffixSort(
				key_type const * const y,
				elem_type const n,
				elem_type * p,
				key_type const K
			)
			{
#if defined(SKEW_SUFFIX_SORT_DEBUG)
				std::cerr << "skewSuffixSort in, n=" << n << std::endl;
#endif

				if ( n <= 3 )
				{
					LexicalWideStringOffsetComparator<key_type> lcomp(y,0,n);
					for ( elem_type i = 0; i < n; ++i ) p[i] = i;
					::std::sort(p,p+n,lcomp);
				}
				else
				{
					// do we need an extension ?
					bool const n_mod_3_0 = ((n%3)==0);

					// compute array P01
					elem_type const P01len = ((2*n)/3)+1; ::libmaus2::autoarray::AutoArray<elem_type> AP01(P01len,false); elem_type * P01 = AP01.get();
					for ( elem_type i = 0; i < n; ++i ) { elem_type const mod = i%3; if ( (mod == 0) || (mod == 1) ) *(P01++) = i; } if ( n_mod_3_0 ) *(P01++) = n; P01 = AP01.get();

#if defined(SKEW_SUFFIX_SORT_DEBUG)
					for ( size_t i = 0; i < P01len; ++i )
						std::cerr << "P01[" << i << "]=" << P01[i] << std::endl;
#endif

#if defined(SKEW_SUFFIX_SORT_DEBUG)
					std::cerr << "Sorting according to prefix of length 3...";
#endif

					// sort according to first_3 (3 passes of radix sort)
					OffsetBucketSort<key_type,elem_type,2>::offsetBucketSort(y,n,P01,P01len,K);
					OffsetBucketSort<key_type,elem_type,1>::offsetBucketSort(y,n,P01,P01len,K);
					OffsetBucketSort<key_type,elem_type,0>::offsetBucketSort(y,n,P01,P01len,K);

#if defined(SKEW_SUFFIX_SORT_DEBUG)
					std::cerr << "done." << std::endl;
#endif

#if defined(SKEW_SUFFIX_SORT_DEBUG)
					for ( size_t i = 0; i < P01len; ++i )
						std::cerr << "P01[" << i << "]=" << P01[i] << std::endl;
#endif

					// compute table of ranks
					OffsetArray<key_type,elem_type,0> O(y,n);
					elem_type trank = 0; key_type c0 = O[P01[0]+0]; key_type c1 = O[P01[0]+1]; key_type c2 = !O[P01[0]+2];
					elem_type const zsize = P01len;
					::libmaus2::autoarray::AutoArray<elem_type> Az(zsize, false); elem_type * z = Az.get();
					for ( elem_type i = 0; i < P01len; ++i )
					{
						key_type const d0 = O[P01[i]+0]; key_type const d1 = O[P01[i]+1]; key_type const d2 = O[P01[i]+2];
						if ( (c0 != d0) || (c1 != d1) || (c2 != d2) ) { trank += 1; c0 = d0; c1 = d1; c2 = d2; }

						elem_type const P = P01[i];

						if ( P % 3 == 0 )
							z[P/3] = trank;
						else
							z[P/3 + ((n/3)+1)] = trank;
					}

					AP01.release();

#if defined(SKEW_SUFFIX_SORT_DEBUG)
					for ( size_t i = 0; i < zsize; ++i )
						std::cerr << "z[" << i << "]=" << (z[i]-1) << std::endl;
#endif

					// recursion, compute suffix array q of z
					::libmaus2::autoarray::AutoArray<elem_type> Aq(zsize,false); elem_type * q = Aq.get();
					// we have unique ranks, compute suffix array directly
					if ( trank == P01len )
						for ( size_t i = 0; i < zsize; ++i )
							q[z[i]-1] = i;
					else
						SkewSuffixSort<elem_type,elem_type>::skewSuffixSort(z,zsize,q,trank);

					// release z
					Az.release();

					// q is now the suffix array of z
					// compute array L01, "suffix array" for the 0 mod 3 and 1 mod 3 suffixes
					::libmaus2::autoarray::AutoArray<elem_type> AL01(zsize,false); elem_type * L01 = AL01.get();
					::libmaus2::autoarray::AutoArray<elem_type> As(n+2,false); elem_type * s = As.get();
					for ( elem_type j = 0; j < zsize; ++j )
					{
						elem_type pos;

						if ( q[j] < ((n/3)+1) )
							pos = 3*q[j];
						else
							pos = 3*(q[j]-((n/3)+1))+1;

						L01[j] = pos;
						s[pos] = (j+1);
					}
					s[n+0] = 0; s[n+1] = 0;
					Aq.release();

#if defined(SKEW_SUFFIX_SORT_DEBUG)
					for ( size_t i = 0; i < zsize; ++i )
						std::cerr << "L01[" << i << "]=" << L01[i] << std::endl;
					for ( size_t i = 0; i < n; i += 3 )
						std::cerr << "s[" << i << "]=" << (s[i]-1) << std::endl;
					for ( size_t i = 1; i < n; i += 3 )
						std::cerr << "s[" << i << "]=" << (s[i]-1) << std::endl;
#endif

					// list of suffixes at positions i mod 3 = 2
					elem_type const L2len = n/3;
					::libmaus2::autoarray::AutoArray<elem_type> AL2(L2len,false); elem_type * L2 = AL2.get();
					for ( elem_type i = 2; i < n; i += 3 ) *(L2++) = i; L2 = AL2.get();
					// sort
					OffsetBucketSort<elem_type,elem_type,1>::offsetBucketSort(s,n,L2,L2len,zsize);
					OffsetBucketSort<key_type,elem_type,0>::offsetBucketSort(y,n,L2,L2len,K);

#if defined(SKEW_SUFFIX_SORT_DEBUG)
					for ( size_t i = 0; i < L2len; ++i )
						std::cerr << "L2["<<i<<"]=" << L2[i] << std::endl;
#endif

					//
					elem_type * L = p;
					// comparator
					Comp<key_type,elem_type> comp(y,s,n);
					//
					elem_type a = 0; elem_type b = 0;
					//
					while ( (a < zsize) && (b < L2len) )
					{
						if ( comp(L01[a],L2[b]) )
						{
							if ( L01[a] != n )
								*(L++) = L01[a];
							a++;
						}
						else
						{
							*(L++) = L2[b++];
						}
					}
					while ( a < zsize )
					{
						if ( L01[a] != n )
							*(L++) = L01[a];
						a++;
					}
					while ( b < L2len )
					{
						*(L++) = L2[b++];
					}

					L = p;

#if defined(SKEW_SUFFIX_SORT_DEBUG)
					for ( size_t i = 0; i < n; ++i )
					{
						std::cerr << "L[" << i << "]=" << L[i] << std::endl;
					}
#endif
				}

#if defined(SKEW_SUFFIX_SORT_DEBUG)
				std::cerr << "skewSuffixSort out, n=" << n << std::endl;
#endif
			}

			// compute plcp	array
			static ::libmaus2::autoarray::AutoArray<elem_type> plcpPhi(
				key_type const * t, size_t const n, elem_type const * const Phi)
			{
				elem_type l = 0;

				::libmaus2::autoarray::AutoArray<elem_type> APLCP(n,false); elem_type * PLCP = APLCP.get();
				key_type const * const tn = t+n;
				for ( size_t i = 0; i < n; ++i )
				{
					elem_type const j = Phi[i];

					key_type const * ti = t+i+l;
					key_type const * tj = t+j+l;

					if ( static_cast<size_t>(j) < i ) while ( (ti != tn) && (*ti == *tj) ) ++l, ++ti, ++tj;
					else         while ( (tj != tn) && (*ti == *tj) ) ++l, ++ti, ++tj;

					PLCP[i] = l;

					if ( l >= 1 )
						l -= 1;
					else
						l = 0;
				}

				return APLCP;
			}

			static ::libmaus2::autoarray::AutoArray<elem_type> computePhi(elem_type const * const SA, uint64_t const n)
			{
				// compute Phi
				::libmaus2::autoarray::AutoArray<elem_type> APhi(n,false); elem_type * Phi = APhi.get();
				Phi[SA[0]] = n;
				for ( size_t i = 1; i < n; ++i )
					Phi[SA[i]] = SA[i-1];

				return APhi;
			}

			// compute plcp	array
			static ::libmaus2::autoarray::AutoArray<elem_type> plcp(
				key_type const * t, size_t const n, elem_type const * const SA)
			{
				// compute Phi
				::libmaus2::autoarray::AutoArray<elem_type> APhi = computePhi(SA,n);
				return plcpPhi(t,n,APhi.get());
			}

			// compute lcp array using plcp array
			static ::libmaus2::autoarray::AutoArray<elem_type> lcpByPlcp(key_type const * t, size_t const n, elem_type const * const SA, size_t const pad = 0)
			{
				::libmaus2::autoarray::AutoArray<elem_type> APLCP = plcp(t,n,SA); elem_type const * const PLCP = APLCP.get();
				::libmaus2::autoarray::AutoArray<elem_type> ALCP(n+pad,false); elem_type * LCP = ALCP.get();

				for ( size_t i = 0; i < n; ++i )
					LCP[i] = PLCP[SA[i]];

				return ALCP;
			}

			// compute lcp array using plcp array
			template<typename iterator>
			static ::libmaus2::autoarray::AutoArray<elem_type> lcpByPlcpRef(iterator const & rt, size_t const n, elem_type const * const SA)
			{
				::libmaus2::autoarray::AutoArray<key_type> t(n);
				for ( uint64_t i = 0; i < n; ++i )
					t[i] = rt[i];

				return lcpByPlcp(t.get(), n, SA);
			}
		};
	}
}
#endif
