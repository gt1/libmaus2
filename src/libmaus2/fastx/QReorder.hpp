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
#if ! defined(LIBMAUS_FASTX_QREORDER_HPP)
#define LIBMAUS_FASTX_QREORDER_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/types/types.hpp>
#include <libmaus2/math/lowbits.hpp>
#include <libmaus2/rank/popcnt.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <cassert>

namespace libmaus2
{
	namespace fastx
	{
		template<unsigned int _k>
		struct QReorderTemplate4Base
		{
			enum { k = _k };
			
			typedef QReorderTemplate4Base<k> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef uint64_t value_type;
			
			unsigned int l0;
			unsigned int l1;
			unsigned int l2;
			unsigned int l3;
			unsigned int l;
			
			unsigned int bits0;
			unsigned int bits1;
			unsigned int bits2;
			unsigned int bits3;
			
			unsigned int bits01;
			unsigned int bits12;
			unsigned int bits13;
			unsigned int bits02;
			unsigned int bits23;
			
			uint64_t mask0;
			uint64_t mask1;
			uint64_t mask2;
			uint64_t mask3;
			uint64_t mask01; uint64_t mask23;
			uint64_t mask02; uint64_t mask13;
			uint64_t mask03; uint64_t mask12;
			
			QReorderTemplate4Base(
				unsigned int const rl0,
				unsigned int const rl1,
				unsigned int const rl2,
				unsigned int const rl3
			) : l0(rl0), l1(rl1), l2(rl2), l3(rl3), l(l0+l1+l2+l3),
			    bits0(k*l0),
			    bits1(k*l1),
			    bits2(k*l2),
			    bits3(k*l3),
			    bits01(bits0+bits1),
			    bits12(bits1+bits2),
			    bits13(bits1+bits3),
			    bits02(bits0+bits2),
			    bits23(bits2+bits3),
			    mask0( libmaus2::math::lowbits(bits0) << (k*(l1+l2+l3)) ),
			    mask1( libmaus2::math::lowbits(bits1) << (k*(l2+l3)) ),
			    mask2( libmaus2::math::lowbits(bits2) << (k*(l3)) ),
			    mask3( libmaus2::math::lowbits(bits3) ),
			    mask01( mask0 | mask1 ),
			    mask23( mask2 | mask3 ),
			    mask02( mask0 | mask2 ),
			    mask13( mask1 | mask3 ),
			    mask03( mask0 | mask3 ),
			    mask12( mask1 | mask2 )
			{
			
			}
			
			uint64_t front01(uint64_t const v) const
			{
				return v;
			}

			uint64_t front01i(uint64_t const v) const
			{
				return v;
			}
			
			uint64_t front02(uint64_t const v) const
			{
				return
					(v & mask0)
					|
					((v & mask2) << bits1)
					|
					((v & mask1) >> bits2)
					|
					(v & mask3);
			}

			uint64_t front02i(uint64_t const v) const
			{
				return
					(v & mask0)
					|
					((v >> bits1) & mask2)
					|
					((v << bits2) & mask1)
					|
					(v & mask3);
			}
			
			uint64_t front03(uint64_t const v) const
			{
				return 
					(v & mask0)
					|
					((v & mask12) >> bits3)
					|
					((v & mask3) << (bits12));
			}

			uint64_t front03i(uint64_t const v) const
			{
				return 
					(v & mask0)
					|
					((v << bits3) & mask12)
					|
					((v >> (bits12)) & mask3);
			}
			
			uint64_t front12(uint64_t const v) const
			{
				return 
					((v&mask0) >> (bits12))
					|
					((v&mask12) << bits0)
					|
					(v & mask3);
			}

			uint64_t front12i(uint64_t const v) const
			{
				return 
					((v << (bits12)) & mask0)
					|
					((v >> bits0) & mask12)
					|
					(v & mask3);
			}
			
			uint64_t front13(uint64_t const v) const
			{
				return
					((v&mask0) >> (bits13)) |
					((v&mask1) << (bits0)) |
					((v&mask2) >> (bits3)) |
					((v&mask3) << (bits02))
				;
			}

