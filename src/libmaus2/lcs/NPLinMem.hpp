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
#if ! defined(NPLINMEM_HPP)
#define NPLINMEM_HPP

#include <libmaus2/lcs/NPNoTrace.hpp>
#include <libmaus2/lcs/NP.hpp>
#include <stack>

namespace libmaus2
{
	namespace lcs
	{
		// linear space version of NP alignment using recursion
		struct NPLinMem : libmaus2::lcs::AlignmentTraceContainer, libmaus2::lcs::Aligner
		{
			typedef NPLinMem this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			struct Entry
			{
				uint64_t abpos;
				uint64_t aepos;
				uint64_t bbpos;
				uint64_t bepos;

				Entry()
				{}

				Entry(
					uint64_t const rabpos,
					uint64_t const raepos,
					uint64_t const rbbpos,
					uint64_t const rbepos
				) : abpos(rabpos), aepos(raepos), bbpos(rbbpos), bepos(rbepos)
				{

				}

				std::string toString() const
				{
					std::ostringstream ostr;

					ostr << "(" << abpos << "," << aepos << "," << bbpos << "," << bepos << ")";

					return ostr.str();
				}
			};

			libmaus2::lcs::NPNoTrace npt;
			libmaus2::lcs::NP npobj;
			std::stack<Entry> Q;
			uint64_t const d;
			
			uint64_t byteSize() const
			{
				return npt.byteSize() + npobj.byteSize() + sizeof(d);
			}

			NPLinMem(uint64_t const rd = 64) : d(rd) {}

			template<typename iter_a, typename iter_b, bool selfcheck>
			int64_t npTemplate(iter_a const a, iter_a const ae, iter_b const b, iter_b const be)
			{
				libmaus2::lcs::AlignmentTraceContainer::reset();
				Q.push(Entry(0,ae-a,0,be-b));
				uint64_t ed = 0;

				while ( !Q.empty() )
				{
					Entry E = Q.top();
					Q.pop();

					bool split = false;

					if (
						std::max(
							E.aepos-E.abpos,
							E.bepos-E.bbpos
						) >= d
					)
					{
						std::pair<int64_t,int64_t> const P = npt.np(
							a+E.abpos,
							a+E.aepos,
							b+E.bbpos,
							b+E.bepos
						);

						bool const aempty = (P.first==0);
						bool const bempty = (P.second==0);
						bool const empty = aempty && bempty;
						bool const afull = P.first == static_cast<int64_t>(E.aepos-E.abpos);
						bool const bfull = P.second == static_cast<int64_t>(E.bepos-E.bbpos);
						bool const full = afull && bfull;
						bool const trivial = empty || full;

						if ( !trivial )
						{
							Q.push(Entry(E.abpos+P.first,E.aepos,E.bbpos+P.second,E.bepos));
							Q.push(Entry(E.abpos,E.abpos+P.first,E.bbpos,E.bbpos+P.second));
							split = true;
						}
					}

					if ( ! split )
					{
						int64_t const led = npobj.np(
							a+E.abpos,
							a+E.aepos,
							b+E.bbpos,
							b+E.bepos,
							selfcheck
						);
						libmaus2::lcs::AlignmentTraceContainer::push(npobj);
						// std::cerr << E.toString() << " " << led << std::endl;
						ed += led;
					}
				}

				// std::cerr << "ed=" << ed << " " << libmaus2::lcs::AlignmentTraceContainer::getAlignmentStatistics() << " " << npobj.byteSize() << " " << npt.byteSize() << std::endl;

				return ed;
			}

			template<typename iter_a, typename iter_b>
			int64_t np(iter_a const a, iter_a const ae, iter_b const b, iter_b const be, bool const self_check = false)
			{
				if ( self_check )
					return npTemplate<iter_a,iter_b,true>(a,ae,b,be);
				else
					return npTemplate<iter_a,iter_b,false>(a,ae,b,be);
			}

			void align(uint8_t const * a, size_t const l_a, uint8_t const * b, size_t const l_b)
			{
				np(a,a+l_a,b,b+l_b);
			}

			AlignmentTraceContainer const & getTraceContainer() const
			{
				return *this;
			}
		};
	}
}
#endif
