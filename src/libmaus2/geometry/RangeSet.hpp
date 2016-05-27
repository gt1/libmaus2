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
#if ! defined(LIBMAUS2_GEOMETRY_RANGESET_HPP)
#define LIBMAUS2_GEOMETRY_RANGESET_HPP

#include <map>
#include <utility>
#include <vector>
#include <deque>
#include <cassert>
#include <libmaus2/math/IntegerInterval.hpp>

namespace libmaus2
{
	namespace geometry
	{
		template<typename element_type>
		struct RangeSet
		{
			std::map< uint64_t,std::vector<element_type> > bins;

			int const k;

			static int length(uint64_t rlen)
			{
				int k = 0;
				while ( rlen )
				{
					rlen >>= 1;
					k++;
				}

				return k-1;
			}

			void clear()
			{
				bins.clear();
			}

			RangeSet(uint64_t const rlen)
			: k(length(rlen))
			{
			}

			void insert(element_type const & elem)
			{
				typedef std::pair<uint64_t,uint64_t> upair;
				typedef std::pair<int,upair> q_element;

				std::deque< q_element > todo;

				if ( elem.getTo() > elem.getFrom() )
				{
					todo.push_back(q_element(k,upair(elem.getFrom(),elem.getTo())));

					while ( todo.size() )
					{
						q_element q = todo.front();
						todo.pop_front();

						int const level = k-q.first;

						uint64_t const basebin = (1ull << level)-1;
						uint64_t const div = (1ull<<(q.first+1));

						uint64_t const mask = (q.first >= 0) ? (1<<q.first) : 0;

						uint64_t const low = q.second.first;
						uint64_t const high = q.second.second;
						uint64_t const bin = basebin + low/div;

						// std::cerr << level << "," << q.first << "," << q.second.first << "," << q.second.second << "," << basebin << "," << div << "," << bin << std::endl;

						assert ( high > low );

						bool const cross = ((high-1)&mask) != (low & mask);

						if ( cross || q.first < 0 )
						{
							bins[bin].push_back(elem);
							break;
						}
						else
						{
							assert ( q.first >= 0 );
							todo.push_back(q_element(q.first-1,upair(low,high)));
						}
					}
				}
			}

			void erase(element_type const & elem)
			{
				typedef std::pair<uint64_t,uint64_t> upair;
				typedef std::pair<int,upair> q_element;

				std::deque< q_element > todo;

				if ( elem.getTo() > elem.getFrom() )
				{
					todo.push_back(q_element(k,upair(elem.getFrom(),elem.getTo())));

					while ( todo.size() )
					{
						q_element q = todo.front();
						todo.pop_front();

						int const level = k-q.first;

						uint64_t const basebin = (1ull << level)-1;
						uint64_t const div = (1ull<<(q.first+1));

						uint64_t const mask = (q.first >= 0) ? (1<<q.first) : 0;

						uint64_t const low = q.second.first;
						uint64_t const high = q.second.second;
						uint64_t const bin = basebin + low/div;

						// std::cerr << level << "," << q.first << "," << q.second.first << "," << q.second.second << "," << basebin << "," << div << "," << bin << std::endl;

						assert ( high > low );

						bool const cross = ((high-1)&mask) != (low & mask);

						if ( cross || q.first < 0 )
						{
							if ( bins.find(bin) != bins.end() )
							{
								std::vector<element_type> & V = bins.find(bin)->second;

								uint64_t o = 0;
								for ( uint64_t i = 0; i < V.size(); ++i )
									if ( !(V[i] == elem) )
										V[o++] = V[i];

								V.resize(o);
							}
							break;
						}
						else
						{
							assert ( q.first >= 0 );
							todo.push_back(q_element(q.first-1,upair(low,high)));
						}
					}
				}
			}

			std::vector<element_type const *> search(element_type const & elem) const
			{
				std::vector<element_type const *> R;
				search(elem,R);
				return R;
			}

			void search(element_type const & elem, std::vector<element_type const *> & R) const
			{
				typedef std::pair<uint64_t,uint64_t> upair;
				typedef std::pair<int,upair> q_element;

				std::deque< q_element > todo;

				// if element is not empty
				if ( elem.getTo() > elem.getFrom() )
					// push it
					todo.push_back(q_element(k,upair(elem.getFrom(),elem.getTo())));

				// while we have intervals to be checked
				while ( todo.size() )
				{
					// get next interval
					q_element q = todo.front();
					todo.pop_front();

					// level
					int const level = k-q.first;

					// base bin for level
					uint64_t const basebin = (1ull << level)-1;
					// div/split size for level
					uint64_t const div = (1ull<<(q.first+1));

					uint64_t const mask = (q.first >= 0) ? (static_cast<uint64_t>(1ull)<<q.first) : static_cast<uint64_t>(0);

					// interval
					uint64_t const low = q.second.first;
					uint64_t const high = q.second.second;
					// bin for low end
					uint64_t const bin = basebin + low/div;

					// see if we have anything in bin matching low end
					if ( bins.find(bin) != bins.end() )
					{
						// get list of entries in bin
						std::vector<element_type> const & V = bins.find(bin)->second;
						// compute intersections between elements in bin and query
						for ( uint64_t i = 0; i < V.size(); ++i )
						{
							libmaus2::math::IntegerInterval<uint64_t> I0(elem.getFrom(),elem.getTo()-1);
							libmaus2::math::IntegerInterval<uint64_t> I1(V[i].getFrom(),V[i].getTo()-1);
							if ( !I0.intersection(I1).isEmpty() )
								R.push_back(&V[i]);
						}
					}

					// std::cerr << level << "," << q.first << "," << q.second.first << "," << q.second.second << "," << basebin << "," << div << "," << bin << std::endl;

					// query interval should not be empty
					assert ( high > low );

					// do we cross the bin split bound?
					bool const cross = ((high-1)&mask) != (low & mask);

					if ( cross )
					{
						uint64_t const mid = ((low >> q.first)+1)<<q.first;

						// std::cerr << "low=" << low << ",mid=" << mid << ",high=" << high << std::endl;

						bool const ok = (mid > low) && (high > mid);
						assert ( ok );

						todo.push_back(q_element(q.first-1,upair(low,mid)));
						todo.push_back(q_element(q.first-1,upair(mid,high)));
					}
					else if ( q.first >= 0 )
					{
						todo.push_back(q_element(q.first-1,upair(low,high)));
					}
					else
					{
						// std::cerr << "base " << low << "," << high << " bin " << bin << std::endl;
					}
				}

				#if defined(LIBMAUS2_GEOMETRY_RANGESET_DEBUG)
				std::vector<element_type const *> RR;
				for ( typename std::map< uint64_t,std::vector<element_type> >::const_iterator ita = bins.begin(); ita != bins.end(); ++ita )
				{
					std::vector<element_type> const & V = ita->second;
					for ( uint64_t i = 0; i < V.size(); ++i )
					{
						libmaus2::math::IntegerInterval<uint64_t> I0(elem.getFrom(),elem.getTo()-1);
						libmaus2::math::IntegerInterval<uint64_t> I1(V[i].getFrom(),V[i].getTo()-1);
						if ( !I0.intersection(I1).isEmpty() )
							RR.push_back(&V[i]);
					}
				}
				assert ( R.size() == RR.size() );
				std::set < element_type const * > SA(R.begin(),R.end());
				std::set < element_type const * > SB(RR.begin(),RR.end());
				assert ( SA == SB );
				#endif
			}
		};
	}
}
#endif
