/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
    Copyright (C) 2011-2013 Genome Research Limited

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
#if ! defined(LIBMAUS2_QUANTISATION_CLUSTERCOMPUTATION_HPP)
#define LIBMAUS2_QUANTISATION_CLUSTERCOMPUTATION_HPP

#include <libmaus2/LibMausConfig.hpp>
#include <libmaus2/quantisation/Quantiser.hpp>
#include <libmaus2/clustering/KMeans.hpp>

namespace libmaus2
{
	namespace quantisation
	{
		struct ClusterComputation
		{
			template<typename value_type>
			static ::libmaus2::quantisation::Quantiser::unique_ptr_type constructQuantiser(
				std::vector<value_type> const & V, uint64_t const k, uint64_t const runs = 100
			)
			{
				std::vector<double> const centres =
					libmaus2::clustering::KMeans::kmeans(V.begin(),V.size(),k,true /* pp */, runs);
				::libmaus2::quantisation::Quantiser::unique_ptr_type Pquant(
					new ::libmaus2::quantisation::Quantiser(centres)
				);
				return Pquant;
			}
		};
	}
}
#endif
