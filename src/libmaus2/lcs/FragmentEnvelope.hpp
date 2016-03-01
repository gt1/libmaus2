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
			std::pair<int64_t,int64_t> findScore(int64_t const x, int64_t const y) const
			{
				#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
				std::cerr << "[D] FragmentEnvelope::findScore " << x << "," << y << std::endl;
				#endif

				// no fragments or x coordinate of all too large?
				if ( ! M.size() || M.begin()->first > x)
				{
					#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
					std::cerr << "no suitable fragments in database (empty or x wrong)" << std::endl;
					#endif

					return std::pair<int64_t,int64_t>(-1,-1);
				}

				assert ( M.begin() != M.end() && M.begin()->first <= x );

				// find smallest >= x
				const_iterator it = M.lower_bound(x);

				if ( it == M.end() || it->first > x )
					--it;

				assert ( it != M.end() && it->first <= x );

				#if 1
				const_iterator itcheck = it;
				itcheck++;
				assert ( itcheck == M.end() || it->first > x );
				#endif

				int64_t const fragx = it->first;
				assert ( fragx <= x );
				int64_t const fragy = it->second.y;
				assert ( fragy <= y );
				int64_t const fragscore = it->second.score;
				int64_t const nscore =
					(fragscore << num_shift)
					-
					((
						(x-fragx)
						+
						(y-fragy)
					) * mu_num)/mu_den;

				#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
				std::cerr << "[D] FragmentEnvelope::findScore candidate " << it->first << "," << it->second << " score " << nscore << " fragscore " << fragscore << " fragscore<<" << (fragscore<<num_shift) << std::endl;
				std::cerr << (x-fragx) << " " << (y-fragy) << std::endl;
				#endif
				// check whether remaining score when reaching x,y is non negative
				if ( nscore >= 0 )
					return std::pair<int64_t,int64_t>(it->second.id,nscore>>num_shift);
				else
					return std::pair<int64_t,int64_t>(-1,-1);
			}

			/**
			 * find best fragment to connect (x,y) to
			 *
			 * @return id of best fragment or -1 (none exists)
			 **/
			int64_t find(int64_t const x, int64_t const y) const
			{
				return findScore(x,y).first;
			}

			bool isDominatedByOther(
				int64_t const x,
				int64_t const y,
				int64_t const score,
				int64_t const xo,
				int64_t const yo,
				int64_t const scoreo
			) const
			{
				return
					(scoreo<<num_shift) >
					(score<<num_shift) +
					(((x-xo) + (y-yo))*mu_num) / mu_den;
			}

			/**
			 * insert fragment ending at (x,y) with best score up to fragment score
			 **/
			void insert(int64_t const x, int64_t const y, int64_t const score, uint64_t const id)
			{
				// fragment object for new fragment
				FragmentEnvelopeYScore yscore(y,score,id);

				#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
				FragmentEnvelopeFragment frag(x,yscore);

				std::cerr << std::string(80,'-') << std::endl;
				std::cerr << "inserting " << frag << " adp score " << (score<<num_shift) + ((x+y)*mu_num)/mu_den << std::endl;
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

					// do we have values not exceeding x in M?
					if ( M.begin()->first <= x )
					{
						#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
						std::cerr << "have previous" << std::endl;
						#endif

						// find iterator to first element >= x in M
						iterator it = M.lower_bound(x);
						// we want the largest one not exceeding x
						if ( it == M.end() || it->first > x )
							--it;

						assert ( it != M.end() && it->first <= x );

						#if 1
						iterator itcheck = it;
						itcheck++;
						assert ( itcheck == M.end() || itcheck->first > x );
						#endif

						#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
						std::cerr << "previous " << FragmentEnvelopeFragment(it->first,it->second)
							<< " adp score prev " <<
								(it->second.score<<num_shift) + ((it->first+it->second.y)*mu_num)/mu_den
							<< std::endl;
						#endif

						// is new fragment dominated by one already present?
						if (
							isDominatedByOther(
								x,y,score,
								it->first,it->second.y,it->second.score
							)
						)
						{
							#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
							std::cerr << "cur fragment " << frag << " dominated by " << FragmentEnvelopeFragment(it->first,it->second) << std::endl;
							#endif
						}
						else
						{
							#if defined(LIBMAUS2_LCS_FRAGMENTENVELOPE_DEBUG)
							std::cerr << "inserting fragment" << std::endl;
							#endif
							M [ x ] = yscore;
							cleaning = true;
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
							isDominatedByOther(
								it->first,
								it->second.y,
								it->second.score,
								x,
								y,
								score
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
