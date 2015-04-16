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


#if ! defined(SAISUTILS_HPP)
#define SAISUTILS_HPP

#include <libmaus2/bitio/BitVector.hpp>
#include <libmaus2/bitio/CompactArray.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>
#include <libmaus2/parallel/OMPLock.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/select/ESelectSimple.hpp>
#include <libmaus2/select/ESelect222B.hpp>
#include <cassert>

namespace libmaus2
{
	namespace suffixsort
	{
		template<typename iterator>
		::libmaus2::bitio::IndexedBitVector::unique_ptr_type computeS(
			iterator C,
			uint64_t const n
		)
		{
			::libmaus2::bitio::IndexedBitVector::unique_ptr_type pstype(new ::libmaus2::bitio::IndexedBitVector(n,1 /* pad */));
			::libmaus2::bitio::IndexedBitVector & stype = *pstype;


			/* compute stype bit vector */
			stype[n-1] = true;
			for ( int64_t i = static_cast<int64_t>(n)-2; i >= 0; --i )
				stype[i] = (C[i] < C[i+1]) || ((C[i] == C[i+1]) && stype[i+1]);

			return UNIQUE_PTR_MOVE(pstype);
		}
		
		static inline std::vector < std::pair<uint64_t,uint64_t> > mergeParIntervals(
			std::vector < std::pair<uint64_t,uint64_t> > intervals)
		{		
			bool parok = true;	
			do
			{
				parok = true;
				
				uint64_t merge = intervals.size();
				
				if ( intervals.size() > 1 )
				{
					for ( uint64_t i = 0; i < intervals.size(); ++i )
						if ( intervals[i].second-intervals[i].first < 128 )
						{
							merge = i;
							parok = false;
							break;
						}
					
					if ( !parok )
					{
						uint64_t mergewith = intervals.size();
						
						if ( merge > 0 && merge+1 < intervals.size() )
						{
							if ( 
								intervals[merge-1].second-intervals[merge-1].first
								<=
								intervals[merge+1].second-intervals[merge+1].first
							)
							{
								mergewith = merge-1;		
							}
							else
							{
								mergewith = merge+1;
							}
						}	
						else if ( merge > 0 )
						{
							mergewith = merge-1;
						}
						else
						{
							mergewith = merge+1;
						}
						
						assert ( mergewith != intervals.size() );
						
						#if 0
						std::cerr << "Merge "
							<< "[" << intervals[merge].first << "," << intervals[merge].second << ")"
							<< " and "
							<< "[" << intervals[mergewith].first << "," << intervals[mergewith].second << ")" << std::endl;
						#endif
										
						std::vector < std::pair<uint64_t,uint64_t> > newintervals;
						
						for ( uint64_t i = 0; i < intervals.size(); ++i )
							if ( i != merge && i != mergewith )
								newintervals.push_back(intervals[i]);
							else if ( i == merge )
								newintervals.push_back(
									std::pair<uint64_t,uint64_t>
									(std::min(intervals[merge].first,intervals[mergewith].first),
									std::max(intervals[merge].second,intervals[mergewith].second))
								);
						
						intervals = newintervals;
					}
				}

			} while ( ! parok );
			
			#if 0
			for ( uint64_t i = 0; i < intervals.size(); ++i )
			{
				std::cerr << "[" << intervals[i].first << "," << intervals[i].second << ")";
			}
			#endif
		
			return intervals;
		}

		template<typename iterator>
		::libmaus2::bitio::IndexedBitVector::unique_ptr_type computeSParallel(
			iterator C,
			uint64_t const n,
			uint64_t const threads
		)
		{
			::libmaus2::bitio::IndexedBitVector::unique_ptr_type pstype(new ::libmaus2::bitio::IndexedBitVector(n,1 /* pad */));
			::libmaus2::bitio::IndexedBitVector & stype = *pstype;

			if ( n )
			{
				uint64_t const packets = 2*threads;
				::libmaus2::autoarray::AutoArray<uint64_t> endpoints(packets+1);
				uint64_t const targfragsize = (n + packets - 1) / packets;
				
				#if 0
				std::cerr << "(***n=" << n << ",targfragsize=" << targfragsize << ")"; 
				#endif
				
				for ( uint64_t i = 0; i < endpoints.size(); ++i )
				{
					endpoints[i] = std::min( i * targfragsize , n-1 );

					while ( 
						endpoints[i]
						&&
						endpoints[i] > endpoints[i-1]
						&&
						endpoints[i] != (n-1)
						&&
						C [ endpoints[i] ] >= C [ endpoints[i] + 1 ]
					)
						endpoints[i]--;	
				}
				
				endpoints[endpoints.size()-1] = n-1;
				
				for ( uint64_t i = 0; i < endpoints.size(); ++i )
					assert (
						endpoints[i] == 0
						||
						endpoints[i] == (n-1)
						||
						C [ endpoints[i] ] < C [ endpoints[i] + 1 ]
					);
				
				std::vector < std::pair<uint64_t,uint64_t> > intervals;
				for ( uint64_t i = 1; i < endpoints.size(); ++i )
					if ( endpoints[i-1] != endpoints[i] )
						intervals.push_back( std::pair<uint64_t,uint64_t>(endpoints[i-1],endpoints[i]) );

				intervals = mergeParIntervals(intervals);
					
				#if 0		
				std::cerr << "(even...";
				#endif
				
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t j = 0; j < static_cast<int64_t>(intervals.size()); j += 2 )
				{
					stype[intervals[j].second] = true;
					for ( int64_t i = static_cast<int64_t>(intervals[j].second)-1; i >= static_cast<int64_t>(intervals[j].first); --i )
						stype[i] = (C[i] < C[i+1]) || ((C[i] == C[i+1]) && stype[i+1]);
				}
				
				#if 0
				std::cerr << ")";
				
				std::cerr << "(odd...";
				#endif
				
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t j = 1; j < static_cast<int64_t>(intervals.size()); j += 2 )
				{
					stype[intervals[j].second] = true;
					for ( int64_t i = static_cast<int64_t>(intervals[j].second)-1; i >= static_cast<int64_t>(intervals[j].first); --i )
						stype[i] = (C[i] < C[i+1]) || ((C[i] == C[i+1]) && stype[i+1]);
				}

