/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_RANK_DNARANKSMEMCONTEXT_HPP)
#define LIBMAUS2_RANK_DNARANKSMEMCONTEXT_HPP

#include <libmaus2/rank/DNARankMEM.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace rank
	{
		struct DNARankSMEMContext
		{
			typedef DNARankSMEMContext this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			//typedef std::vector < std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > >::const_iterator prev_iterator;
			typedef std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > const * prev_iterator;
			// typedef std::vector < DNARankMEM > match_container_type;
			typedef libmaus2::autoarray::AutoArray < DNARankMEM > match_container_type;
			// typedef match_container_type::const_iterator match_iterator;
			typedef DNARankMEM const * match_iterator;

			private:
			//std::vector < std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > > Vcur;
			libmaus2::autoarray::AutoArray< std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > > Acur;
			uint64_t ocur;
			//std::vector < std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > > Vprev;
			libmaus2::autoarray::AutoArray< std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > > Aprev;
			uint64_t oprev;
			match_container_type Amatch;
			uint64_t omatch;

			public:
			DNARankSMEMContext() : ocur(0), oprev(0), omatch(0)
			{

			}

			match_container_type const & getMatches() const
			{
				//return Vmatch;
				return Amatch;
			}

			uint64_t getNumMatches() const
			{
				//return Vmatch.size();
				return omatch;
			}

			prev_iterator pbegin() const
			{
				//return Vprev.begin();
				return Aprev.begin();
			}

			prev_iterator pend() const
			{
				//return Vprev.end();
				return Aprev.begin()+oprev;
			}

			match_iterator mbegin() const
			{
				//return Vmatch.begin();
				return Amatch.begin();
			}

			match_iterator mend() const
			{
				//return Vmatch.end();
				return Amatch.begin()+omatch;
			}

			void reset()
			{
				//Vcur.resize(0);
				//Vprev.resize(0);
				ocur = 0;
				oprev = 0;
			}

			void resetCur()
			{
				//Vcur.resize(0);
				ocur = 0;
			}

			void reverseCur()
			{
				//std::reverse(Vcur.begin(),Vcur.end());
				std::reverse(Acur.begin(),Acur.begin()+ocur);
			}

			void resetMatch()
			{
				//Vmatch.resize(0);
				omatch = 0;
			}

			void reverseMatch(uint64_t const start)
			{
				std::reverse(Amatch.begin()+start,Amatch.begin()+omatch);
			}

			void pushCur(std::pair< libmaus2::rank::DNARankBiDirRange, uint64_t > const & I)
			{
				//Vcur.push_back(I);
				Acur.push(ocur,I);
			}

			void pushMatch(DNARankMEM const & I)
			{
				//Vmatch.push_back(I);
				Amatch.push(omatch,I);
			}

			void swapCurPrev()
			{
				// Vcur.swap(Vprev);
				Acur.swap(Aprev);
				std::swap(ocur,oprev);
			}

			bool curEmpty() const
			{
				//return Vcur.empty();
				return ocur == 0;
			}

			bool prevEmpty() const
			{
				//return Vprev.empty();
				return oprev == 0;
			}
		};
	}
}
#endif