			uint64_t front13i(uint64_t const v) const
			{
				return
					((v << (bits13)) &mask0) |
					((v >> (bits0)) &mask1) |
					((v << (bits3)) &mask2) |
					((v >> (bits02)) &mask3)
				;
			}
			
			uint64_t front23(uint64_t const v) const
			{
				return
					(v&mask01) >> (bits23)
					|
					(v&mask23) << (bits01);
			}

			uint64_t front23i(uint64_t const v) const
			{
				return
					(v << (bits23) &mask01)
					|
					(v >> (bits01) &mask23);
			}
		};

		enum select_two {
			select_two_01,
			select_two_02,
			select_two_03,
			select_two_12,
			select_two_13,
			select_two_23
		};

		template<unsigned int _k, select_two _sel_two>
		struct QReorderTemplate4 : public QReorderTemplate4Base<_k>
		{
			enum { k = _k };
			typedef QReorderTemplate4Base<k> base_type;			
			enum { sel_two = _sel_two };

			typedef QReorderTemplate4<k,_sel_two> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			unique_ptr_type uclone() const
			{
				unique_ptr_type O(new this_type(base_type::l0,base_type::l1,base_type::l2,base_type::l3));
				return UNIQUE_PTR_MOVE(O);
			}
			
			QReorderTemplate4(
				unsigned int const l0,
				unsigned int const l1,
				unsigned int const l2,
				unsigned int const l3)
			: QReorderTemplate4Base<_k>(l0,l1,l2,l3) {}
			
			uint64_t frontmask() const
			{
				switch ( _sel_two )
				{
					case select_two_01: return (*this)(base_type::mask01);
					case select_two_02: return (*this)(base_type::mask02);
					case select_two_03: return (*this)(base_type::mask03); 
					case select_two_12: return (*this)(base_type::mask12); 
					case select_two_13: return (*this)(base_type::mask13); 
					case select_two_23: return (*this)(base_type::mask23);
				}
			}

			uint64_t backmask() const
			{
				switch ( _sel_two )
				{
					case select_two_01: return libmaus2::math::lowbits(base_type::bits2 + base_type::bits3);
					case select_two_02: return libmaus2::math::lowbits(base_type::bits1 + base_type::bits3);
					case select_two_03: return libmaus2::math::lowbits(base_type::bits1 + base_type::bits2);
					case select_two_12: return libmaus2::math::lowbits(base_type::bits0 + base_type::bits3);
					case select_two_13: return libmaus2::math::lowbits(base_type::bits0 + base_type::bits2);
					case select_two_23: return libmaus2::math::lowbits(base_type::bits0 + base_type::bits1);
				}
			}

			uint64_t operator()(uint64_t const v) const
			{
				switch ( _sel_two )
				{
					case select_two_01: return base_type::front01(v);
					case select_two_02: return base_type::front02(v);
					case select_two_03: return base_type::front03(v);
					case select_two_12: return base_type::front12(v);
					case select_two_13: return base_type::front13(v);
					case select_two_23: return base_type::front23(v);
				}
			}

			uint64_t inverse(uint64_t const v) const
			{
				switch ( _sel_two )
				{
					case select_two_01: return base_type::front01i(v);
					case select_two_02: return base_type::front02i(v);
					case select_two_03: return base_type::front03i(v);
					case select_two_12: return base_type::front12i(v);
					case select_two_13: return base_type::front13i(v);
					case select_two_23: return base_type::front23i(v);
				}
			}
		};
	
		/*
		 * reorder qgram (swap middle part with back, k bits per symbol)
		 */
		template<unsigned int _k>
		struct QReorderTemplate
		{
			enum { k = _k };

			typedef QReorderTemplate<k> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef uint64_t value_type;

			unsigned int const l;
			unsigned int const l0;
			unsigned int const l1;
			unsigned int const l2;
			uint64_t const m0;
			uint64_t const m1;
			uint64_t const m2;
			unsigned int const s0;
			unsigned int const s1;
			unsigned int const s2;
			uint64_t const mm0;
			uint64_t const mm1;
			uint64_t const mm2;
			unsigned int const ss1;
			unsigned int const ss2;
			uint64_t const f;
			
