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
#if ! defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_HPP)
#define LIBMAUS2_LCS_FRAGMENTENVELOPE_HPP

#include <libmaus2/lcs/FragmentEnvelopeFragment.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <map>
#include <cassert>
#include <ostream>
#include <iostream>

namespace libmaus2
{
	namespace lcs
	{
		/**
		 * support class for sparse dynamic programming
		 **/
		struct FragmentEnvelope
		{
			typedef FragmentEnvelope this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			std::map<int64_t, FragmentEnvelopeYScore > M;
			typedef std::map<int64_t, FragmentEnvelopeYScore >::iterator iterator;
			typedef std::map<int64_t, FragmentEnvelopeYScore >::const_iterator const_iterator;

			static unsigned int const num_shift = 16;
			int64_t const mu_num;
			int64_t const mu_den;

			/**
			 * get adapted score
			 *
			 * @param x current x value
			 * @param px previous x value at score
			 * @param y current y value
			 * @param py previous y value at score
			 * @param score previous score
			 * @return adapted score at (x,y) multiplied by 2^num_shift
			 **/
			int64_t getAdaptedPrevScore(
				int64_t const x,
				int64_t const px,
				int64_t const y,
				int64_t const py,
				int64_t const score
			) const
			{
				return (score << num_shift) - (( ((y-py) + (x-px)) * mu_num ) / mu_den);
			}

			public:
			FragmentEnvelope(int64_t const r_mu_num = 1, int64_t const r_mu_den = 1)
			: mu_num(r_mu_num << num_shift), mu_den(r_mu_den)
			{

			}

			void reset()
			{
				M.clear();
			}

			/**
			 * find best fragment to connect (x,y) to
			 *
			 * @return id of best fragment or -1 (none exists)
			 **/
			int64_t find(int64_t const x, int64_t const y) const
			{
				if ( ! M.size() || M.begin()->first > x)
					return -1;

				assert ( M.begin() != M.end() && M.begin()->first <= x );

				const_iterator it = M.lower_bound(x);

				if ( it == M.end() || it->first > x )
					--it;

				assert ( it != M.end() && it->first <= x );

				int64_t const fragx = it->first;
				int64_t const fragy = it->second.y;
				int64_t const fragscore = it->second.score;

				// check whether remaining score when reaching x,y is non negative
				if ( getAdaptedPrevScore(x,fragx,y,fragy,fragscore) >= 0 )
					return it->second.id;
				else
					return -1;
			}

			/**
			 * insert fragment ending at (x,y) with best score up to fragment score
			 **/
			void insert(int64_t const x, int64_t const y, int64_t const score, uint64_t const id)
			{
				FragmentEnvelopeYScore yscore(y,score,id);
				#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
				FragmentEnvelopeFragment frag(x,yscore);
				std::cerr << "inserting " << frag << std::endl;
				#endif

				// is M empty?
				if ( ! M.size() )
				{
					#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
					std::cerr << "map is empty" << std::endl;
					#endif
					M [ x ] = yscore;
				}
				else
				{
					#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
					std::cerr << "map is not empty" << std::endl;
					#endif

					bool cleaning = false;
					// shifted score
					int64_t const mscore = (score << num_shift);

					// do we have values not exceeding x in M?
					if ( M.begin()->first <= x )
					{
						#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
						std::cerr << "have previous" << std::endl;
						#endif

						iterator it = M.lower_bound(x);
						if ( it == M.end() || it->first > x )
							--it;

						assert ( it != M.end() && it->first <= x );

						#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
						std::cerr << "previos " << FragmentEnvelopeFragment(it->first,it->second) << " adapted score "
							<< getAdaptedPrevScore(x,it->first,y,it->second.y,it->second.score) << " mscore " << mscore << std::endl;
						#endif

						// if new fragment is not dominated by previous one
						if (
							mscore
							>=
							getAdaptedPrevScore(
								x,it->first,
								y,it->second.y,
								it->second.score
							)
						)
						{
							#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
							std::cerr << "inserting fragment" << std::endl;
							#endif
							M [ x ] = yscore;
							cleaning = true;
						}
						else
						{
							#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
							std::cerr << "cur fragment " << frag << " dominated by " << FragmentEnvelopeFragment(it->first,it->second) << std::endl;
							#endif
						}
					}
					// all values in M are larger than x
					else
					{
						#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
						std::cerr << "no previous" << std::endl;
						#endif

						assert ( M.begin()->first > x );
						M [ x ] = yscore;
						cleaning = true;
					}

					/*
					 * if we inserted the new fragment then remove all old fragments which
					 * are dominated by the new one
					 */
					while ( cleaning )
					{
						cleaning = false;

						iterator it = M.lower_bound(x);
						assert ( it != M.end() );
						assert ( it->first == x );

						it++;

						if (
							it != M.end()
							&&
							(
								mscore
								>=
								getAdaptedPrevScore(
									x,it->first,
									y,it->second.y,
									it->second.score
								)
							)
						)
						{
							#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
							std::cerr << "erasing fragment " << FragmentEnvelopeFragment(it->first,it->second) << " dominated by " << frag << std::endl;
							#endif
							M.erase(it);
							cleaning = true;
						}
					}
				}
			}
		};
	}
}
#endif
