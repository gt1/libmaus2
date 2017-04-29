/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_MATH_CLUSTEREDBERNOULLI_HPP)
#define LIBMAUS2_MATH_CLUSTEREDBERNOULLI_HPP

#include <libmaus2/math/GeneralisedClusterBernoulli.hpp>
#include <libmaus2/clustering/KMeans.hpp>

namespace libmaus2
{
	namespace math
	{
		/**
		 * generalised Bernoulli using K-means clustering to reduce complexity
		 * trials are grouped into n clusters via k-means clustering
		 **/
		struct ClusteredBernoulli
		{
			std::vector<double> const & PV;
			uint64_t const n;
			std::vector<double> const KM;
			std::vector<uint64_t> KCNT;
			libmaus2::math::GeneralisedClusterBernoulli KE;
			std::vector<double> const & E;

			std::vector<uint64_t> computeKCNT() const
			{
				std::vector<uint64_t> KCNT(KM.size());
				for ( uint64_t i = 0; i < PV.size(); ++i )
					KCNT [ libmaus2::clustering::KMeans::findClosest(KM,PV[i]) ] ++;
				return KCNT;
			}

			ClusteredBernoulli(std::vector<double> const & rPV, uint64_t const rn)
			: PV(rPV), n(rn), KM(libmaus2::clustering::KMeans::kmeans(PV.begin(),PV.size(),n)), KCNT(computeKCNT()), KE(KM,KCNT), E(KE.E)
			{

			}

			double eval(uint64_t const c) const
			{
				return KE.eval(c);
			}

			uint64_t search(double const p) const
			{
				return KE.search(p);
			}
		};
	}
}
#endif