			unique_ptr_type uclone() const
			{
				unique_ptr_type U(new this_type(*this));
				return UNIQUE_PTR_MOVE(U);
			}
			
			shared_ptr_type sclone() const
			{
				shared_ptr_type U(new this_type(*this));
				return U;
			}
			
			QReorderTemplate(QReorderTemplate const & o)
			:
				l(o.l), l0(o.l0), l1(o.l1), l2(o.l2),
				m0(::libmaus2::math::lowbits(k*l0)),
				m1(::libmaus2::math::lowbits(k*l1)),
				m2(::libmaus2::math::lowbits(k*l2)),
				s0((l1+l2)*k),
				s1((l2)*k),
				s2(0*k),
				mm0( m0 << s0 ),
				mm1( m1 << s1 ),
				mm2( m2 << s2 ),
				ss1(k*l2),
				ss2(k*l1),
				f(mm0 | (mm2 << ss2))
			{
				assert ( l0+l1+l2 == l );			
			}
			
			QReorderTemplate(
				unsigned int const rl, 
				unsigned int const rl0, 
				unsigned int const rl1, 
				unsigned int const rl2
			)
			: l(rl), l0(rl0), l1(rl1), l2(rl2),
			  m0(::libmaus2::math::lowbits(k*l0)),
			  m1(::libmaus2::math::lowbits(k*l1)),
			  m2(::libmaus2::math::lowbits(k*l2)),
			  s0((l1+l2)*k),
			  s1((l2)*k),
			  s2(0*k),
			  mm0( m0 << s0 ),
			  mm1( m1 << s1 ),
			  mm2( m2 << s2 ),
			  ss1(k*l2),
			  ss2(k*l1),
			  f(mm0 | (mm2 << ss2))
			{
				assert ( l0+l1+l2 == l );
			}
			
			uint64_t operator()(uint64_t const v) const
			{
				return (v & mm0) | ((v & mm1) >> ss1) | ((v & mm2) << ss2);
			}
			
			uint64_t inverse(uint64_t const v) const
			{
				return (v & mm0) | ((v << ss1) & mm1) | ((v >> ss2) & mm2);
			}
			
			uint64_t backmask() const
			{
				return mm2;
			}
			uint64_t frontmask() const
			{
				return f;
			}
		};
		
		struct QReorderComparator
		{
			uint64_t const frontmask;
			
			template<typename qreorder_type>			
			QReorderComparator(
				qreorder_type const & rqreorder
			)
			: frontmask(rqreorder.frontmask()) 
			{
			
			}
			
			template<typename value_type>
			bool operator()(value_type const a, value_type const b) const
			{
				return (a & frontmask) < (b & frontmask);
			}
		};

		typedef QReorderTemplate<2> QReorder2;
		typedef QReorderTemplate<3> QReorder3;
		
		struct QCache
		{
			unsigned int const lookupbits;
			unsigned int const lookupshift;
			libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > cache;
			
			template<typename iterator, typename qreorder_type>
			QCache(
				iterator ita,
				iterator ite,
				unsigned int const rlookupbits,
				qreorder_type const & qreorder
			) : lookupbits( 
				std::min(
					rlookupbits, 
					libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(qreorder->frontmask()) 
				) 
			    ),
			    lookupshift ( qreorder->k*qreorder->l - lookupbits ),
			    cache(1ull << lookupbits)
			{
				// std::cerr << "lookupbits " << lookupbits << " lookupshift " << lookupshift << std::endl;
			
				std::fill(cache.begin(),cache.end(),std::pair<uint64_t,uint64_t>(0,0));
			
				iterator it = ita;

				while ( it != ite )
				{
					iterator itc = it;
					
					while ( itc != ite && (*itc >> lookupshift) == (*it >> lookupshift) )
						++itc;
					
					cache[ *it >> lookupshift ] = std::pair<uint64_t,uint64_t>(it - ita, itc - ita);
					
					it = itc;
				}
			}
		};
		
