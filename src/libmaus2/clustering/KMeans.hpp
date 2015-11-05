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
#if ! defined(LIBMAUS2_CLUSTERING_KMEANS_HPP)
#define LIBMAUS2_CLUSTERING_KMEANS_HPP

#include <vector>
#include <algorithm>
#include <cassert>
#include <set>
#include <cmath>
#include <limits>
#include <map>

#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/random/Random.hpp>
#include <libmaus2/random/UniformUnitRandom.hpp>

namespace libmaus2
{
	namespace clustering
	{
		struct KMeans
		{
			typedef KMeans this_type;

			static double error(
				std::vector<double> const & A,
				std::vector<double> const & B
			)
			{
				if ( A.size() != B.size() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "KMeans::error(std::vector<double> const & A,std::vector<double> const & B): A.size()=" << A.size() << " != B.size()=" << B.size() << std::endl;
					lme.finish();
					throw lme;
				}
				assert ( A.size() == B.size() );
				double e = 0;
				for ( uint64_t i = 0; i < A.size(); ++i )
					e += (A[i]-B[i])*(A[i]-B[i]);
				return e;
			}

			static double error(std::vector< std::vector<double> > const & V, std::vector< std::vector<double> > const & R)
			{
				double e = 0;
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					uint64_t const index = findClosest(R,V[i]);
					e += error(V[i],R[index]);
				}
				return e;
			}


			static std::vector< std::vector<double> >::size_type findClosest(
				std::vector< std::vector<double> > const & R,
				std::vector<double> const & V
			)
			{
				if ( ! R.size() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::clustering::KMeans::findClosest: vector is empty" << std::endl;
					lme.finish();
					throw lme;
				}

				double e = std::numeric_limits<double>::max();
				int64_t index = 0;

				for ( uint64_t i = 0; i < R.size(); ++i )
				{
					std::vector<double> const & C = R[i];

					if ( C.size() != V.size() )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "KMeans::findClosest(): C.size()=" << C.size() << " != V.size()=" << V.size() << std::endl;
						lme.finish();
						throw lme;
					}
					assert ( C.size() == V.size() );
					double le = error(C,V);
					if ( le < e )
					{
						index = i;
						e = le;
					}
				}

				return index;
			}

			static std::vector< std::vector<double> > kmeans(
				std::vector< std::vector<double> > const & V,
				uint64_t const k,
				bool const pp = true,
				uint64_t const iterations = 10,
				uint64_t const maxloops = 16*1024, double const ethres = 1e-6
			)
			{
				std::vector< std::vector<double> > R = kmeansCore(V,k,pp,maxloops,ethres);
				double e = error(V,R);

				for ( uint64_t i = 1; i < iterations; ++i )
				{
					std::vector<std::vector<double> > RC = kmeansCore(V,k,pp,maxloops,ethres);
					double re = error(V,RC);
					if ( re < e )
					{
						e = re;
						R = RC;
					}
				}

				return R;
			}

