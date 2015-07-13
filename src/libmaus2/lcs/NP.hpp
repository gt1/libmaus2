/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_LCS_NP_HPP)
#define LIBMAUS2_LCS_NP_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct NP
		{
			struct NPElement
			{
				int offset;
			};

			libmaus2::autoarray::AutoArray<NPElement> DE;
			libmaus2::autoarray::AutoArray<NPElement> DO;
			
			template<typename iter_a, typename iter_b, bool neg>
			static inline int slide(iter_a a, iter_a const ae, iter_b b, iter_a const be, int const offset)
			{
				a += offset;
				b += offset;

				iter_a ac = a;
				iter_b bc = b;
				
				if ( ae-ac < be-bc )
				{
					while ( ac < ae && *ac == *bc )
						++ac, ++bc;
				}
				else
				{
					while ( bc < be && *ac == *bc )
						++ac, ++bc;
				}

				if ( neg )
					return ac-a;
				else
					return bc-b;
			}

			template<typename iter_a, typename iter_b>
			int np(iter_a const a, iter_a const ae, iter_b const b, iter_b const be)
			{
				size_t const an = ae-a;
				size_t const bn = be-b;
				size_t const sn = std::max(an,bn);
				int const numdiag = (sn<<1)+1;
				
				if ( numdiag > static_cast<int>(DE.size()) )
				{
					DE.resize(numdiag);
					DO.resize(numdiag);
				}
						
				NPElement * DP = DE.begin() + sn;
				NPElement * DN = DO.begin() + sn;

				// diagonal containing bottom right of matrix
				int const fdiag = (ae-a) - (be-b);
				int const fdiagoff = std::min(ae-a,be-b);
				DP[fdiag].offset = 0;
				DN[fdiag].offset = 0;

				// how far do we get without an error?
				DP[0].offset = slide<iter_a,iter_b,false>(a,ae,b,be,0);
				
				int d = 1;
				if ( DP[fdiag].offset != fdiagoff )
				{
					{
						int const p = DP[0].offset;
						int const s = slide<iter_a,iter_b,true>(a,ae,b+1,be,p);
						DN[-1].offset = p + s;
					}
					{
						int const p = DP[0].offset+1;
						int const s = slide<iter_a,iter_b,false>(a,ae,b,be,p);
						DN[ 0].offset = p + s;
					}
					{
						int const p = DP[0].offset;
						int const s = slide<iter_a,iter_b,false>(a+1,ae,b,be,p);
						DN[ 1].offset = p + s;
					}
					d += 1;
					std::swap(DP,DN);
				}
				for ( ; DP[fdiag].offset != fdiagoff; ++d )
				{
					iter_a aa = a;
					iter_b bb = b + d;
				
					{
						// extend below
						int const p = DP[-d+1].offset;
						int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
						DN[-d].offset   = p + s;
						bb -= 1;
					}
					
					{
						int const top  = DP[-d+2].offset;
						int const diag = DP[-d+1].offset;
						
						if ( diag+1 >= top )
						{
							int const p = diag+1;
							int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p); 
							DN[-d+1].offset = p + s;
						}
						else
						{
							int const p = top;
							int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
							DN[-d+1].offset = p + s;
						}
						
						bb -= 1;
					}

					for ( int di = -d+2; di < 0; ++di )
					{
						int const left = DP[di-1].offset;
						int const diag = DP[di].offset;
						int const top  = DP[di+1].offset;
						
						if ( diag >= left )
						{
							if ( diag+1 >= top )
							{
								int const p = diag+1;
								int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
							else
							{
								int const p = top;
								int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
						}
						else
						{
							if ( left+1 >= top )
							{
								int const p = left+1;
								int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
							else
							{
								int const p = top;
								int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
						}
						
						bb -= 1;
					}

					{
						int const left = DP[-1].offset;
						int const diag = DP[0].offset;
						int const top = DP[1].offset;
						
						if ( diag >= left )
						{
							if ( diag >= top )
							{
								int const p = diag+1;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[0].offset = p + s;
							}
							else
							{
								int const p = top+1;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[0].offset = p + s;
							}
						}
						else
						{
							if ( left >= top )
							{
								int const p = left+1;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[0].offset = p + s;
							}
							else
							{
								int const p = top+1;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[0].offset = p + s;
							}
						}
						
						aa += 1;
					}
					
					for ( int di = 1; di <= d-2 ; ++di )
					{
						int const left = DP[di-1].offset;
						int const diag = DP[di].offset;
						int const top  = DP[di+1].offset;
						
						if ( diag+1 >= left )
						{
							if ( diag >= top )
							{
								int const p = diag+1;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
							else
							{
								int const p = top+1;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
						}
						else
						{
							if ( left >= top+1 )
							{
								int const p = left;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
							else
							{
								int const p = top+1;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
						}
						
						aa += 1;
					}

					{
						int const left = DP[d-2].offset;
						int const diag = DP[d-1].offset;
						
						if ( diag+1 >= left )
						{
							int const p = diag+1;
							int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
							DN[ d-1].offset = p + s;
						}
						else
						{
							int const p = left;
							int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
							DN[ d-1].offset = p + s;
						}
						
						aa += 1;
					}
					
					{
						// extend above
						int const p = DP[ d-1].offset;
						int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
						DN[d  ].offset = p + s; 
					}
					
					std::swap(DP,DN);
				}
				
				// std::cerr << "d=" << d << std::endl;
				return d-1;
			}
		};
	}
}
#endif