		/*
		 *
		 */
		template<typename _value_type, typename _qreorder_type>
		struct QReorderComponent
		{
			typedef _value_type value_type;
			typedef _qreorder_type qreorder_type;
			typedef QReorderComponent<value_type,qreorder_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			typename qreorder_type::unique_ptr_type qreorder;
			libmaus2::autoarray::AutoArray<value_type> const C;
			QReorderComparator const recomp;
			QCache const cache;
			
			template<typename iterator>
			static libmaus2::autoarray::AutoArray<value_type> generateC(qreorder_type const & qreorder, iterator ita, iterator ite)
			{
				libmaus2::autoarray::AutoArray<value_type> C(ite-ita,false);
				value_type * outp = C.begin();
				while ( ita != ite )
					*(outp++) = qreorder(*(ita++));
				std::sort(C.begin(),C.end());
				
				return C;
			}
			
			template<typename iterator>
			QReorderComponent(qreorder_type const & rqreorder, iterator ita, iterator ite, unsigned int const rlookupbits)
			: qreorder(UNIQUE_PTR_MOVE(rqreorder.uclone())), C(generateC(*qreorder,ita,ite)),
			  recomp(*qreorder),
			  cache(C.begin(),C.end(),rlookupbits,qreorder)
			{
				#if 0
				for ( uint64_t i = 0; i < C.size(); ++i )
				{
					std::cerr << C[i] << ";";
				}
				std::cerr << std::endl;
				
				for ( iterator it = ita; it != ite; ++it )
				{
					std::pair<value_type const *, value_type const *> p = search(*it);
					assert ( p.first != p.second );
					
					std::cerr << "[" << p.first - C.begin() << "," << p.second - C.begin() << ")";
					
					bool found = false;
					
					for ( value_type const * pp = p.first; pp != p.second; ++pp )
						if ( 
							(
								(*qreorder)(*it)
							) == (*pp)
						)
							found = true;
					std::cerr << "{" << found << "}";
					assert ( found );
				}
				
				std::cerr << std::endl;
				#endif
			}
			
			std::pair<value_type const *, value_type const *> search(value_type const v)
			{
				value_type const rv = (*qreorder)(v);
				std::pair<uint64_t,uint64_t> const lookup = cache.cache[rv >> cache.lookupshift];
			
				std::pair<value_type const *, value_type const *> p =
					std::equal_range(
						C.begin()+lookup.first,
						C.begin()+lookup.second,
						rv,
						recomp
					);
					
				return p;
			}
		};
		
		template<int i, unsigned int k, unsigned int d>
		struct KmaskPower
		{
			static uint64_t const power = 0;
		};
		
		template<int i, unsigned int k>
		struct KmaskPower<i,k,0>
		{
			static uint64_t const power = 1ull << (i*k);
		};
		
		template<int i, unsigned int k>
		struct KmaskComputation
		{
			static uint64_t const kmask = KmaskPower<i,k,(i*k)/64>::power | KmaskComputation<i-1,k>::kmask;
		};
		
		template<unsigned int k>
		struct KmaskComputation<-1,k>
		{
			static uint64_t const kmask = 0;
		};
		
		template<unsigned int k>
		struct KMask
		{
			static uint64_t const kmask = KmaskComputation<(64/k),k>::kmask;
		};
		
		template<unsigned int k>
		struct KBlockBitK
		{
			static uint64_t compute(uint64_t const v)
			{
				return (v >> (k-1)) | KBlockBitK<k-1>::compute(v);
			}
		};
		
		template<>
		struct KBlockBitK<0>
		{
			static uint64_t compute(uint64_t const)
			{
				return 0;
			}		
		};
		
		template<unsigned int k>
		struct KBlockBitCountPrep
		{
			static uint64_t compute(uint64_t const v)
			{
				return KBlockBitK<k>::compute(v) & KMask<k>::kmask;
			}
		};

		template<unsigned int k>
		struct KBlockBitCount
		{
			static uint64_t compute(uint64_t const v)
			{
				return libmaus2::rank::PopCnt8<sizeof(unsigned long)>::popcnt8(KBlockBitCountPrep<k>::compute(v));
			}
			
			static uint64_t diff(uint64_t const a, uint64_t const b)
			{
				return compute(a ^ b);
			}
		};
		
