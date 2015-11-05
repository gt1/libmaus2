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

#if ! defined(LIBMAUS2_SORTING_SORTING_HPP)
#define LIBMAUS2_SORTING_SORTING_HPP

#include <iostream>
#include <algorithm>
#include <cassert>

namespace libmaus2
{
	namespace sorting
	{
		// #define QUICKSORT_DEBUG

		struct IntProjector
		{
			typedef int value_type;

			#if defined(QUICKSORT_DEBUG)
			template<typename type>
			static value_type proj(type & A, unsigned int i)
			{
				return A[i];
			}
			#endif

			template<typename type>
			static void swap(type & A, unsigned int i, unsigned int j)
			{
				std::swap(A[i],A[j]);
			}

			template<typename type>
			static bool comp(type & A, unsigned int i, unsigned int j)
			{
				return A[i] < A[j];
			}
		};

		template<typename type, typename ptype>
		unsigned int median(
			type & A,
			unsigned int left,
			unsigned int right
			)
		{
			assert ( right > left );

			unsigned int const l = left;
			unsigned int const r = right;
			unsigned int const m = (l+r)>>1;

			if ( ptype::comp(A,l,m) ) { if ( ptype::comp(A,l,r-1) ) { if ( ptype::comp(A,m,r-1) ) return m; else return r-1; } else { return l; } }
			else {  if ( ptype::comp(A,m,r-1) ) { if ( ptype::comp(A,l,r-1) ) return l; else return r-1; } else { return m; } }
		}


		#if defined(QUICKSORT_DEBUG)
		template<typename type, typename ptype>
		void print(type & A, unsigned int left, unsigned int right)
		{
			for ( unsigned int i = left; i < right; ++i )
			{
				std::cerr << ptype::proj(A,i);
				if ( i+1 < right )
					std::cerr << ";";
			}
		}
		#endif

		template<typename type, typename ptype>
		void shellsort(type & A, unsigned int left, unsigned int right)
		{
			unsigned int g = (right-left+1) >> 1;

			#if defined(QUICKSORT_DEBUG)
			std::cerr << "Input: "; print<type,ptype>(A,left,right); std::cerr << std::endl;
			#endif

			while ( true )
			{
				for (unsigned int i = left + g; i < right; ++i )
				{
					// insert i into previous sequence
					for ( int j = i - g; j >= static_cast<int>(left); j -= g )
						if ( ptype::comp(A,j+g,j) )
							ptype::swap(A,j,j+g);
						else
							break;
				}

				if ( g == 1 )
					break;

				g = (g+1) >> 1;
			}

			#if defined(QUICKSORT_DEBUG)
			std::cerr << "Output: "; print<type,ptype>(A,left,right); std::cerr << std::endl;
			#endif
		}

		template<typename type, typename ptype>
		void insertionsort(type & A, unsigned int left, unsigned int right)
		{
			#if defined(QUICKSORT_DEBUG)
			std::cerr << "Input: "; print<type,ptype>(A,left,right); std::cerr << std::endl;
			#endif

			for (unsigned int i = left + 1; i < right; ++i )
				for ( int j = i - 1; j >= static_cast<int>(left); --j )
					if ( ptype::comp(A,j+1,j) )
						ptype::swap(A,j,j+1);
					else
						break;

			#if defined(QUICKSORT_DEBUG)
			std::cerr << "Output: "; print<type,ptype>(A,left,right); std::cerr << std::endl;
			#endif
		}

		template<typename type, typename ptype>
		void quicksort(type & A, unsigned int left, unsigned int right)
		{
			assert ( right >= left );

			#if defined(QUICKSORT_DEBUG)
			std::cerr << "input: "; print<type,ptype>(A,left,right); std::cerr << std::endl;
			#endif

			if ( right - left < 2 )
				return;

			unsigned int const m = median<type,ptype>(A,left,right);

			#if defined(QUICKSORT_DEBUG)
			std::cerr << "pivot position: " << m << " pivot value " << ptype::proj(A,m) << std::endl;
			#endif

			// bring pivot element to front
			ptype::swap(A,left,m);

			#if defined(QUICKSORT_DEBUG)
			std::cerr << "pivot swap: "; print<type,ptype>(A,left,right); std::cerr << std::endl;
			#endif

			unsigned int l = left+1;
			unsigned int r = right-1;

			while ( true )
			{
				#if defined(QUICKSORT_DEBUG)
				std::cerr << "entering loop: "; print<type,ptype>(A,left,right); std::cerr << " l=" << l << " r=" << r << std::endl;
				#endif

				while ( l < right && !(ptype::comp(A,left,l)) )
					++l;
				while ( r > left && ptype::comp(A,left,r) )
					--r;

				if ( r < l )
				{
					#if defined(QUICKSORT_DEBUG)
					std::cerr << "Breaking loop, l=" << l << " r=" << r << std::endl;
					#endif
					break;
				}

				ptype::swap(A,l,r);
				l++;
				r--;
			}

			#if defined(QUICKSORT_DEBUG)
			std::cerr << "entering loop: "; print<type,ptype>(A,left,right); std::cerr << " l=" << l << " r=" << r << std::endl;
			#endif

			assert ( l > r );

			ptype::swap(A,left,l-1);

			#if defined(QUICKSORT_DEBUG)
			std::cerr << "swapped pivot to middle: "; print<type,ptype>(A,left,right); std::cerr << " l=" << l << " r=" << r << std::endl;
			#endif

			unsigned int const cutoff = 24;

			if ( (l-1)-left < cutoff )
				insertionsort<type,ptype>(A,left,l-1);
			else
				quicksort<type,ptype>(A,left,l-1);

			if ( right - l < cutoff )
				insertionsort<type,ptype>(A,l,right);
			else
				quicksort<type,ptype>(A,l,right);

			#if defined(QUICKSORT_DEBUG)
			std::cerr << "sorted: "; print<type,ptype>(A,left,right); std::cerr << std::endl;
			#endif
		}
	}
}
#endif
