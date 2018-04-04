/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if ! defined(LIBMAUS2_LCS_NPLNOTRACE_HPP)
#define LIBMAUS2_LCS_NPLNOTRACE_HPP

#include <libmaus2/lcs/Aligner.hpp>

namespace libmaus2
{
	namespace lcs
	{
		/**
		 * NP variant aligning until end of one string is reached (instead of both as for NP)
		 **/
		struct NPLNoTrace
		{
			typedef NPLNoTrace this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef int32_t index_type;

			struct NPElement
			{
				index_type offset;
			};

			libmaus2::autoarray::AutoArray<NPElement> DE;
			libmaus2::autoarray::AutoArray<NPElement> DO;

			uint64_t byteSize() const
			{
				return DE.byteSize() + DO.byteSize();
			}

			template<typename iter_a, typename iter_b, bool neg>
			static inline index_type slide(iter_a a, iter_a const ae, iter_b b, iter_b const be, index_type const offset)
			{
				a += offset;
				b += offset;

				iter_a ac = a;
				iter_b bc = b;

				if ( (ae-ac) < (be-bc) )
					while ( ac < ae && *ac == *bc )
						++ac, ++bc;
				else
					while ( bc < be && *ac == *bc )
						++ac, ++bc;

				if ( neg )
					return ac-a;
				else
					return bc-b;
			}

			struct ReturnValue
			{
				uint64_t alen;
				uint64_t blen;
				uint64_t ed;

				ReturnValue() {}
				ReturnValue(
					uint64_t const ralen,
					uint64_t const rblen,
					uint64_t const red
				) : alen(ralen), blen(rblen), ed(red) {}

				std::string toString() const
				{
					std::ostringstream ostr;
					ostr << "ReturnValue(" << alen << "," << blen << "," << ed << ")";
					return ostr.str();
				}
			};