				#if 0
				std::cerr << ")";
				#endif
			}

			return UNIQUE_PTR_MOVE(pstype);
		}

		inline void sToSast(::libmaus2::bitio::IndexedBitVector & stype)
		{
			uint64_t const n = stype.size();
			/* convert stype bit vector to s*type bit vector */
			for ( int64_t i = static_cast<int64_t>(n)-1; i >= 1; --i )
				stype [ i ] = stype[i] && (!stype[i-1]);
		}

		inline void sToSastParallel(::libmaus2::bitio::IndexedBitVector & stype, uint64_t const threads)
		{
			uint64_t const n = stype.size();
			
			if ( n )
			{
				uint64_t const packets = 2*threads;
				::libmaus2::autoarray::AutoArray<uint64_t> endpoints(packets+1);
				uint64_t const targfragsize = (n + packets - 1) / packets;
				
				#if 0
				std::cerr << "(***n=" << n << ",targfragsize=" << targfragsize << ")"; 
				#endif
				
				for ( uint64_t i = 0; i < endpoints.size(); ++i )
				{
					endpoints[i] = std::min( i * targfragsize , n-1 );

					while ( endpoints[i] && !stype[endpoints[i]] )
						endpoints[i]--;	
					while ( ! stype[endpoints[i]] )
						endpoints[i]++;
						
					assert ( stype[endpoints[i]] );
					
					while ( endpoints[i] && stype[endpoints[i]-1] )
						endpoints[i]--;
						
					assert ( stype[endpoints[i]] && (endpoints[i] == 0 || !stype[endpoints[i]-1]) );
				}
				
				endpoints[endpoints.size()-1] = n-1;

				std::vector < std::pair<uint64_t,uint64_t> > intervals;
				for ( uint64_t i = 1; i < endpoints.size(); ++i )
					if ( endpoints[i-1] != endpoints[i] )
						intervals.push_back( std::pair<uint64_t,uint64_t>(endpoints[i-1],endpoints[i]) );

				intervals = mergeParIntervals(intervals);
				
				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t j = 0; j < static_cast<int64_t>(intervals.size()); j += 2 )
					for ( uint64_t i = intervals[j].second; i > intervals[j].first; --i )
						stype [ i ] = stype[i] && (!stype[i-1]);

				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t j = 1; j < static_cast<int64_t>(intervals.size()); j += 2 )
					for ( uint64_t i = intervals[j].second; i > intervals[j].first; --i )
						stype [ i ] = stype[i] && (!stype[i-1]);
			}
		}

		template<typename iterator>
		::libmaus2::bitio::IndexedBitVector::unique_ptr_type computeSast(
			iterator C,
			uint64_t const n
		)
		{
			uint64_t const threads = 8;
		
			::libmaus2::bitio::IndexedBitVector::unique_ptr_type pstype(computeSParallel(C,n,threads));
			::libmaus2::bitio::IndexedBitVector & stype = *pstype;

			sToSastParallel(stype,threads);
			stype.setupIndex();
			
			return UNIQUE_PTR_MOVE(pstype);
		}


		template<typename iterator>
		::libmaus2::bitio::IndexedBitVector::unique_ptr_type computeSast(
			iterator C,
			uint64_t const n,
			::libmaus2::autoarray::AutoArray < uint64_t > & Shist,
			::libmaus2::autoarray::AutoArray < uint64_t > & Lhist,
			typename ::std::iterator_traits< iterator > :: value_type & maxk
		)
		{
			::libmaus2::bitio::IndexedBitVector::unique_ptr_type pstype = computeS(C,n);
			::libmaus2::bitio::IndexedBitVector & stype = *pstype;
			
			typedef typename ::std::iterator_traits< iterator > :: value_type value_type;
			maxk = std::numeric_limits< value_type >::min();
			
			for ( uint64_t i = 0; i < n; ++i )
				maxk = std::max(maxk,C[i]);

			Shist = ::libmaus2::autoarray::AutoArray < uint64_t >( maxk+2 );
			Lhist = ::libmaus2::autoarray::AutoArray < uint64_t >( maxk+2 );

			for ( uint64_t i = 0; i < n; ++i )
				if ( stype.get(i) )
					Shist[C[i]]++;
				else
					Lhist[C[i]]++;

			sToSast(stype);
			stype.setupIndex();

			return UNIQUE_PTR_MOVE(pstype);
		}

		// comparator for S* type substrings
		struct STypeComparator
		{
			::libmaus2::bitio::CompactArray const & C;
			::libmaus2::bitio::IndexedBitVector const & stype;
			
			STypeComparator(
				::libmaus2::bitio::CompactArray const & rC,
				::libmaus2::bitio::IndexedBitVector const & rstype
			)
			: C(rC), stype(rstype)
			{
			
			}
			
			bool operator()(uint64_t a, uint64_t b) const
			{
				if ( a == b )
					return false;
				
				assert ( C.get(a) == C.get(b) );
				
				uint64_t const la = stype.next1(a+1)-a+1;
				uint64_t const lb = stype.next1(b+1)-b+1;
				
				// same length, compare until next s* type character (excl)
				if ( la == lb )
				{
					// symbols -> bits
					a *= C.getB();
					b *= C.getB();
					uint64_t compbits = la*C.getB();
					
					while ( compbits )
					{
						uint64_t const rbits = std::min(static_cast<uint64_t>(64),compbits);

						uint64_t const va = ::libmaus2::bitio::getBits(C.D,a,rbits);
						uint64_t const vb = ::libmaus2::bitio::getBits(C.D,b,rbits);
						
						if ( va != vb )
							return va < vb;
						
						compbits -= rbits;
						a += rbits;
						b += rbits;
					}

					return false;
				}
				// different length, compare until we find a difference (or reach the end of one substring)
				else
				{
					// symbols -> bits
					a *= C.getB();
					b *= C.getB();
					uint64_t abits = la * C.getB();
					uint64_t bbits = lb * C.getB();
					uint64_t compbits = std::min(abits,bbits);
					
					while ( compbits )
					{
						uint64_t const rbits = std::min(static_cast<uint64_t>(64),compbits);

						uint64_t const va = ::libmaus2::bitio::getBits(C.D,a,rbits);
						uint64_t const vb = ::libmaus2::bitio::getBits(C.D,b,rbits);
						
						if ( va != vb )
							return va < vb;
						
						compbits -= rbits;
						a += rbits;
						b += rbits;
						abits -= rbits;
						bbits -= rbits;
					}

					assert ( abits != bbits );
					assert ( abits == 0 || bbits == 0 );
					
					return (!bbits);
				}		
			}
		};
		
		template<typename hash_type>
		struct NameDecodeDict
		{
			typedef NameDecodeDict<hash_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			uint64_t const numcol;
			uint64_t const numnoncol;
			::libmaus2::bitio::IndexedBitVector::unique_ptr_type SubColVec;

			::libmaus2::autoarray::AutoArray<hash_type> HHNonCol;
			::libmaus2::bitio::CompactArray lennoncol;
			
			::libmaus2::bitio::CompactArray HHCol;
			
			NameDecodeDict(
				uint64_t const redast,
				uint64_t const rnumcol,
				uint64_t const rnumnoncol,
				unsigned int const shortbits,
				unsigned int const longbits
				)
			: numcol(rnumcol), numnoncol(rnumnoncol), SubColVec(new ::libmaus2::bitio::IndexedBitVector(redast)),
			  HHNonCol(numnoncol), lennoncol(numnoncol,shortbits), HHCol(numcol,longbits) {}
			
			uint64_t byteSize() const
			{
				return 2*sizeof(uint64_t) + SubColVec->byteSize() + HHNonCol.byteSize() + lennoncol.byteSize();
			}
			
			void setNonCol(uint64_t const name, uint64_t const val, unsigned int const len)
			{
				uint64_t const rank = SubColVec->rank0(name)-1;
				HHNonCol[rank] = val;
				lennoncol[rank] = len;
				
			}
			
			void setCol(uint64_t const name, uint64_t pos)
			{
				uint64_t const rank = SubColVec->rank1(name)-1;
				HHCol [ rank ] = pos;
			}
			
			uint64_t getColPos(uint64_t const name) const
			{
				uint64_t const rank = SubColVec->rank1(name)-1;
				return HHCol.get(rank);			
			}

			std::pair<uint64_t,unsigned int> getNonColWord(uint64_t const name) const
			{
				uint64_t const rank = SubColVec->rank0(name)-1;
				return std::pair<uint64_t,unsigned int>(HHNonCol[rank],lennoncol[rank]);
			}
			
			void setupSubColVec(
				::libmaus2::bitio::IndexedBitVector const & ColBV,
				::libmaus2::bitio::IndexedBitVector const & NonColBV,
				::libmaus2::autoarray::AutoArray<uint64_t> const & ColIndex
				)
			{
				for ( uint64_t i = 0; i < SubColVec->size(); ++i )
					(*SubColVec)[i] = true;
				for ( uint64_t hash = 0; hash < NonColBV.size(); ++hash )
					if ( NonColBV.get(hash) )
						(*SubColVec)[ColIndex [ ColBV.rank1(hash) ] + NonColBV.rank1(hash)-1] = false;
				SubColVec->setupIndex();		
			}
		};
		
		template<typename hash_type>
		struct HashReducedString
		{
			typedef HashReducedString<hash_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
		
			::libmaus2::bitio::CompactArray::unique_ptr_type hreduced;
			typename NameDecodeDict<hash_type>::unique_ptr_type namedict;
			
			HashReducedString(
				uint64_t const redast,
				uint64_t const numsubname,
				uint64_t const numcol,
				uint64_t const numnoncol,
				unsigned int const shortbits,
				unsigned int const longbits
				)
			: hreduced(new ::libmaus2::bitio::CompactArray(redast,::libmaus2::math::numbits(numsubname))),
			  namedict(new NameDecodeDict<hash_type>(redast,numcol,numnoncol,shortbits,longbits))
			{}
			
			uint64_t byteSize() const
			{
				return hreduced->byteSize() + namedict->byteSize();
			}
		};

		template<typename hash_type>
		struct HashLookupTable
		{
			typedef ::libmaus2::bitio::CompactArray array_type;
			typedef array_type::unique_ptr_type array_ptr_type;

			typedef ::libmaus2::util::ConstIterator<HashLookupTable,uint64_t> const_iterator;

			typedef HashLookupTable this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			// bit vector
			::libmaus2::bitio::IndexedBitVector::unique_ptr_type B;
			// table for short strings
			::libmaus2::autoarray::AutoArray< array_ptr_type > S;
			// explicit names of long strings
			array_ptr_type L;
			// text
			::libmaus2::bitio::CompactArray const & C;
			// s* bit vector
			::libmaus2::bitio::IndexedBitVector const & stype;
			// select dictionary
			// typedef ::libmaus2::select::ESelect222B<true> select_type;
			static unsigned int const selectstepbitslog = 7;
			typedef ::libmaus2::select::ESelectSimple<true,selectstepbitslog> select_type;
			typename select_type::unique_ptr_type eselect;
			//
			uint64_t const soffset;
			// name dictionary
			typename NameDecodeDict<hash_type>::unique_ptr_type namedict;
			
			uint64_t const K;

			HashLookupTable ( 
				uint64_t const mini,
				uint64_t const b,
				uint64_t const numsubbits,
				uint64_t const redast,
				uint64_t const minsumhigh,
				::libmaus2::bitio::CompactArray const & rC,
				::libmaus2::bitio::IndexedBitVector const & rstype,
				// name dict
				uint64_t const numcol,
				uint64_t const numnoncol,
				unsigned int const shortbits,
				unsigned int const longbits
			)
			: 
			  B(new ::libmaus2::bitio::IndexedBitVector(redast)),
			  S ( mini+1 ), 
			  L(new array_type(minsumhigh,numsubbits)),
			  C(rC),
			  stype(rstype),
			  eselect ( new select_type(stype.A.get(),stype.A.size()*64) ),
			  soffset( stype.size() ? (stype[0] ? 1 : 0) : 0 ),
			  namedict(new NameDecodeDict<hash_type>(redast,numcol,numnoncol,shortbits,longbits)),
			  K(numcol + numnoncol)
			{
				for ( uint64_t i = 0; i < S.size(); ++i )
				{
					array_ptr_type Si(
                                                new array_type( 1ull << (i*b) , numsubbits )
                                                );
					S[i] = UNIQUE_PTR_MOVE(Si);
				}
			}
			
			uint64_t byteSize() const
			{
				uint64_t s = B->byteSize() + L->byteSize() + namedict->byteSize() + eselect->byteSize();
				for ( uint64_t i = 0; i < S.size(); ++i )
					s += S[i]->byteSize();
				return s;
			}
			
			inline std::pair<uint64_t,uint64_t> sPos(uint64_t const i) const
			{
				if ( i == B->size()-1 )
				{
					return std::pair<uint64_t,uint64_t>(C.n-1,C.n);
				}
				else
				{
					// uint64_t const right = stype.select1( i + soffset );
					uint64_t const right = eselect->select1( i + soffset );
					uint64_t const left = (i+soffset) ? stype.prev1(right-1) : 0;
					assert ( stype[right] );
					assert ( left == 0 || stype[left] );
					return std::pair<uint64_t,uint64_t>(left,right+1);
				}
			}
			
			inline uint64_t getSLen(uint64_t const i) const
			{
				std::pair<uint64_t,uint64_t> const P = sPos(i);
				return P.second-P.first;
			}
			
			inline std::ostream & printS(std::ostream & out, uint64_t const i) const
			{
				std::pair<uint64_t,uint64_t> const P = sPos(i);
				uint64_t const slen = getSLen(i);
				out << "[" << P.first << "," << P.second << ")" << " len " << slen;
				return out;
			}
			
			inline uint64_t get(uint64_t const i) const
			{
				return (*this)[i];
			}
			
			inline uint64_t operator[](uint64_t const i) const
			{
				if ( B->get(i) )
				{
					std::pair<uint64_t,uint64_t> const P = sPos(i);
					uint64_t const len = P.second-P.first;
					uint64_t const v = ::libmaus2::bitio::getBits(C.D,P.first*C.getB(),len*C.getB());
					assert ( v < S[len]->size() );
					return S[len]->get(v);
				}
				else
				{
					return L->get( B->rank0(i)-1 );				
				}
			}
			
			const_iterator begin() const
			{
				return const_iterator(this);
			}
			const_iterator end() const
			{
				return const_iterator(this,B->size());
			}
			
			uint64_t size() const
			{
				return B->size();
			}
			
			uint64_t getB() const
			{
				return L->getB();
			}
		};
		
		template<typename array_type, typename hash_type=uint32_t>
		struct HashReduction
		{
			static uint64_t const printfrac = 256;
			
			array_type const & C;
			::libmaus2::bitio::IndexedBitVector const & stype;
			uint64_t const smallthres;
			bool const verbose;
			uint64_t const fraction;
			uint64_t const n;
			uint64_t const b;
			unsigned int const uintbits;
			unsigned int const uintbits1;
			unsigned int const shortbits;
			unsigned int const longbits;
			uint64_t redast;
			uint64_t numcolnames;
			uint64_t numnoncolnames;
			uint64_t numsubnames;
			uint64_t numsubbits;
			uint64_t hashbits;
			STypeComparator stypecomp;

			/* data structures for hashing */
			// low bits arrays
			::libmaus2::autoarray::AutoArray<hash_type> LB;
			// hash table
			::libmaus2::autoarray::AutoArray<hash_type> HH;
			// collision bit vector
			::libmaus2::bitio::IndexedBitVector::unique_ptr_type ColBV;
			// non collision bit vector
			::libmaus2::bitio::IndexedBitVector::unique_ptr_type NonColBV;
			// 
			::libmaus2::autoarray::AutoArray<uint64_t> ColIndex;
			//
			::libmaus2::autoarray::AutoArray<uint64_t> ColIndexCopy;
			//
			::libmaus2::autoarray::AutoArray<uint64_t,::libmaus2::autoarray::alloc_type_c> ColVec;
			//
			::libmaus2::util::Histogram slenhist;
		
			HashReduction(
				array_type const & rC, 
				::libmaus2::bitio::IndexedBitVector const & rstype,
				uint64_t const rsmallthres,
				bool const rverbose = false,
				uint64_t const rfraction = 8
			) : C(rC), stype(rstype), smallthres(rsmallthres), verbose(rverbose), 
			    fraction(rfraction), n(C.size()), b(C.getB()),
			    uintbits(::libmaus2::math::numbits(std::numeric_limits<hash_type>::max())), 
			    uintbits1(uintbits-1),
			    shortbits( ::libmaus2::math::numbits((uintbits-1)/b) ),
			    longbits ( ::libmaus2::math::numbits(n-1) ),
			    redast(0),
			    numcolnames(0), numnoncolnames(0), numsubnames(0), numsubbits(0), 
			    hashbits(0),
			    stypecomp(C,stype),
			    LB(uintbits+1)
			{
				// typedef typename array_type::const_iterator compact_const_it;
				
				uint64_t const maxhashmem = std::max ( static_cast<uint64_t>(((n*b+7)/8)/fraction) , static_cast<uint64_t>(1024) );
				hashbits = 1;
				while ( ((1ull << (hashbits+1))*sizeof(hash_type) < maxhashmem) && (hashbits+1 <= uintbits) )
					hashbits += 1;
					
				hashbits = std::max(hashbits,b);
					
				::libmaus2::timing::RealTimeClock rtc;
				
				if ( verbose )
					std::cerr << "(hashbits=" << hashbits << ")";

				/* data structures for hashing */
				// hash table
				HH = ::libmaus2::autoarray::AutoArray<hash_type> ( 1ull << hashbits );
				// collision bit vector
				::libmaus2::bitio::IndexedBitVector::unique_ptr_type tColBV(new ::libmaus2::bitio::IndexedBitVector(HH.size()) );
				ColBV = UNIQUE_PTR_MOVE(tColBV);
				// non collision bit vector
				::libmaus2::bitio::IndexedBitVector::unique_ptr_type tNonColBV(new ::libmaus2::bitio::IndexedBitVector( HH.size() ) );
				NonColBV = UNIQUE_PTR_MOVE(tNonColBV);
				
				if ( verbose )
					std::cerr << "(bs="
						<< static_cast<double>((LB.byteSize() + HH.byteSize() + ColBV->byteSize() + NonColBV->byteSize()))/C.byteSize() << ")";
				
				for ( unsigned int i = 0; i < LB.size(); ++i )
					LB[i] = ::libmaus2::math::lowbits(i);
				hash_type const colmask = (static_cast<hash_type>(1) << (uintbits-1));

				if ( verbose )
				{
					std::cerr << "(computing hash table...";
					rtc.start();
				}

				uint64_t lastspos = 0;
				for ( uint64_t i = ((1 < n)?stype.next1(1):n); i < n; i= ((i==(n-1))?(i+1):stype.next1(i+1)) )
				{
					assert ( stype.get(i) );
					
					uint64_t const len = (i-lastspos)+1;
					uint64_t const availc = std::min( (n-lastspos) , len );
					unsigned int const getbits = std::min( availc*b, hashbits );
					unsigned int const shiftbits = hashbits - getbits;
					uint64_t const hash = (::libmaus2::bitio::getBits ( C.D , lastspos*b, getbits ) << shiftbits) | LB[shiftbits];
		
					#if 0
					std::cerr << "+++: ";			
					for ( uint64_t j = 0; j < len; ++j )
						std::cerr << static_cast<uint8_t>(C[lastspos+j]);
					std::cerr << std::endl;
					#endif

					if ( len*b < uintbits && len <= smallthres )
					{
						uint64_t const v = ::libmaus2::bitio::getBits ( C.D , lastspos*b, len*b ) << (uintbits1-len*b);
						
						if ( ! HH[hash] )
							HH[hash] = v;
						else if ( HH[hash] != v )
							HH[hash] = colmask;
					}
					else
					{
						HH[hash] = colmask;
					}

					lastspos = i;
				}
					
				if ( verbose )
					std::cerr << "time=" << rtc.getElapsedSeconds() << ")";

				if ( verbose )
				{
					std::cerr << "(computing collision bit vectors...";
					rtc.start();
				}
				
				(*NonColBV)[0] = true;
				for ( uint64_t i = 0; i < HH.size(); ++i )
					if ( HH[i] & colmask )
						(*ColBV)[i] = true;
					else if ( HH[i] )
						(*NonColBV)[i] = true;
				ColBV->setupIndex();
				NonColBV->setupIndex();
				uint64_t const numcol = ColBV->rank1(ColBV->size()-1);

				if ( verbose )
					std::cerr << "time=" << rtc.getElapsedSeconds() << ", bs="
						<< static_cast<double>((LB.byteSize() + HH.byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColBV->byteSize() + NonColBV->byteSize()))/C.byteSize() << ", collisions on " << numcol << " hash values)";
				
				if ( verbose )
				{
					std::cerr << "(computing ColIndex, number of suffixes for each colliding hash value...";
					rtc.start();
				}
					
				ColIndex = ::libmaus2::autoarray::AutoArray<uint64_t>(numcol+1);
				lastspos = 0;
				for ( uint64_t i = ((1 < n)?stype.next1(1):n); i < n; i= ((i==(n-1))?(i+1):stype.next1(i+1)) )
				{
					assert ( stype.get(i) );

					uint64_t const len = (i-lastspos)+1;
					uint64_t const availc = std::min( (n-lastspos) , len );
					unsigned int const getbits = std::min( availc*b, hashbits );
					unsigned int const shiftbits = hashbits - getbits;
					uint64_t const hash = (::libmaus2::bitio::getBits ( C.D , lastspos*b, getbits ) << shiftbits) | LB[shiftbits];

					if ( ColBV->get(hash) )
						ColIndex [ ColBV->rank1(hash) - 1 ]++;

					lastspos = i;
				}
				uint64_t maxcol = 0;
				for ( uint64_t i = 0; i < numcol; ++i )
					maxcol = std::max(maxcol,ColIndex[i]);
				ColIndex.prefixSums();
				ColIndexCopy = ColIndex.clone();

				if ( verbose )
				{
					std::cerr << "time=" << rtc.getElapsedSeconds() << ", bs="
						<< static_cast<double>((LB.byteSize() + HH.byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColIndex.byteSize() + ColIndexCopy.byteSize()))/C.byteSize() 
						<< ", maxcol=" << maxcol << ", total=" << ColIndex[ColIndex.size()-1]
						<< ")"
						;
				}

				if ( verbose )
				{
					std::cerr << "(Computing ColVec, positions of substrings with colliding hash values...";
					rtc.start();
				}

				ColVec = ::libmaus2::autoarray::AutoArray<uint64_t,::libmaus2::autoarray::alloc_type_c>( ColIndex[ColIndex.size()-1] );
				lastspos = 0;
				redast = 0;
				for ( uint64_t i = ((1 < n)?stype.next1(1):n); i < n; i= ((i==(n-1))?(i+1):stype.next1(i+1)) )
				{
					assert ( stype.get(i) );

					uint64_t const len = (i-lastspos)+1;
					uint64_t const availc = std::min( (n-lastspos) , len );
					unsigned int const getbits = std::min( availc*b, hashbits );
					unsigned int const shiftbits = hashbits - getbits;
					uint64_t const hash = (::libmaus2::bitio::getBits ( C.D , lastspos*b, getbits ) << shiftbits) | LB[shiftbits];

					if ( ColBV->get(hash) )
						ColVec [ ColIndex [ ColBV->rank1(hash) - 1 ]++ ] = lastspos;

					lastspos = i;
					redast++;

					slenhist(len);
				}
				redast++; // terminator
				slenhist(1);
				std::copy(ColIndexCopy.begin(),ColIndexCopy.end(),ColIndex.begin());

				if ( verbose )
					std::cerr << "time=" << rtc.getElapsedSeconds() << ", bs="
						<< static_cast<double>((LB.byteSize() + HH.byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColIndex.byteSize() + ColIndexCopy.byteSize() + ColVec.byteSize()))/C.byteSize() << ")";
						

				if ( verbose )
				{
					std::cerr << "(Sorting strings in collision buckets, total=" << ColIndex[ColIndex.size()-1] << "...";
					rtc.start();
				}

				STypeComparator stypecomp(C,stype);
				uint64_t const printfrac = 1024;
				// uint64_t const printthres = (ColIndex[ColIndex.size()-1] + printfrac - 1) / printfrac;
				uint64_t printacc = 0;
				
				if ( ColIndex.size() >= 1 )
				{
					uint64_t const innerloops = ColIndex.size()-1ull;
					uint64_t const blocksize = ( ColIndex.size() + printfrac - 1 ) / printfrac;
					uint64_t const blocks = ( innerloops + blocksize - 1 ) / blocksize;
					
					::libmaus2::parallel::OMPLock printlock;
					
					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1)
					#endif
					for ( int64_t block = 0; block < static_cast<int64_t>(blocks); ++block )
					{
						uint64_t const blocklow = block*blocksize;
						uint64_t const blockhigh = std::min ( blocklow + blocksize , innerloops );
						uint64_t acc = 0;
						
						for ( uint64_t i = blocklow; i < blockhigh; ++i )
						{
							uint64_t const low = ColIndex[i];
							uint64_t const high = ColIndex[i+1];
							std::sort(ColVec.begin()+low,ColVec.begin()+high,stypecomp);
							acc += (high-low);
						}
						
						printlock.lock();
						printacc += acc;
						if ( verbose )
							std::cerr << "(" << static_cast<double>(printacc) / ColIndex[ColIndex.size()-1] << ")";
						printlock.unlock();
					}
				}

				if ( verbose )
					std::cerr << "(1)";

				if ( verbose )
					std::cerr << "time=" << rtc.getElapsedSeconds() << ", bs="
						<< static_cast<double>((LB.byteSize() + HH.byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColIndex.byteSize() + ColIndexCopy.byteSize() + ColVec.byteSize()))/C.byteSize() << ")";

				if ( verbose )
				{
					std::cerr << "(Compacting collision buckets...";
					rtc.start();
				}

				#if defined(_OPENMP)
				#pragma omp parallel for
				#endif
				for ( int64_t i = 1; i < static_cast<int64_t>(ColIndex.size()); ++i )
				{
					uint64_t const low = ColIndex[i-1];
					uint64_t const high = ColIndex[i];
					uint64_t outp = low;
				
					// compact by keeping only distinct substrings	
					uint64_t numdif = 1;
					ColVec[outp++] = ColVec[low];
					for ( uint64_t j = low+1; j < high; ++j )
						if ( stypecomp(ColVec[j-1],ColVec[j]) )
						{
							numdif++;
							ColVec[outp++] = ColVec[j];
						}
					ColIndexCopy[i-1] = numdif;		
				}
				
				uint64_t outp = 0;
				for ( int64_t i = 0; i < static_cast<int64_t>(ColIndexCopy.size())-1; ++i )
				{
					uint64_t const low = ColIndex[i];
					uint64_t const cnt = ColIndexCopy[i];
					for ( uint64_t j = 0; j < cnt; ++j )
						ColVec[outp++] = ColVec[low+j];
					ColIndex[i] = ColIndexCopy[i];
				}
				ColIndex.prefixSums();

				if ( verbose )
					std::cerr << "time=" << rtc.getElapsedSeconds() << ", bs="
						<< static_cast<double>((LB.byteSize() + HH.byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColIndex.byteSize() + ColIndexCopy.byteSize() + ColVec.byteSize()))/C.byteSize() 
						<< ", reduced to "
						<< ColIndex[ColIndex.size()-1]
						<< "/"
						<< ColVec.size()
						<< ")";
						
				ColVec.resize(ColIndex[ColIndex.size()-1]);

				numcolnames = ColIndex[ColIndex.size()-1];
				numnoncolnames = NonColBV->rank1( NonColBV->size()-1);
				numsubnames = numnoncolnames + numcolnames;
				numsubbits = ::libmaus2::math::numbits(numsubnames);

				if ( verbose )
					std::cerr << "(shrinked ColVec, bs="
						<< static_cast<double>((LB.byteSize() + HH.byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColIndex.byteSize() + ColIndexCopy.byteSize() + ColVec.byteSize()))/C.byteSize() 
						<< ", sub names=" << numsubnames << ", sub bits=" << numsubbits
						<< ")";
				
				if ( verbose )
				{
					std::cerr << "(*** numcolnames=" << numcolnames << " ***)";
					std::cerr << "(*** numnoncolnames=" << numnoncolnames << " ***)";
					std::cerr << "(*** numsubnames=" << numsubnames << " ***)";
					std::cerr << "(*** numsubbits=" << numsubbits << " ***)";
					std::cerr << "(*** redast=" << redast << " ***)";
				}
			}
			
			typename HashReducedString<hash_type>::unique_ptr_type computeReducedString()
			{
				::libmaus2::timing::RealTimeClock rtc;
			
				if ( verbose )
				{
					rtc.start();
					std::cerr << "(Constructing reduced string...";
				}
				
				typename HashReducedString<hash_type>::unique_ptr_type HRS(new HashReducedString<hash_type>(redast,numsubnames,numcolnames,numnoncolnames,shortbits,longbits));
				HRS->namedict->setupSubColVec(*ColBV,*NonColBV,ColIndex);

				uint64_t const totalast = redast;
				uint64_t const astprintthres = (redast+printfrac-1)/printfrac;
				// uint64_t highptr = 0;
				uint64_t lastspos = 0;
				redast = 0;
				uint64_t lastprint = 0;
				for ( uint64_t i = ((1 < n)?stype.next1(1):n); i < n; i= ((i==(n-1))?(i+1):stype.next1(i+1)) )
				{
					assert ( stype.get(i) );

					uint64_t const len = (i-lastspos)+1;
					uint64_t const availc = std::min( (n-lastspos) , len );
					unsigned int const getbits = std::min( availc*b, hashbits );
					unsigned int const shiftbits = hashbits - getbits;
					uint64_t const hash = (::libmaus2::bitio::getBits ( C.D , lastspos*b, getbits ) << shiftbits) | LB[shiftbits];

					uint64_t name;

					// collision
					if ( ColBV->get(hash) )
					{
						uint64_t const colind = ColBV->rank1(hash)-1;
						uint64_t const collow = ColIndex[colind];
						uint64_t const colhigh = ColIndex[colind+1];
						std::pair < uint64_t const *, uint64_t const * > P = 
							std::equal_range(ColVec.begin()+collow,ColVec.begin()+colhigh,lastspos,stypecomp);
						assert ( P.first != P.second );
						assert ( P.second - P.first == 1 );
						
						name = (P.first - ColVec.begin()) + NonColBV->rank1(hash);					
						assert ( HRS->namedict->SubColVec->get(name) == true );

						HRS->namedict->setCol(name,lastspos);
					}
					// no collision
					else
					{
						name = ColIndex [ ColBV->rank1(hash) ] + NonColBV->rank1(hash)-1;
						assert ( HRS->namedict->SubColVec->get(name) == false );
						uint64_t const v = HH[hash] >> (uintbits1-len*b);
						assert ( v == ::libmaus2::bitio::getBits ( C.D , lastspos*b, len*b ) );
						
						HRS->namedict->setNonCol(name,v,len);
					}
					
					#if 0
					std::cerr << "***" << name << " -> " << lastspos << std::endl;
					#endif
					
					HRS->hreduced->set(redast,name);

					lastspos = i;
					redast++;
					
					if ( verbose )
					{
						if ( redast-lastprint >= astprintthres )
						{
							std::cerr << "(" << static_cast<double>(redast)/totalast << ")";
							lastprint = redast;
						}
					}
				}
				HRS->hreduced->set(redast++,0);
				
				if ( verbose )
					std::cerr << "(1)";

				if ( verbose )
					std::cerr << "(HRS->namedict->bs " << HRS->namedict->byteSize() << ")";
				
				if ( verbose )
					std::cerr << "time=" << rtc.getElapsedSeconds() << ", bs="
						<< static_cast<double>((LB.byteSize() + HH.byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColIndex.byteSize() + ColVec.byteSize() + HRS->byteSize()))/C.byteSize() 
						<< ")";
				
				return UNIQUE_PTR_MOVE(HRS);
			}

			typename HashLookupTable<hash_type>::unique_ptr_type computeHashLookupTable()
			{
				::libmaus2::timing::RealTimeClock rtc; rtc.start();
				
				// slenhist.print(std::cerr);
				std::map<uint64_t,uint64_t> const & slenhistmap = slenhist.get();

				uint64_t mini = 0, minsum = std::numeric_limits<uint64_t>::max(), minsumhigh = 0;
				for ( uint64_t i = 0; (1ull << (b*i)) * numsubbits <= (redast+1) * numsubbits ; ++i )
				{
					uint64_t sumhigh = 0;
					uint64_t sumlow = 0;
					for ( std::map<uint64_t,uint64_t>::const_iterator ita = slenhistmap.begin();
						ita != slenhistmap.end(); ++ita )
						if ( ita->first > i )
							sumhigh += ita->second;
						else
							sumlow += ita->second;
					
					// lookup table
					uint64_t const lowbits = ((1ull<<(b*i)) * numsubbits);
					// clear text names
					uint64_t const highbits =  sumhigh * numsubbits;
					// sum
					uint64_t const bits = lowbits + highbits;
					
					if ( bits <= minsum )
					{
						minsum = bits;
						mini = i;
						minsumhigh = sumhigh;
					}
					
				}

				if ( verbose )
					std::cerr << "(i=" << mini << " space=" << static_cast<double>(minsum) / (b*n) << ")";

				typename HashLookupTable<hash_type>::unique_ptr_type HLT (
					new HashLookupTable<hash_type>(mini,b,numsubbits,redast,minsumhigh,C,stype,
						numcolnames, numnoncolnames, shortbits, longbits) );
				HLT->namedict->setupSubColVec(*ColBV,*NonColBV,ColIndex);
				
				uint64_t const totalast = redast;
				uint64_t const astprintthres = (redast+printfrac-1)/printfrac;
				uint64_t highptr = 0;
				uint64_t lastspos = 0;
				redast = 0;
				uint64_t lastprint = 0;
				for ( uint64_t i = ((1 < n)?stype.next1(1):n); i < n; i= ((i==(n-1))?(i+1):stype.next1(i+1)) )
				{
					assert ( stype.get(i) );

					uint64_t const len = (i-lastspos)+1;
					uint64_t const availc = std::min( (n-lastspos) , len );
					unsigned int const getbits = std::min( availc*b, hashbits );
					unsigned int const shiftbits = hashbits - getbits;
					uint64_t const hash = (::libmaus2::bitio::getBits ( C.D , lastspos*b, getbits ) << shiftbits) | LB[shiftbits];

					uint64_t name;

					// collision
					if ( ColBV->get(hash) )
					{
						uint64_t const colind = ColBV->rank1(hash)-1;
						uint64_t const collow = ColIndex[colind];
						uint64_t const colhigh = ColIndex[colind+1];
						std::pair < uint64_t const *, uint64_t const * > P = 
							std::equal_range(ColVec.begin()+collow,ColVec.begin()+colhigh,lastspos,stypecomp);
						assert ( P.first != P.second );
						assert ( P.second - P.first == 1 );
						
						name = (P.first - ColVec.begin()) + NonColBV->rank1(hash);					
						assert ( HLT->namedict->SubColVec->get(name) == true );

						HLT->namedict->setCol(name,lastspos);
					}
					// no collision
					else
					{
						name = ColIndex [ ColBV->rank1(hash) ] + NonColBV->rank1(hash)-1;
						assert ( HLT->namedict->SubColVec->get(name) == false );
						uint64_t const v = HH[hash] >> (uintbits1-len*b);
						assert ( v == ::libmaus2::bitio::getBits ( C.D , lastspos*b, len*b ) );
						
						HLT->namedict->setNonCol(name,v,len);
					}
					
					#if 0
					std::cerr << "***" << name << " -> " << lastspos << std::endl;
					#endif
					
					if ( len <= mini )
					{
						HLT->S[len]->set(::libmaus2::bitio::getBits ( C.D , lastspos*b, len*b ), name);
						HLT->B->set(redast,true);
					}
					else
					{
						HLT->L->set(highptr++,name);
						HLT->B->set(redast,false);
					}

					lastspos = i;
					redast++;
					
					if ( verbose )
					{
						if ( redast-lastprint >= astprintthres )
						{
							std::cerr << "(" << static_cast<double>(redast)/totalast << ")";
							lastprint = redast;
						}
					}
				}
				if ( 1 <= mini )
				{
					HLT->S[1]->set(::libmaus2::bitio::getBits ( C.D , (n-1)*b, 1*b ), 0/*name*/);
					HLT->B->set(redast,true);
				}
				else
				{			
					HLT->L->set(highptr++,0/*name*/);
					HLT->B->set(redast,false);
				}
				HLT->B->setupIndex();
				redast++;
				
				if ( verbose )
					std::cerr << "(1)";

				if ( verbose )
					std::cerr << "(HLT->bs=" << HLT->byteSize() << ")";
				
				if ( verbose )
					std::cerr << "time=" << rtc.getElapsedSeconds() << ", bs="
						<< static_cast<double>((LB.byteSize() + HH.byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColBV->byteSize() + NonColBV->byteSize() + ColIndex.byteSize() + ColIndexCopy.byteSize() + ColVec.byteSize() + HLT->byteSize()))/C.byteSize()
						<< ")";
						;
						
				return UNIQUE_PTR_MOVE(HLT);
			}
		};
		
		template<typename array_type, typename hash_type /* = uint32_t */>
		typename HashReducedString<hash_type>::unique_ptr_type compareHashReductions(
			array_type const & C, 
			::libmaus2::bitio::IndexedBitVector const & stype,
			uint64_t const smallthres,
			bool const verbose = false,
			uint64_t const fraction = 8
		)
		{
			HashReduction<array_type,hash_type> HR(C,stype,smallthres,verbose,fraction);
			typename HashReducedString<hash_type>::unique_ptr_type HRS = HR.computeReducedString();
			typename HashLookupTable<hash_type>::unique_ptr_type HLT = HR.computeHashLookupTable();
			
			assert ( HRS->hreduced->size() == HLT->size() );
			if ( verbose )
				std::cerr << "(Comparing HRS and HLT...";
			for ( uint64_t i = 0; i < HRS->hreduced->size(); ++i )
				assert ( (*(HRS->hreduced))[i] == (*HLT)[i] );
			if ( verbose )
				std::cerr << "done)";
			
			return HRS;
		
		}

		template<typename array_type, typename hash_type /* =uint32_t */>
		typename HashReducedString<hash_type>::unique_ptr_type computeReducedStringHash(
			array_type const & C, 
			::libmaus2::bitio::IndexedBitVector const & stype,
			uint64_t const smallthres,
			bool const verbose = false,
			uint64_t const fraction = 8
		)
		{
			HashReduction<array_type,hash_type> HR(C,stype,smallthres,verbose,fraction);
			typename HashReducedString<hash_type>::unique_ptr_type HRS = HR.computeReducedString();
			return UNIQUE_PTR_MOVE(HRS);
		}

		template<typename array_type, typename hash_type /* =uint32_t */>
		typename HashLookupTable<hash_type>::unique_ptr_type computeReducedStringHashLookupTable(
			array_type const & C, 
			::libmaus2::bitio::IndexedBitVector const & stype,
			uint64_t const smallthres,
			bool const verbose = false,
			uint64_t const fraction = 8
		)
		{
			HashReduction<array_type,hash_type> HR(C,stype,smallthres,verbose,fraction);
			typename HashLookupTable<hash_type>::unique_ptr_type HLT = HR.computeHashLookupTable();
			return UNIQUE_PTR_MOVE(HLT);
		}
	}
}
#endif