			static std::vector< std::vector<double> > kmeansCore(
				std::vector< std::vector<double> > const & V,
				uint64_t k,
				bool const pp,
				uint64_t const maxloops,
				double const ethres
			)
			{
				std::set< std::vector<double> > S;

				uint64_t const n = V.size();
				std::vector<uint64_t> ileft(n);
				std::vector<double> D(n,1.0 / n);
				for ( uint64_t i = 0; i < ileft.size(); ++i )
					ileft[i] = i;

				// choose initial centres
				while ( S.size() < k && ileft.size() )
				{
					uint64_t i;

					if ( pp && S.size() )
					{
						std::vector< std::vector<double> > R(S.begin(),S.end());

						double s = 0;
						// iterate over points still available
						for ( uint64_t z = 0; z < ileft.size(); ++z )
						{
							// find closests centre
							uint64_t const j = findClosest(R,V[ileft[z]]);
							// compute distance
							double const dif = error(R[j],V[ileft[z]]);
							// set distance
							D[z] = dif;
							// accumulate
							s += dif;
						}
						// if any distance
						if ( s )
						{
							// divide by total (turn into probability)
							for ( uint64_t z = 0; z < ileft.size(); ++z )
								D[z] /= s;
							// compute prefix sums
							s = 0;
							for ( uint64_t z = 0; z < ileft.size(); ++z )
							{
								double const t = D[z];
								D[z] = s;
								s += t;
							}

							// get random value in [0,1]
							double const p = libmaus2::random::UniformUnitRandom::uniformUnitRandom();
							// look for closest prob
							std::vector<double>::const_iterator it = std::lower_bound(D.begin(),D.begin()+ileft.size(),p);
							if ( it == D.begin()+ileft.size() )
								it -= 1;
							i = it-D.begin();
						}
						// no more distance, randomly chosse point (which we have chosen before)
						else
						{
							i = libmaus2::random::Random::rand64() % ileft.size();
						}

						// insert new centre
						S.insert(V[ileft[i]]);
						// move chosen point to back
						std::swap(ileft[i],ileft[ileft.size()-1]);
						ileft.pop_back();
					}
					else
					{
						i = libmaus2::random::Random::rand64() % ileft.size();
						// insert new centre
						S.insert(V[ileft[i]]);
						// move chosen point to back
						std::swap(ileft[i],ileft[ileft.size()-1]);
						ileft.pop_back();
					}
				}

				// centre vector
				std::vector< std::vector<double> > R(S.begin(),S.end());

				// cluster id for point i
				std::vector<uint64_t> I(n);
				// number of points in cluster id
				std::vector<uint64_t> H(R.size());
				// previous error
				double preve = std::numeric_limits<double>::max();
				for ( uint64_t l = 0; l < maxloops; ++l )
				{
					// erase histogram
					std::fill(H.begin(),H.end(),0);
					// error
					double e = 0;
					// find cluster for each value in V
					for ( uint64_t i = 0; i < n; ++i )
					{
						I[i] = findClosest(R,V[i]);
						H[I[i]] += 1;
						e += error(V[i],R[I[i]]);
					}

					// break if improvement is smaller than threshold
					if ( std::abs(e-preve) < ethres )
						break;

					preve = e;

					// erase centres
					for ( std::vector<double>::size_type i = 0; i < R.size(); ++i )
					{
						std::fill(R[i].begin(),R[i].end(),0.0);
					}
					// sum
					for ( uint64_t i = 0; i < n; ++i )
					{
						for ( uint64_t j = 0; j < V[i].size(); ++j )
							R[I[i]][j] += V[i][j];
					}
					// compute averages
					for ( std::vector<double>::size_type i = 0; i < R.size(); ++i )
					{
						if ( H[i] )
						{
							for ( uint64_t j = 0; j < R[i].size(); ++j )
								R[i][j] /= H[i];
						}
						else
							// randomly choose new centre if previous one did not attract any points
							R[i] = V[libmaus2::random::Random::rand64() % n];
					}

					// sort new centres
					std::sort(R.begin(),R.end());
				}

				return R;
			}

			template<typename centres_type>
			static std::vector<double>::size_type findClosest(centres_type const & R, double const v)
			{
				if ( ! R.size() )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::clustering::KMeans::findClosest: vector is empty" << std::endl;
					lme.finish();
					throw lme;
				}

				typename centres_type::const_iterator it = ::std::lower_bound(R.begin(),R.end(),v);

				uint64_t r;
				// larger than last
				if ( it == R.end() )
				{
					r = R.size()-1;
				}
				// smaller/eq than first
				else if ( it == R.begin() )
				{
					r = 0;
				}
				// same as element
				else if ( v == *it )
				{
					r = it-R.begin();
				}
				else
				{
					bool const ok = ( *it >= v ) && ( *(it-1) <= v );
					if ( ! ok )
					{
						std::cerr << "KMeans::findClosest(centres_type const & R, double const v): failed v=" << v << " *(it-1)=" << *(it-1) << " *it=" << *it << std::endl;
						assert ( ok );
					}

					double const e_it = (v-*it) * (v-*it);
					double const e_it_1 = (v-(*(it-1))) * (v-(*(it-1)));

					if ( e_it_1 <= e_it)
						r = (it-1) - R.begin();
					else
						r = it - R.begin();
				}

