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
#if ! defined(LIBMAUS2_LCS_NPNOTRACE_HPP)
#define LIBMAUS2_LCS_NPNOTRACE_HPP

#include <libmaus2/lcs/Aligner.hpp>

namespace libmaus2
{
	namespace lcs
	{
		template<typename _length_type = int32_t>
		struct NPNoTraceTemplate
		{
			typedef _length_type length_type;
			typedef NPNoTraceTemplate<length_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			struct NPElement
			{
				length_type offset;
				std::pair<length_type,length_type> mid;
			};

			libmaus2::autoarray::AutoArray<NPElement> DE;
			libmaus2::autoarray::AutoArray<NPElement> DO;

			uint64_t byteSize() const
			{
				return DE.byteSize() + DO.byteSize();
			}

			template<typename iter_a, typename iter_b, bool neg>
			static inline int64_t slide(iter_a a, iter_a const ae, iter_b b, iter_b const be, int64_t const offset)
			{
				a += offset;
				b += offset;

				iter_a ac = a;
				iter_b bc = b;

				if ( ae-ac < be-bc )
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

			void align(uint8_t const * a, size_t const l_a, uint8_t const * b, size_t const l_b)
			{
				np(a,a+l_a,b,b+l_b);
			}

			template<typename iter_a, typename iter_b>
			std::pair<int64_t,int64_t> np(iter_a const a, iter_a const ae, iter_b const b, iter_b const be, bool const self_check = false)
			{
				if ( self_check )
					return npTemplate<iter_a,iter_b,true>(a,ae,b,be);
				else
					return npTemplate<iter_a,iter_b,false>(a,ae,b,be);
			}

			template<typename iter_a, typename iter_b, bool self_check>
			std::pair<int64_t,int64_t> npTemplate(iter_a const a, iter_a const ae, iter_b const b, iter_b const be)
			{
				size_t const an = ae-a;
				size_t const bn = be-b;
				size_t const sn = std::max(an,bn);
				int64_t const sn2 = (sn>>1);
				int64_t const numdiag = (sn<<1)+1;
				int64_t const fdiag = (ae-a) - (be-b);
				int64_t const fdiagoff = std::min(ae-a,be-b);
				NPElement * DP = 0;
				NPElement * DN = 0;

				if ( !an || !bn )
					return std::pair<int64_t,int64_t>(0,0);

				if ( numdiag > static_cast<int64_t>(DE.size()) )
				{
					DE.resize(numdiag);
					DO.resize(numdiag);
				}

				DP = DE.begin() + sn;
				DN = DO.begin() + sn;

				if (
					!(( (static_cast<int64_t>(sn) + fdiag) >= 0 )
					&&
					( (static_cast<int64_t>(sn) + fdiag)  < static_cast<int64_t>(DE.size()) )
					&&
					( (static_cast<int64_t>(sn) + fdiag)  < static_cast<int64_t>(DO.size()) ))
				)
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "[E] ae-a=" << (ae-a) << std::endl;
					lme.getStream() << "[E] be-b=" << (be-b) << std::endl;
					lme.getStream() << "[E] sn=" << sn << std::endl;
					lme.getStream() << "[E] numdiag=" << numdiag << std::endl;
					lme.getStream() << "[E] fdiag=" << fdiag << std::endl;
					lme.finish();
					throw lme;
				}

				// diagonal containing bottom right of matrix
				std::pair<length_type,length_type> const Punset(
					std::numeric_limits<length_type>::min(),
					std::numeric_limits<length_type>::min()
				);

				DP[fdiag].offset = 0;
				DN[fdiag].offset = 0;
				DP[fdiag].mid = Punset;
				DN[fdiag].mid = Punset;

				// how far do we get without an error?
				{
					if ( (!self_check) || (a!=b) )
					{
						int const s = slide<iter_a,iter_b,false>(a,ae,b,be,0);

						DP[0].offset = s;
						if ( DP[0].offset >= sn2 )
							DP[0].mid = std::pair<length_type,length_type>(s,s);
						else
							DP[0].mid = Punset;
					}
					else
					{
						DP[0].offset = 0;
						DP[0].mid = Punset;
					}
				}

				int d = 1;
				if ( DP[fdiag].offset != fdiagoff )
				{
					// slide for diagonal -1
					{
						if ( (!self_check) || (a!=b+1) )
						{
							int const p = DP[0].offset;
							int const s = slide<iter_a,iter_b,true>(a,ae,b+1,be,p);
							DN[-1].offset = p + s;

							if ( DP[0].mid != Punset )
								DN[-1].mid = DP[0].mid;
							else if ( DN[-1].offset >= sn2 )
								DN[-1].mid = std::pair<length_type,length_type>((a-a)+s+p,(b+1-b)+s+p);
							else
								DN[-1].mid = Punset;
						}
						else
						{
							DN[-1].offset = -1;
							DN[-1].mid = Punset;

							if ( fdiag == -1 && DP[0].offset == fdiagoff )
								DN[fdiag].offset = fdiagoff;
						}
					}

					// slide for diagonal 0 after mismatch
					{
						if ( (!self_check) || (a!=b) )
						{
							int const p = DP[0].offset+1;
							int const s = slide<iter_a,iter_b,false>(a,ae,b,be,p);
							DN[ 0].offset = p + s;

							if ( DP[0].mid != Punset )
								DN[0].mid = DP[0].mid;
							else if ( DN[0].offset >= sn2 )
								DN[0].mid = std::pair<length_type,length_type>((a-a)+p+s,(b-b)+p+s);
							else
								DN[0].mid = Punset;
						}
						else
						{
							DN[ 0].offset = -1;
							DN[ 0].mid = Punset;

							if ( fdiag == 0 && DP[0].offset+1 == fdiagoff )
								DN[fdiag].offset = fdiagoff;
						}
					}

					// slide for diagonal 1
					{
						if ( (!self_check) || (a+1!=b) )
						{
							int const p = DP[0].offset;
							int const s = slide<iter_a,iter_b,false>(a+1,ae,b,be,p);
							DN[ 1].offset = p + s;

							if ( DP[0].mid != Punset )
								DN[1].mid = DP[0].mid;
							else if ( DN[1].offset >= sn2 )
								DN[1].mid = std::pair<length_type,length_type>((a+1-a)+p+s,(b-b)+p+s);
							else
								DN[1].mid = Punset;
						}
						else
						{
							DN[ 1].offset = -1;
							DN[ 1].mid = Punset;

							if ( fdiag == 1 && DP[0].offset == fdiagoff )
								DN[fdiag].offset = fdiagoff;
						}
					}
					d += 1;
					std::swap(DP,DN);
				}
				for ( ; DP[fdiag].offset != fdiagoff; ++d )
				{
					iter_a aa = a;
					iter_b bb = b + d;

					// extend to -d from -d+1
					{
						if ( (!self_check) || (aa!=bb) )
						{
							// extend below
							int const p = DP[-d+1].offset;
							int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);

							DN[-d].offset   = p + s;

							if ( DP[-d+1].mid != Punset )
								DN[-d].mid = DP[-d+1].mid;
							else if ( DN[-d].offset >= sn2 )
								DN[-d].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
							else
								DN[-d].mid = Punset;
						}
						else
						{
							// extend below
							DN[-d].offset   = -1;
							DN[-d].mid = Punset;

							if ( fdiag == -d && DP[-d+1].offset == fdiagoff )
								DN[fdiag].offset = fdiagoff;
						}

						bb -= 1;
					}

					// extend for -d+1 (try -d+1 and -d+2)
					{
						int const top  = DP[-d+2].offset;
						int const diag = DP[-d+1].offset;

						if ( (!self_check) || (aa!=bb) )
						{
							if ( diag+1 >= top )
							{
								int const p = diag+1;
								int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[-d+1].offset = p + s;

								if ( DP[-d+1].mid != Punset )
									DN[-d+1].mid = DP[-d+1].mid;
								else if ( DN[-d+1].offset >= sn2 )
									DN[-d+1].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
								else
									DN[-d+1].mid = Punset;
							}
							else
							{
								int const p = top;
								int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[-d+1].offset = p + s;

								if ( DP[-d+2].mid != Punset )
									DN[-d+1].mid = DP[-d+2].mid;
								else if ( DN[-d+1].offset >= sn2 )
									DN[-d+1].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
								else
									DN[-d+1].mid = Punset;
							}
						}
						else
						{
							if ( diag+1 >= top )
							{
								DN[-d+1].offset = -1;
								DN[-d+1].mid = Punset;

								if ( fdiag == -d+1 && diag+1 == fdiagoff )
									DN[fdiag].offset = fdiagoff;
							}
							else
							{
								DN[-d+1].offset = -1;
								DN[-d+1].mid = Punset;

								if ( fdiag == -d+1 && top == fdiagoff )
									DN[fdiag].offset = fdiagoff;
							}
						}

						bb -= 1;
					}

					for ( int di = -d+2; di < 0; ++di )
					{
						int const left = DP[di-1].offset;
						int const diag = DP[di].offset;
						int const top  = DP[di+1].offset;

						if ( (!self_check) || (aa!=bb) )
						{
							if ( diag >= left )
							{
								if ( diag+1 >= top )
								{
									int const p = diag+1;
									int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
									DN[di].offset = p + s;

									if ( DP[di].mid != Punset )
										DN[di].mid = DP[di].mid;
									else if ( DN[di].offset >= sn2 )
										DN[di].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[di].mid = Punset;
								}
								else
								{
									int const p = top;
									int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
									DN[di].offset = p + s;

									if ( DP[di+1].mid != Punset )
										DN[di].mid = DP[di+1].mid;
									else if ( DN[di].offset >= sn2 )
										DN[di].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[di].mid = Punset;
								}
							}
							else
							{
								if ( left+1 >= top )
								{
									int const p = left+1;
									int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
									DN[di].offset = p + s;

									if ( DP[di-1].mid != Punset )
										DN[di].mid = DP[di-1].mid;
									else if ( DN[di].offset >= sn2 )
										DN[di].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[di].mid = Punset;
								}
								else
								{
									int const p = top;
									int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
									DN[di].offset = p + s;

									if ( DP[di+1].mid != Punset )
										DN[di].mid = DP[di+1].mid;
									else if ( DN[di].offset >= sn2 )
										DN[di].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[di].mid = Punset;
								}
							}
						}
						else
						{
							if ( diag >= left )
							{
								if ( diag+1 >= top )
								{
									DN[di].offset = -1;
									DN[di].mid = Punset;

									if ( fdiag == di && diag+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[di].offset = -1;
									DN[di].mid = Punset;

									if ( fdiag == di && top == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
							}
							else
							{
								if ( left+1 >= top )
								{
									DN[di].offset = -1;
									DN[di].mid = Punset;

									if ( fdiag == di && left+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[di].offset = -1;
									DN[di].mid = Punset;

									if ( fdiag == di && top == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
							}

						}

						bb -= 1;
					}

					{
						int const left = DP[-1].offset;
						int const diag = DP[0].offset;
						int const top = DP[1].offset;

						if ( (!self_check) || (aa!=bb) )
						{
							if ( diag >= left )
							{
								if ( diag >= top )
								{
									int const p = diag+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[0].offset = p + s;

									if ( DP[0].mid != Punset )
										DN[0].mid = DP[0].mid;
									else if ( DN[0].offset >= sn2 )
										DN[0].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[0].mid = Punset;
								}
								else
								{
									int const p = top+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[0].offset = p + s;

									if ( DP[1].mid != Punset )
										DN[0].mid = DP[1].mid;
									else if ( DN[0].offset >= sn2 )
										DN[0].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[0].mid = Punset;
								}
							}
							else
							{
								if ( left >= top )
								{
									int const p = left+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[0].offset = p + s;

									if ( DP[-1].mid != Punset )
										DN[0].mid = DP[-1].mid;
									else if ( DN[0].offset >= sn2 )
										DN[0].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[0].mid = Punset;
								}
								else
								{
									int const p = top+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[0].offset = p + s;

									if ( DP[1].mid != Punset )
										DN[0].mid = DP[1].mid;
									else if ( DN[0].offset >= sn2 )
										DN[0].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[0].mid = Punset;
								}
							}
						}
						else
						{
							if ( diag >= left )
							{
								if ( diag >= top )
								{
									DN[0].offset = -1;
									DN[0].mid = Punset;

									if ( fdiag == 0 && diag+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[0].offset = -1;
									DN[0].mid = Punset;

									if ( fdiag == 0 && top+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
							}
							else
							{
								if ( left >= top )
								{
									DN[0].offset = -1;
									DN[0].mid = Punset;

									if ( fdiag == 0 && left+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[0].offset = -1;
									DN[0].mid = Punset;

									if ( fdiag == 0 && top+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
							}
						}

						aa += 1;
					}

					for ( int di = 1; di <= d-2 ; ++di )
					{
						int const left = DP[di-1].offset;
						int const diag = DP[di].offset;
						int const top  = DP[di+1].offset;

						if ( (!self_check) || (aa!=bb) )
						{
							if ( diag+1 >= left )
							{
								if ( diag >= top )
								{
									int const p = diag+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[di].offset = p + s;

									if ( DP[di].mid != Punset )
										DN[di].mid = DP[di].mid;
									else if ( DN[di].offset >= sn2 )
										DN[di].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[di].mid = Punset;
								}
								else
								{
									int const p = top+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[di].offset = p + s;

									if ( DP[di+1].mid != Punset )
										DN[di].mid = DP[di+1].mid;
									else if ( DN[di].offset >= sn2 )
										DN[di].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[di].mid = Punset;
								}
							}
							else
							{
								if ( left >= top+1 )
								{
									int const p = left;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[di].offset = p + s;

									if ( DP[di-1].mid != Punset )
										DN[di].mid = DP[di-1].mid;
									else if ( DN[di].offset >= sn2 )
										DN[di].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[di].mid = Punset;
								}
								else
								{
									int const p = top+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[di].offset = p + s;

									if ( DP[di+1].mid != Punset )
										DN[di].mid = DP[di+1].mid;
									else if ( DN[di].offset >= sn2 )
										DN[di].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
									else
										DN[di].mid = Punset;
								}
							}
						}
						else
						{
							if ( diag+1 >= left )
							{
								if ( diag >= top )
								{
									DN[di].offset = -1;
									DN[di].mid = Punset;

									if ( fdiag == di && diag+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[di].offset = -1;
									DN[di].mid = Punset;

									if ( fdiag == di && top+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
							}
							else
							{
								if ( left >= top+1 )
								{
									DN[di].offset = -1;
									DN[di].mid = Punset;

									if ( fdiag == di && left == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[di].offset = -1;
									DN[di].mid = Punset;

									if ( fdiag == di && top+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
							}
						}

						aa += 1;
					}

					{
						int const left = DP[d-2].offset;
						int const diag = DP[d-1].offset;

						if ( (!self_check) || (aa!=bb) )
						{
							if ( diag+1 >= left )
							{
								int const p = diag+1;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[ d-1].offset = p + s;

								if ( DP[d-1].mid != Punset )
									DN[ d-1].mid = DP[d-1].mid;
								else if ( DN[ d-1].offset >= sn2 )
									DN[ d-1].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
								else
									DN[ d-1].mid = Punset;
							}
							else
							{
								int const p = left;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[ d-1].offset = p + s;

								if ( DP[d-2].mid != Punset )
									DN[ d-1].mid = DP[d-2].mid;
								else if ( DN[ d-1].offset >= sn2 )
									DN[ d-1].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
								else
									DN[ d-1].mid = Punset;
							}
						}
						else
						{
							if ( diag+1 >= left )
							{
								DN[ d-1].offset = -1;
								DN[ d-1].mid = Punset;

								if ( fdiag == d-1 && diag+1 == fdiagoff )
									DN[fdiag].offset = fdiagoff;
							}
							else
							{
								DN[ d-1].offset = -1;
								DN[ d-1].mid = Punset;

								if ( fdiag == d-1 && left == fdiagoff )
									DN[fdiag].offset = fdiagoff;
							}

						}

						aa += 1;
					}

					{
						if ( (!self_check) || (aa!=bb) )
						{
							// extend above
							int const p = DP[ d-1].offset;
							int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
							DN[d  ].offset = p + s;

							if ( DP[ d-1].mid != Punset )
								DN[d  ].mid = DP[ d-1].mid;
							else if ( DN[d  ].offset >= sn2 )
								DN[d  ].mid = std::pair<length_type,length_type>((aa-a)+s+p,(bb-b)+s+p);
							else
								DN[d  ].mid = Punset;
						}
						else
						{
							// extend above
							DN[d  ].offset = -1;
							DN[d  ].mid = Punset;

							if ( fdiag == d && DP[ d-1].offset == fdiagoff )
								DN[fdiag].offset = fdiagoff;
						}
					}

					std::swap(DP,DN);
				}

				// int64_t const ed = static_cast<int64_t>(d)-1;

				if ( DP[fdiag].mid == Punset )
					return std::pair<length_type,length_type>(0,0);
				else
					return DP[fdiag].mid;
			}
		};
		typedef NPNoTraceTemplate<> NPNoTrace;
	}
}
#endif