		template<typename _iterator>
		struct WordPutObject
		{
			typedef _iterator iterator;
			typedef typename ::std::iterator_traits<iterator>::value_type value_type;
			
			iterator it;
			
			WordPutObject(iterator rit) : it(rit) {}
			
			void put(value_type const val)
			{
				*(it++) = val;
			}
		};

		template<typename _value_type>
		struct AutoArrayWordPutObject
		{
			typedef _value_type value_type;
			
			libmaus2::autoarray::AutoArray<value_type> A;
			uint64_t p;
			
			AutoArrayWordPutObject() : p(0) {}
			
			void put(value_type const val)
			{
				if ( p == A.size() )
				{
					uint64_t newsize = A.size() ? 2*A.size() : 1;
					libmaus2::autoarray::AutoArray<value_type> N(newsize,false);
					std::copy(A.begin(),A.end(),N.begin());
					A = N;
				}
				assert ( p < A.size() );
				
				A[p++] = val;
			}
			
			void reset()
			{
				p = 0;
			}
			
			void sort()
			{
				std::sort(A.begin(),A.begin()+p);
			}
			
			void unique()
			{
				p = std::unique(A.begin(),A.begin()+p) - A.begin();
			}
		};

		/*
		 *
		 */
		template<typename _value_type, typename _qreorder_type>
		struct QReorder4Component
		{
			typedef _value_type value_type;
			typedef _qreorder_type qreorder_type;
			typedef QReorder4Component<value_type,qreorder_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef KBlockBitCount<qreorder_type::k> diff_type;
			
			typename qreorder_type::unique_ptr_type qreorder;
			libmaus2::autoarray::AutoArray<value_type> const C;
			QReorderComparator const recomp;
			QCache const cache;
			
			static uint64_t diff(uint64_t const a, uint64_t const b)
			{
				return diff_type::diff(a,b);
			}
			
			template<typename iterator>
			static libmaus2::autoarray::AutoArray<value_type> generateC(qreorder_type const & qreorder, iterator ita, iterator ite)
			{
				libmaus2::autoarray::AutoArray<value_type> C(ite-ita,false);
				value_type * outp = C.begin();
				while ( ita != ite )
					*(outp++) = qreorder(*(ita++));
				std::sort(C.begin(),C.end());
				
				return C;
			}
			
			template<typename other_qreorder_type>
			typename QReorder4Component<value_type,other_qreorder_type>::unique_ptr_type
				getReorderedClone(other_qreorder_type const & rqreorder) const
			{
				typename QReorder4Component<value_type,other_qreorder_type>::unique_ptr_type U(
					new QReorder4Component<value_type,other_qreorder_type>(
						rqreorder,C.begin(),C.end(),cache.lookupbits)
				);
				return UNIQUE_PTR_MOVE(U);
			}
			
			template<typename iterator>
			QReorder4Component(
				qreorder_type const & rqreorder, 
				iterator ita, iterator ite, 
				unsigned int const rlookupbits
			)
			: qreorder(rqreorder.uclone()), 
			  C(generateC(*qreorder,ita,ite)),
			  recomp(*qreorder),
			  cache(C.begin(),C.end(),rlookupbits,qreorder)
			{
				#if 0
				for ( uint64_t i = 0; i < C.size(); ++i )
				{
					std::cerr << C[i] << ";";
				}
				std::cerr << std::endl;
				
				for ( iterator it = ita; it != ite; ++it )
				{
					std::pair<value_type const *, value_type const *> p = search(*it);
					assert ( p.first != p.second );
					
					std::cerr << "[" << p.first - C.begin() << "," << p.second - C.begin() << ")";
					
					bool found = false;
					
					for ( value_type const * pp = p.first; pp != p.second; ++pp )
						if ( ( (*qreorder)(*it) ) == (*pp) )
							found = true;
					std::cerr << "{" << found << "}";
					assert ( found );
				}
				
				std::cerr << std::endl;
				#endif
			}
			
			uint64_t searchRank(value_type const v) const
			{
				value_type const rv = (*qreorder)(v);
				std::pair<uint64_t,uint64_t> const lookup = cache.cache[rv >> cache.lookupshift];
				
				uint64_t const off = 
					std::lower_bound(
						C.begin()+lookup.first,
						C.begin()+lookup.second,rv
					) - C.begin();
					
				assert ( C[off] == v );
					
				return off;
			}