				#if defined(LIBMAUS2_CLUSTERING_KMEANS_FINDCLOSESTS_DEBUG)
				std::vector<double>::size_type mindiffindex = 0;
				double mindiff = (R[mindiffindex]-v) * (R[mindiffindex]-v);

				for ( std::vector<double>::size_type i = 1; i < R.size(); ++i )
				{
					double const diff = (R[i]-v)*(R[i]-v);
					if ( diff < mindiff )
					{
						mindiff = diff;
						mindiffindex = i;
					}
				}

				assert ( r == mindiffindex );
				#endif

				return r;
			}

			/**
			 * compute total error
			 *
			 * V value vector
			 * n number of values
			 * R centres
			 **/
			template<typename iterator, typename dissimilarity_type>
			static double error(iterator V, uint64_t n, std::vector<double> const & R, dissimilarity_type const & dissimilarity)
			{
				typedef typename std::iterator_traits<iterator>::value_type value_type;

				double e = 0;
				while ( n-- )
				{
					value_type const v = *(V++);
					uint64_t const i = findClosest(R,v);
					e += dissimilarity(R[i],v);
				}

				return e;
			}

			template<typename iterator, typename dissimilarity_type>
			static std::vector<double> kmeansCore(
				iterator V, uint64_t const n, uint64_t k,
				bool const pp,
				uint64_t const maxloops,
				double const ethres,
				dissimilarity_type const & dissimilarity
			)
			{
				// pre centres
				std::set<double> S;

				// points left
				std::vector<uint64_t> ileft(n);
				std::vector<double> D(n,1.0 / n);
				for ( uint64_t i = 0; i < ileft.size(); ++i )
					ileft[i] = i;

				while ( S.size() < k && ileft.size() )
				{
					uint64_t i;
					// kmeans++
					if ( pp && S.size() )
					{
						// total distance
						double s = 0.0;
						// centres vector
						std::vector<double> const R(S.begin(),S.end());
						// iterate over points still available
						for ( uint64_t z = 0; z < ileft.size(); ++z )
						{
							// find closests centre
							uint64_t const j = findClosest(R,V[ileft[z]]);
							// compute distance
							double const dif = dissimilarity(R[j],V[ileft[z]]);
							// set distance
							D[z] = dif;
							// accumulate
							s += dif;
						}
						// if any distance
						if ( s )
						{
							// divide by total (turn into probability)
							for ( uint64_t z = 0; z < ileft.size(); ++z )
								D[z] /= s;
							// compute prefix sums
							s = 0;
							for ( uint64_t z = 0; z < ileft.size(); ++z )
							{
								double const t = D[z];
								D[z] = s;
								s += t;
							}

							// get random value in [0,1]
							double const p = libmaus2::random::UniformUnitRandom::uniformUnitRandom();
							// look for closest prob
							std::vector<double>::const_iterator it = std::lower_bound(D.begin(),D.begin()+ileft.size(),p);
							if ( it == D.begin()+ileft.size() )
								it -= 1;
							i = it-D.begin();
						}
						// no more distance, randomly chosse point (which we have chosen before)
						else
						{
							i = libmaus2::random::Random::rand64() % ileft.size();
						}
					}
					// either first point or random start point selection
					else
					{
						i = libmaus2::random::Random::rand64() % ileft.size();
					}

					// insert new centre
					S.insert(static_cast<double>(V[ileft[i]]));
					// move chosen point to back
					std::swap(ileft[i],ileft[ileft.size()-1]);
					ileft.pop_back();
				}

				// centre vector
				std::vector<double> R(S.begin(),S.end());

				// cluster id for point i
				std::vector<uint64_t> I(n);
				// number of points in cluster id
				std::vector<uint64_t> H(R.size());
				// previous error
				double preve = std::numeric_limits<double>::max();
				for ( uint64_t l = 0; l < maxloops; ++l )
				{
					// erase histogram
					std::fill(H.begin(),H.end(),0);
					// error
					double e = 0;
					// find cluster for each value in V
					for ( uint64_t i = 0; i < n; ++i )
					{
						I[i] = findClosest(R,V[i]);
						H[I[i]] += 1;
						e += dissimilarity(V[i],R[I[i]]);
					}

					// break if improvement is smaller than threshold
					if ( std::abs(e-preve) < ethres )
						break;

					preve = e;

					// erase centres
					for ( std::vector<double>::size_type i = 0; i < R.size(); ++i )
						R[i] = 0.0;
					// sum
					for ( uint64_t i = 0; i < n; ++i )
						R[I[i]]	+= V[i];
					// compute averages
					for ( std::vector<double>::size_type i = 0; i < R.size(); ++i )
					{
						if ( H[i] )
						{
							R[i] /= H[i];
						}
						else
							// randomly choose new centre if previous one did not attract any points
							R[i] = V[libmaus2::random::Random::rand64() % n];
					}

					// sort new centres
					std::sort(R.begin(),R.end());
				}

				return R;
			}

			struct SquareDissimilary
			{
				double operator()(double const a, double const b) const
				{
					double const d = a-b;
					return d*d;
				}
			};


			template<typename iterator, typename dissimilarity_type>
			static std::vector<double> kmeans(
				iterator V,
				uint64_t const n,
				uint64_t const k,
				bool const pp = true,
				uint64_t const iterations = 10,
				uint64_t const maxloops = 16*1024, double const ethres = 1e-6,
				dissimilarity_type const & dissimilarity = dissimilarity_type()
			)
			{
				std::vector<double> R = kmeansCore(V,n,k,pp,maxloops,ethres,dissimilarity);
				double e = error(V,n,R,dissimilarity);

				for ( uint64_t i = 1; i < iterations; ++i )
				{
					std::vector<double> RC = kmeansCore(V,n,k,pp,maxloops,ethres,dissimilarity);
					double re = error(V,n,RC,dissimilarity);
					if ( re < e )
					{
						e = re;
						R = RC;
					}
				}

				return R;
			}

			template<typename iterator>
			static std::vector<double> kmeans(
				iterator V,
				uint64_t const n,
				uint64_t const k,
				bool const pp = true,
				uint64_t const iterations = 10,
				uint64_t const maxloops = 16*1024, double const ethres = 1e-6
			)
			{
				return kmeans<iterator,SquareDissimilary>(V,n,k,pp,iterations,maxloops,ethres,SquareDissimilary());
			}


			static double dissimilarity(double const v1, double const v2)
			{
				return (v1-v2)*(v1-v2);
				// return std::abs(v1-v2);
			}

			static double dissimilarity(std::vector<double> const & v1, std::vector<double> const & v2)
			{
				double e = 0;
				for ( uint64_t i = 0; i < v1.size(); ++i )
					e += dissimilarity(v1[i],v2[i]);
				return e;
			}

			/**
			 * V values
			 * C centres
			 **/
			static double silhouette(
				std::vector<double> const & V,
				std::vector<double> const & C
			)
			{
				// find cluster for each data point
				std::map < uint64_t, std::vector<double> > M;

				for ( uint64_t i = 0; i < V.size(); ++i )
					M[findClosest(C,V[i])].push_back(V[i]);

				double s = 0;
				uint64_t sn = 0;
				for ( std::map < uint64_t, std::vector<double> >::const_iterator ita = M.begin(); ita != M.end(); ++ita )
				{
					// id of this cluster
					uint64_t const thiscluster = ita->first;
					// values in this cluster
					std::vector<double> const & thisvalvec = ita->second;

					double   clusters = 0.0;
					uint64_t clustersn = 0;

					// iterate over values in this cluster
					for ( uint64_t x0 = 0; x0 < thisvalvec.size(); ++x0 )
					{
						// get value
						double const v_x_0 = thisvalvec[x0];
						double a_x_0 = 0.0;

						// compare to other values in this cluster
						for ( uint64_t x1 = 0; x1 < thisvalvec.size(); ++x1 )
							a_x_0 += dissimilarity(v_x_0,thisvalvec[x1]);

						if ( thisvalvec.size() )
							a_x_0 /= thisvalvec.size();

						double b_x_0_min = std::numeric_limits<double>::max();
						// iterate over other clusters
						for ( std::map<uint64_t, std::vector<double> >::const_iterator sita = M.begin(); sita != M.end(); ++sita )
							if ( sita->first != thiscluster )
							{
								std::vector<double> const & othervalvec = sita->second;

								double b_x_0 = 0.0;
								uint64_t const b_x_0_n = othervalvec.size();

								for ( uint64_t x1 = 0; x1 < othervalvec.size(); ++x1 )
									b_x_0 += dissimilarity(v_x_0,othervalvec[x1]);

								b_x_0 /= b_x_0_n ? b_x_0_n : 1.0;

								if ( b_x_0 < b_x_0_min )
									b_x_0_min = b_x_0;
							}

						double const s_x_0 = (b_x_0_min - a_x_0) / std::max(a_x_0,b_x_0_min);

						clusters += s_x_0;
						clustersn += 1;
						s += s_x_0;
						sn += 1;
					}

					clusters /= clustersn;
				}

				s /= sn;

				return s;
			}

			/**
			 * V values
			 * C centres
			 **/
			static double silhouette(
				std::vector< std::vector<double> > const & V,
				std::vector< std::vector<double> > const & C
			)
			{
				// find cluster for each data point
				std::map < uint64_t, std::vector< std::vector<double> > > M;

				for ( uint64_t i = 0; i < V.size(); ++i )
					M[findClosest(C,V[i])].push_back(V[i]);

				double s = 0;
				uint64_t sn = 0;
				for ( std::map < uint64_t, std::vector<std::vector<double> > >::const_iterator ita = M.begin(); ita != M.end(); ++ita )
				{
					// id of this cluster
					uint64_t const thiscluster = ita->first;
					// values in this cluster
					std::vector< std::vector< double > > const & thisvalvec = ita->second;

					double   clusters = 0.0;
					uint64_t clustersn = 0;

					// iterate over values in this cluster
					for ( uint64_t x0 = 0; x0 < thisvalvec.size(); ++x0 )
					{
						// get value
						std::vector<double> const v_x_0 = thisvalvec[x0];
						double a_x_0 = 0.0;

						// compare to other values in this cluster
						for ( uint64_t x1 = 0; x1 < thisvalvec.size(); ++x1 )
							a_x_0 += dissimilarity(v_x_0,thisvalvec[x1]);

						if ( thisvalvec.size() )
							a_x_0 /= thisvalvec.size();

						double b_x_0_min = std::numeric_limits<double>::max();
						// iterate over other clusters
						for ( std::map<uint64_t, std::vector<std::vector<double> > >::const_iterator sita = M.begin(); sita != M.end(); ++sita )
							if ( sita->first != thiscluster )
							{
								std::vector<std::vector<double> > const & othervalvec = sita->second;

								double b_x_0 = 0.0;
								uint64_t const b_x_0_n = othervalvec.size();

								for ( uint64_t x1 = 0; x1 < othervalvec.size(); ++x1 )
									b_x_0 += dissimilarity(v_x_0,othervalvec[x1]);

								b_x_0 /= b_x_0_n ? b_x_0_n : 1.0;

								if ( b_x_0 < b_x_0_min )
									b_x_0_min = b_x_0;
							}

						double const s_x_0 = (b_x_0_min - a_x_0) / std::max(a_x_0,b_x_0_min);

						clusters += s_x_0;
						clustersn += 1;
						s += s_x_0;
						sn += 1;
					}

					clusters /= clustersn;
				}

				s /= sn;

				return s;
			}

		};
	}
}
#endif
