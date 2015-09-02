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

#include <libmaus2/lcs/Aligner.hpp>

namespace libmaus2
{
	namespace lcs
	{
		struct NP : public libmaus2::lcs::Aligner, public libmaus2::lcs::AlignmentTraceContainer
		{
			typedef NP this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			struct NPElement
			{
				int offset;
				int id;
			};
			
			enum trace_op {
				trace_op_none,
				trace_op_diag,
				trace_op_ins,
				trace_op_del
			};
			struct TraceElement
			{
				int prev;
				int slide;
				trace_op op;
			};

			libmaus2::autoarray::AutoArray<NPElement> DE;
			libmaus2::autoarray::AutoArray<NPElement> DO;
			libmaus2::autoarray::AutoArray<TraceElement,libmaus2::autoarray::alloc_type_c> trace;
			
			template<typename iter_a, typename iter_b, bool neg>
			static inline int slide(iter_a a, iter_a const ae, iter_b b, iter_a const be, int const offset)
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
			
			AlignmentTraceContainer const & getTraceContainer() const
			{
				return *this;
			}

			template<typename iter_a, typename iter_b>
			int np(iter_a const a, iter_a const ae, iter_b const b, iter_b const be, bool const self_check = false)
			{
				if ( self_check )
					return npTemplate<iter_a,iter_b,true>(a,ae,b,be);
				else
					return npTemplate<iter_a,iter_b,false>(a,ae,b,be);
			}