			std::pair<value_type const *, value_type const *> search(value_type const v) const
			{
				value_type const rv = (*qreorder)(v);
				std::pair<uint64_t,uint64_t> const lookup = cache.cache[rv >> cache.lookupshift];
			
				std::pair<value_type const *, value_type const *> p =
					std::equal_range(
						C.begin()+lookup.first,
						C.begin()+lookup.second,
						rv,
						recomp
					);
					
				return p;
			}

			template<typename output_type>
			uint64_t search(value_type const v, output_type & out, unsigned int const maxmis) const
			{
				// reordered query rv
				value_type const rv = (*qreorder)(v);
				// use cache to narrow down search
				std::pair<uint64_t,uint64_t> const lookup = cache.cache[rv >> cache.lookupshift];
				// get matching range by binary search
				std::pair<value_type const *, value_type const *> p =
					std::equal_range(
						C.begin()+lookup.first,
						C.begin()+lookup.second,
						rv,
						recomp
					);
					
				uint64_t c = 0;
				// iterate over interval
				for ( value_type const * it = p.first; it != p.second; ++it )
				{
					value_type const iv = *it;
					
					if ( diff(iv,rv) <= maxmis )
					{
						++c;
						out.put(qreorder->inverse(iv));
					}
				}
				
				return c;
			}			
		};

		/**
		 * class for searching under Hamming distance with up to 2 mismatches.
		 * symbols are _k bits wide. strings are kept in words of type _value_type
		 **/
		template<unsigned int _k, typename _value_type>
		struct QReorder4Set
		{
			enum { k = _k };
			typedef _value_type value_type;
			
			typedef QReorder4Set<_k,_value_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			unsigned int l0;
			unsigned int l1;
			unsigned int l2;
			unsigned int l3;

			typename QReorderTemplate4<k,select_two_01>::unique_ptr_type reorder01;
			typename QReorderTemplate4<k,select_two_02>::unique_ptr_type reorder02;
			typename QReorderTemplate4<k,select_two_03>::unique_ptr_type reorder03;
			typename QReorderTemplate4<k,select_two_12>::unique_ptr_type reorder12;
			typename QReorderTemplate4<k,select_two_13>::unique_ptr_type reorder13;
			typename QReorderTemplate4<k,select_two_23>::unique_ptr_type reorder23;

			typedef QReorder4Component< value_type,QReorderTemplate4<k,select_two_01> > comp01_type;
			typedef QReorder4Component< value_type,QReorderTemplate4<k,select_two_02> > comp02_type;
			typedef QReorder4Component< value_type,QReorderTemplate4<k,select_two_03> > comp03_type;
			typedef QReorder4Component< value_type,QReorderTemplate4<k,select_two_12> > comp12_type;
			typedef QReorder4Component< value_type,QReorderTemplate4<k,select_two_13> > comp13_type;
			typedef QReorder4Component< value_type,QReorderTemplate4<k,select_two_23> > comp23_type;

			typedef typename comp01_type::unique_ptr_type comp01_ptr_type;
			typedef typename comp02_type::unique_ptr_type comp02_ptr_type;
			typedef typename comp03_type::unique_ptr_type comp03_ptr_type;
			typedef typename comp12_type::unique_ptr_type comp12_ptr_type;
			typedef typename comp13_type::unique_ptr_type comp13_ptr_type;
			typedef typename comp23_type::unique_ptr_type comp23_ptr_type;

			comp01_ptr_type comp01;
			comp02_ptr_type comp02;
			comp03_ptr_type comp03;
			comp12_ptr_type comp12;
			comp13_ptr_type comp13;
			comp23_ptr_type comp23;