			template<typename iter_a, typename iter_b>
			ReturnValue np(iter_a const a, iter_a const ae, iter_b const b, iter_b const be, index_type const maxd = std::numeric_limits<index_type>::max())
			{
				assert ( ae-a >= 0 );
				assert ( be-b >= 0 );

				size_t const an = ae-a;
				size_t const bn = be-b;

				if ( ! an || ! bn )
				{
					return ReturnValue(an,bn,std::max(an,bn));
				}

				size_t const sn = std::max(an,bn);
				// number of diagonals
				index_type const numdiag = (sn<<1)+1;

				if ( numdiag > static_cast<index_type>(DE.size()) )
				{
					DE.resize(numdiag);
					DO.resize(numdiag);
				}

				NPElement * DP = DE.begin() + sn;
				NPElement * DN = DO.begin() + sn;

				// diagonal containing bottom right of matrix
				int64_t fdiag = std::numeric_limits<int64_t>::max();

				// how far do we get without an error?
				{
					index_type const s = slide<iter_a,iter_b,false>(a,ae,b,be,0);
					DP[0].offset = s;
				}

				if ( DP[0].offset >= static_cast<int64_t>(std::min(an,bn)) )
				{
					assert ( DP[0].offset == static_cast<int64_t>(std::min(an,bn)) );

					uint64_t const n = std::min(an,bn);
					return ReturnValue(n,n,0);
				}

				index_type d = 1;

				assert ( DP[0].offset < static_cast<int64_t>(std::min(an,bn)) );

				{
					index_type const p = DP[0].offset;
					index_type const s = slide<iter_a,iter_b,true>(a,ae,b+1,be,p);
					DN[-1].offset = p + s;
				}
				{
					index_type const p = DP[0].offset+1;
					index_type const s = slide<iter_a,iter_b,false>(a,ae,b,be,p);
					DN[ 0].offset = p + s;
				}
				{
					index_type const p = DP[0].offset;
					index_type const s = slide<iter_a,iter_b,false>(a+1,ae,b,be,p);
					DN[ 1].offset = p + s;
				}
				d += 1;
				std::swap(DP,DN);

				for ( ; d < maxd ; ++d )
				{
					// std::cerr << "d=" << d << std::endl;

					bool done = false;

					for ( int64_t di = -d+1; di <= d-1; ++di )
					{
						int64_t const apos = std::max( di,static_cast<int64_t>(0))+DP[di].offset;
						int64_t const bpos = std::max(-di,static_cast<int64_t>(0))+DP[di].offset;
						assert ( apos >= 0 );
						assert ( bpos >= 0 );

						assert ( static_cast< uint64_t >(apos) <= an );
						assert ( static_cast< uint64_t >(bpos) <= bn );

						// std::cerr << "d=" << d << " di=" << di << " apos=" << apos << " bpos=" << bpos << " an=" << an << " bn=" << bn << std::endl;

						if (
							static_cast< uint64_t >(apos) == an
							||
							static_cast< uint64_t >(bpos) == bn
						)
						{
							fdiag = di;
							done = true;
						}
					}
					if ( done )
					{
						break;
					}

					iter_a aa = a;
					iter_b bb = b + d;

					{
						// extend below
						index_type const p = DP[-d+1].offset;
						index_type const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
						DN[-d].offset   = p + s;

						bb -= 1;
					}

					{
						index_type const top  = DP[-d+2].offset;
						index_type const diag = DP[-d+1].offset;

						if ( diag+1 >= top )
						{
							index_type const p = diag+1;
							index_type const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
							DN[-d+1].offset = p + s;
						}
						else
						{
							index_type const p = top;
							index_type const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
							DN[-d+1].offset = p + s;
						}

						bb -= 1;
					}

					for ( index_type di = -d+2; di < 0; ++di )
					{
						index_type const left = DP[di-1].offset;
						index_type const diag = DP[di].offset;
						index_type const top  = DP[di+1].offset;

						if ( diag >= left )
						{
							if ( diag+1 >= top )
							{
								index_type const p = diag+1;
								index_type const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
							else
							{
								index_type const p = top;
								index_type const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
						}
						else
						{
							if ( left+1 >= top )
							{
								index_type const p = left+1;
								index_type const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
							else
							{
								index_type const p = top;
								index_type const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
						}

						bb -= 1;
					}

					{
						index_type const left = DP[-1].offset;
						index_type const diag = DP[0].offset;
						index_type const top = DP[1].offset;

						if ( diag >= left )
						{
							if ( diag >= top )
							{
								index_type const p = diag+1;
								index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[0].offset = p + s;
							}
							else
							{
								index_type const p = top+1;
								index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[0].offset = p + s;
							}
						}
						else
						{
							if ( left >= top )
							{
								index_type const p = left+1;
								index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[0].offset = p + s;
							}
							else
							{
								index_type const p = top+1;
								index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[0].offset = p + s;
							}
						}

						aa += 1;
					}

					for ( index_type di = 1; di <= d-2 ; ++di )
					{
						index_type const left = DP[di-1].offset;
						index_type const diag = DP[di].offset;
						index_type const top  = DP[di+1].offset;

						if ( diag+1 >= left )
						{
							if ( diag >= top )
							{
								index_type const p = diag+1;
								index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
							else
							{
								index_type const p = top+1;
								index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
						}
						else
						{
							if ( left >= top+1 )
							{
								index_type const p = left;
								index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
							else
							{
								index_type const p = top+1;
								index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[di].offset = p + s;
							}
						}

						aa += 1;
					}

					{
						index_type const left = DP[d-2].offset;
						index_type const diag = DP[d-1].offset;

						if ( diag+1 >= left )
						{
							index_type const p = diag+1;
							index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
							DN[ d-1].offset = p + s;
						}
						else
						{
							index_type const p = left;
							index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
							DN[ d-1].offset = p + s;
						}

						aa += 1;
					}

					{
						// extend above
						index_type const p = DP[ d-1].offset;
						index_type const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
						DN[d  ].offset = p + s;
					}

					std::swap(DP,DN);
				}

				index_type const ed = d-1;

				int64_t const apos = std::max( fdiag,static_cast<int64_t>(0))+DP[fdiag].offset;
				int64_t const bpos = std::max(-fdiag,static_cast<int64_t>(0))+DP[fdiag].offset;
				assert ( apos >= 0 );
				assert ( bpos >= 0 );

				return ReturnValue(apos,bpos,ed);
			}
		};
	}
}
#endif