			template<typename iter_a, typename iter_b, bool self_check>
			int npTemplate(iter_a const a, iter_a const ae, iter_b const b, iter_b const be)
			{
				size_t const an = ae-a;
				size_t const bn = be-b;
				
				if ( ! an )
				{
					if ( AlignmentTraceContainer::capacity() <= bn )
						AlignmentTraceContainer::resize(bn);
					AlignmentTraceContainer::reset();
					AlignmentTraceContainer::ta -= bn;
					std::fill(AlignmentTraceContainer::ta,AlignmentTraceContainer::te,STEP_INS);
					return bn;
				}
				else if ( ! bn )
				{
					if ( AlignmentTraceContainer::capacity() <= an )
						AlignmentTraceContainer::resize(an);
					AlignmentTraceContainer::reset();
					AlignmentTraceContainer::ta -= an;
					std::fill(AlignmentTraceContainer::ta,AlignmentTraceContainer::te,STEP_DEL);
					return an;
				}
				
				size_t const sn = std::max(an,bn);
				int const numdiag = (sn<<1)+1;
				int id = 0;
				
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
				{
					if ( trace.size() < id+1 )
						trace.resize(id+1);
				
					if ( (!self_check) || (a!=b) )
					{
						int const s = slide<iter_a,iter_b,false>(a,ae,b,be,0);
						DP[0].offset = s;
						int const nodeid = id++;
						assert ( nodeid < trace.size() );
						DP[0].id = nodeid;
						trace[nodeid].prev = 0;
						trace[nodeid].slide = s;
						trace[nodeid].op = trace_op_none;
					}
					else
					{
						DP[0].offset = 0;
						int const nodeid = id++;
						assert ( nodeid < trace.size() );
						DP[0].id = nodeid;
						trace[nodeid].prev = 0;
						trace[nodeid].slide = 0;
						trace[nodeid].op = trace_op_none;
					}
				}
				
				int d = 1;
				if ( DP[fdiag].offset != fdiagoff )
				{
					if ( trace.size() < id+3 )
						trace.resize(id+3);
						
					{
						if ( (!self_check) || (a!=b+1) )
						{
							int const p = DP[0].offset;
							int const s = slide<iter_a,iter_b,true>(a,ae,b+1,be,p);
							DN[-1].offset = p + s;
							int const nodeid = id++;
							assert ( nodeid < trace.size() );
							DN[-1].id = nodeid;
							trace[nodeid].prev = DP[0].id;
							trace[nodeid].slide = s;
							trace[nodeid].op = trace_op_ins;
						}
						else
						{
							DN[-1].offset = -1;
							int const nodeid = id++;
							assert ( nodeid < trace.size() );
							DN[-1].id = nodeid;
							trace[nodeid].prev = DP[0].id;
							trace[nodeid].slide = 0;
							trace[nodeid].op = trace_op_ins;
							
							if ( fdiag == -1 && DP[0].offset == fdiagoff )
								DN[fdiag].offset = fdiagoff;
						}
					}
					{
						if ( (!self_check) || (a!=b) )
						{
							int const p = DP[0].offset+1;
							int const s = slide<iter_a,iter_b,false>(a,ae,b,be,p);
							DN[ 0].offset = p + s;
							int const nodeid = id++;
							assert ( nodeid < trace.size() );
							DN[ 0].id = nodeid;
							trace[nodeid].prev = DP[0].id;
							trace[nodeid].slide = s;
							trace[nodeid].op = trace_op_diag;
						}
						else
						{
							DN[ 0].offset = -1;
							int const nodeid = id++;
							assert ( nodeid < trace.size() );
							DN[ 0].id = nodeid;
							trace[nodeid].prev = DP[0].id;
							trace[nodeid].slide = 0;
							trace[nodeid].op = trace_op_diag;

							if ( fdiag == 0 && DP[0].offset+1 == fdiagoff )
								DN[fdiag].offset = fdiagoff;
						}
					}
					{
						if ( (!self_check) || (a+1!=b) )
						{
							int const p = DP[0].offset;
							int const s = slide<iter_a,iter_b,false>(a+1,ae,b,be,p);
							DN[ 1].offset = p + s;
							int const nodeid = id++;
							assert ( nodeid < trace.size() );
							DN[ 1].id = nodeid;
							trace[nodeid].prev = DP[0].id;
							trace[nodeid].slide = s;
							trace[nodeid].op = trace_op_del;
						}
						else
						{						
							DN[ 1].offset = -1;
							int const nodeid = id++;
							assert ( nodeid < trace.size() );
							DN[ 1].id = nodeid;
							trace[nodeid].prev = DP[0].id;
							trace[nodeid].slide = 0;
							trace[nodeid].op = trace_op_del;

							if ( fdiag == 1 && DP[0].offset == fdiagoff )
								DN[fdiag].offset = fdiagoff;
						}
					}
					d += 1;
					std::swap(DP,DN);
				}
				for ( ; DP[fdiag].offset != fdiagoff; ++d )
				{
					if ( trace.size() < id + (2*d+1) )
					{
						static uint64_t const cnt = 11;
						static uint64_t const div = 10;
						uint64_t const mult = (trace.size() * cnt) / div;
						uint64_t const reqsize = id + (2*d+1);
						uint64_t const newsize = std::max(mult,reqsize);
						trace.resize(newsize);
					}
										
					iter_a aa = a;
					iter_b bb = b + d;
				
					{
						if ( (!self_check) || (aa!=bb) )
						{
							// extend below
							int const p = DP[-d+1].offset;
							int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
							DN[-d].offset   = p + s;

							int const nodeid = id++;
							assert ( nodeid < trace.size() );
							DN[-d].id = nodeid;
							trace[nodeid].prev = DP[-d+1].id;
							trace[nodeid].slide = s;
							trace[nodeid].op = trace_op_ins;
						}
						else
						{
							// extend below
							DN[-d].offset   = -1;
							int const nodeid = id++;
							assert ( nodeid < trace.size() );
							DN[-d].id = nodeid;
							trace[nodeid].prev = DP[-d+1].id;
							trace[nodeid].slide = 0;
							trace[nodeid].op = trace_op_ins;						

							if ( fdiag == -d && DP[-d+1].offset == fdiagoff )
								DN[fdiag].offset = fdiagoff;
						}

						bb -= 1;
					}
					
					{
						int const top  = DP[-d+2].offset;
						int const diag = DP[-d+1].offset;

						int const nodeid = id++;
						assert ( nodeid < trace.size() );
						DN[-d+1].id = nodeid;
						
						if ( (!self_check) || (aa!=bb) )
						{
							if ( diag+1 >= top )
							{
								int const p = diag+1;
								int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p); 
								DN[-d+1].offset = p + s;
								trace[nodeid].prev = DP[-d+1].id;
								trace[nodeid].slide = s;
								trace[nodeid].op = trace_op_diag;
							}
							else
							{
								int const p = top;
								int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
								DN[-d+1].offset = p + s;
								trace[nodeid].prev = DP[-d+2].id;
								trace[nodeid].slide = s;
								trace[nodeid].op = trace_op_ins;
							}
						}
						else
						{
							if ( diag+1 >= top )
							{
								DN[-d+1].offset = -1;
								trace[nodeid].prev = DP[-d+1].id;
								trace[nodeid].slide = 0;
								trace[nodeid].op = trace_op_diag;

								if ( fdiag == -d+1 && diag+1 == fdiagoff )
									DN[fdiag].offset = fdiagoff;
							}
							else
							{
								DN[-d+1].offset = -1;
								trace[nodeid].prev = DP[-d+2].id;
								trace[nodeid].slide = 0;
								trace[nodeid].op = trace_op_ins;

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

						int const nodeid = id++;
						assert ( nodeid < trace.size() );
						DN[di].id = nodeid;
						
						if ( (!self_check) || (aa!=bb) )
						{
							if ( diag >= left )
							{
								if ( diag+1 >= top )
								{
									int const p = diag+1;
									int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
									DN[di].offset = p + s;
									trace[nodeid].prev = DP[di].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_diag;
								}
								else
								{
									int const p = top;
									int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
									DN[di].offset = p + s;
									trace[nodeid].prev = DP[di+1].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_ins;
								}
							}
							else
							{
								if ( left+1 >= top )
								{
									int const p = left+1;
									int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
									DN[di].offset = p + s;
									trace[nodeid].prev = DP[di-1].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_del;
								}
								else
								{
									int const p = top;
									int const s = slide<iter_a,iter_b,true>(aa,ae,bb,be,p);
									DN[di].offset = p + s;
									trace[nodeid].prev = DP[di+1].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_ins;
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
									trace[nodeid].prev = DP[di].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_diag;
									
									if ( fdiag == di && diag+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[di].offset = -1;
									trace[nodeid].prev = DP[di+1].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_ins;

									if ( fdiag == di && top == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
							}
							else
							{
								if ( left+1 >= top )
								{
									DN[di].offset = -1;
									trace[nodeid].prev = DP[di-1].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_del;

									if ( fdiag == di && left+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[di].offset = -1;
									trace[nodeid].prev = DP[di+1].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_ins;

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

						int const nodeid = id++;
						assert ( nodeid < trace.size() );
						DN[0].id = nodeid;
						
						if ( (!self_check) || (aa!=bb) )
						{
							if ( diag >= left )
							{
								if ( diag >= top )
								{
									int const p = diag+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[0].offset = p + s;
									trace[nodeid].prev = DP[0].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_diag;
								}
								else
								{
									int const p = top+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[0].offset = p + s;
									trace[nodeid].prev = DP[1].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_ins;
								}
							}
							else
							{
								if ( left >= top )
								{
									int const p = left+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[0].offset = p + s;
									trace[nodeid].prev = DP[-1].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_del;
								}
								else
								{
									int const p = top+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[0].offset = p + s;
									trace[nodeid].prev = DP[1].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_ins;
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
									trace[nodeid].prev = DP[0].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_diag;

									if ( fdiag == 0 && diag+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[0].offset = -1;
									trace[nodeid].prev = DP[1].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_ins;

									if ( fdiag == 0 && top+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
							}
							else
							{
								if ( left >= top )
								{
									DN[0].offset = -1;
									trace[nodeid].prev = DP[-1].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_del;

									if ( fdiag == 0 && left+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[0].offset = -1;
									trace[nodeid].prev = DP[1].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_ins;

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

						int const nodeid = id++;
						assert ( nodeid < trace.size() );
						DN[di].id = nodeid;
						
						if ( (!self_check) || (aa!=bb) )
						{						
							if ( diag+1 >= left )
							{
								if ( diag >= top )
								{
									int const p = diag+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[di].offset = p + s;
									trace[nodeid].prev = DP[di].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_diag;
								}
								else
								{
									int const p = top+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[di].offset = p + s;
									trace[nodeid].prev = DP[di+1].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_ins;
								}
							}
							else
							{
								if ( left >= top+1 )
								{
									int const p = left;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[di].offset = p + s;
									trace[nodeid].prev = DP[di-1].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_del;
								}
								else
								{
									int const p = top+1;
									int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
									DN[di].offset = p + s;
									trace[nodeid].prev = DP[di+1].id;
									trace[nodeid].slide = s;
									trace[nodeid].op = trace_op_ins;
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
									trace[nodeid].prev = DP[di].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_diag;

									if ( fdiag == di && diag+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[di].offset = -1;
									trace[nodeid].prev = DP[di+1].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_ins;

									if ( fdiag == di && top+1 == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
							}
							else
							{
								if ( left >= top+1 )
								{
									DN[di].offset = -1;
									trace[nodeid].prev = DP[di-1].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_del;

									if ( fdiag == di && left == fdiagoff )
										DN[fdiag].offset = fdiagoff;
								}
								else
								{
									DN[di].offset = -1;
									trace[nodeid].prev = DP[di+1].id;
									trace[nodeid].slide = 0;
									trace[nodeid].op = trace_op_ins;

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

						int const nodeid = id++;
						assert ( nodeid < trace.size() );
						DN[d-1].id = nodeid;
						
						if ( (!self_check) || (aa!=bb) )
						{						
							if ( diag+1 >= left )
							{
								int const p = diag+1;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[ d-1].offset = p + s;
								trace[nodeid].prev = DP[d-1].id;
								trace[nodeid].slide = s;
								trace[nodeid].op = trace_op_diag;
							}
							else
							{
								int const p = left;
								int const s = slide<iter_a,iter_b,false>(aa,ae,bb,be,p);
								DN[ d-1].offset = p + s;
								trace[nodeid].prev = DP[d-2].id;
								trace[nodeid].slide = s;
								trace[nodeid].op = trace_op_del;
							}
						}
						else
						{
							if ( diag+1 >= left )
							{
								DN[ d-1].offset = -1;
								trace[nodeid].prev = DP[d-1].id;
								trace[nodeid].slide = 0;
								trace[nodeid].op = trace_op_diag;

								if ( fdiag == d-1 && diag+1 == fdiagoff )
									DN[fdiag].offset = fdiagoff;
							}
							else
							{
								DN[ d-1].offset = -1;
								trace[nodeid].prev = DP[d-2].id;
								trace[nodeid].slide = 0;
								trace[nodeid].op = trace_op_del;

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
							int const nodeid = id++;
							assert ( nodeid < trace.size() );
							DN[d].id = nodeid;
							trace[nodeid].prev = DP[ d-1].id;
							trace[nodeid].slide = s;
							trace[nodeid].op = trace_op_del;
						}
						else
						{
							// extend above
							DN[d  ].offset = -1; 
							int const nodeid = id++;
							assert ( nodeid < trace.size() );
							DN[d].id = nodeid;
							trace[nodeid].prev = DP[ d-1].id;
							trace[nodeid].slide = 0;
							trace[nodeid].op = trace_op_del;						

							if ( fdiag == d && DP[ d-1].offset == fdiagoff )
								DN[fdiag].offset = fdiagoff;
						}
					}
					
					std::swap(DP,DN);
				}
				
				int const ed = d-1;
								
				if ( AlignmentTraceContainer::capacity() <= std::min(an,bn)+ed )
					AlignmentTraceContainer::resize(std::min(an,bn)+ed);
				AlignmentTraceContainer::reset();
				
				int tid = DP[fdiag].id;
				while ( trace[tid].op != trace_op_none )
				{
					for ( int i = 0; i < trace[tid].slide; ++i )
						*(--AlignmentTraceContainer::ta) = STEP_MATCH;
					// std::cerr << "(" << trace[tid].slide << ")";
					
					switch ( trace[tid].op )
					{
						case trace_op_diag:
							*(--AlignmentTraceContainer::ta) = STEP_MISMATCH;
							//std::cerr << "diag";
							break;
						case trace_op_ins:
							*(--AlignmentTraceContainer::ta) = STEP_INS;
							//std::cerr << "ins";
							break;
						case trace_op_del:
							*(--AlignmentTraceContainer::ta) = STEP_DEL;
							//std::cerr << "del";
							break;
						case trace_op_none:
							break;
					}
					tid = trace[tid].prev;
				}
				
				for ( int i = 0; i < trace[tid].slide; ++i )
					*(--AlignmentTraceContainer::ta) = STEP_MATCH;
				//std::cerr << "(" << trace[tid].slide << ")";
				//std::cerr << std::endl;
				
				// std::cerr << "d=" << d << std::endl;
				return ed;
			}
		};
	}
}
#endif