			template<typename iterator>			
			QReorder4Set(unsigned int l, iterator ita, iterator ite, unsigned int const rlookupbits)
			{
				l0 = (l+3)/4; l -= l0;
				l1 = (l+2)/3; l -= l1;
				l2 = (l+1)/2; l -= l2;
				l3 = l;     ; l -= l3;
					
				typename QReorderTemplate4<_k,select_two_01>::unique_ptr_type rreorder01(new QReorderTemplate4<_k,select_two_01>(l0,l1,l2,l3)); reorder01 = UNIQUE_PTR_MOVE(rreorder01);
				typename QReorderTemplate4<_k,select_two_02>::unique_ptr_type rreorder02(new QReorderTemplate4<_k,select_two_02>(l0,l1,l2,l3)); reorder02 = UNIQUE_PTR_MOVE(rreorder02);
				typename QReorderTemplate4<_k,select_two_03>::unique_ptr_type rreorder03(new QReorderTemplate4<_k,select_two_03>(l0,l1,l2,l3)); reorder03 = UNIQUE_PTR_MOVE(rreorder03);
				typename QReorderTemplate4<_k,select_two_12>::unique_ptr_type rreorder12(new QReorderTemplate4<_k,select_two_12>(l0,l1,l2,l3)); reorder12 = UNIQUE_PTR_MOVE(rreorder12);
				typename QReorderTemplate4<_k,select_two_13>::unique_ptr_type rreorder13(new QReorderTemplate4<_k,select_two_13>(l0,l1,l2,l3)); reorder13 = UNIQUE_PTR_MOVE(rreorder13);
				typename QReorderTemplate4<_k,select_two_23>::unique_ptr_type rreorder23(new QReorderTemplate4<_k,select_two_23>(l0,l1,l2,l3)); reorder23 = UNIQUE_PTR_MOVE(rreorder23);
				
				typename QReorder4Component< value_type,QReorderTemplate4<k,select_two_01> >::unique_ptr_type 
					rcomp01(new QReorder4Component< value_type,QReorderTemplate4<k,select_two_01> >(*reorder01, ita, ite, rlookupbits)); 
				comp01 = UNIQUE_PTR_MOVE(rcomp01);
				
				comp02_ptr_type tcomp02(comp01->getReorderedClone(*reorder02));
				comp02 = UNIQUE_PTR_MOVE(tcomp02);

				comp03_ptr_type tcomp03(comp01->getReorderedClone(*reorder03));
				comp03 = UNIQUE_PTR_MOVE(tcomp03);

				comp12_ptr_type tcomp12(comp01->getReorderedClone(*reorder12));
                                comp12 = UNIQUE_PTR_MOVE(tcomp12);

				comp13_ptr_type tcomp13(comp01->getReorderedClone(*reorder13));
                                comp13 = UNIQUE_PTR_MOVE(tcomp13);

				comp23_ptr_type tcomp23(comp01->getReorderedClone(*reorder23));
                                comp23 = UNIQUE_PTR_MOVE(tcomp23);
			}

			uint64_t slowComp(uint64_t a, uint64_t b) const
			{	
				unsigned int const l = l0+l1+l2+l3;
				
				unsigned int d = 0;
				uint64_t const mask = libmaus2::math::lowbits(k);
				
				for ( unsigned int i = 0; i < l; ++i, a >>= k, b >>= k )
					 if ( (a & mask) != (b & mask) )
					 {
						d++;
					 }
					 
				return d;
			}

			uint64_t search(value_type const v, unsigned int const maxmis, AutoArrayWordPutObject<value_type> & A) const
			{
				A.reset();
				comp01->search(v,A,maxmis);
				comp02->search(v,A,maxmis);
				comp03->search(v,A,maxmis);
				comp12->search(v,A,maxmis);
				comp13->search(v,A,maxmis);
				comp23->search(v,A,maxmis);	
				A.sort();			
				A.unique();
				
				#if 0
				for ( uint64_t i = 0; i < A.p; ++i )
				{
					assert ( 
						slowComp(A.A[i],v) <= maxmis
					);
				}
				#endif
				
				return A.p;
			}

			uint64_t searchRanks(value_type const v, unsigned int const maxmis, AutoArrayWordPutObject<value_type> & A) const
			{
				search(v,maxmis,A);
				for ( uint64_t i = 0; i < A.p; ++i )
					A.A[i] = comp01->searchRank(A.A[i]);
				return A.p;
			}
		};
	}
}
#endif
