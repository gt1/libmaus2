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
						std::cerr << "failed v=" << v << " *(it-1)=" << *(it-1) << " *it=" << *it << std::endl;
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
			
			template<typename iterator>
			static double error(iterator V, uint64_t n, std::vector<double> const & R)
			{
				typedef typename std::iterator_traits<iterator>::value_type value_type;
				
				double e = 0;
				while ( n-- )
				{
					value_type const v = *(V++);
					uint64_t const i = findClosest(R,v);
					e += (R[i]-v)*(R[i]-v);
				}
				
				return e;
			}

			template<typename iterator>
			static std::vector<double> kmeansCore(
				iterator V, uint64_t const n, uint64_t k, 
				bool const pp,
				uint64_t const maxloops, 
				double const ethres
			)
			{
				typedef typename std::iterator_traits<iterator>::value_type value_type;
			
				std::set<double> S;
				
				std::vector<uint64_t> ileft(n);
				std::vector<double> D(n,1.0 / n);
				for ( uint64_t i = 0; i < ileft.size(); ++i )
					ileft[i] = i;
					
				while ( S.size() < k && ileft.size() )
				{
					uint64_t i;
					if ( pp && S.size() )
					{
						double s = 0.0;
						std::vector<double> const R(S.begin(),S.end());
						for ( uint64_t i = 0; i < ileft.size(); ++i )
						{
							uint64_t const j = findClosest(R,V[ileft[i]]);
							double const dif = (R[j]-V[ileft[i]]) * (R[j]-V[ileft[i]]);
							D[i] = dif;
							s += dif;
						}
						if ( s )
						{
							for ( uint64_t i = 0; i < ileft.size(); ++i )
								D[i] /= s;
							s = 0;
							for ( uint64_t i = 0; i < ileft.size(); ++i )
							{
								double const t = D[i];
								D[i] = s;
								s += t;
							}
							
							double const p = libmaus2::random::UniformUnitRandom::uniformUnitRandom();
							std::vector<double>::const_iterator it = std::lower_bound(D.begin(),D.begin()+ileft.size(),p);
							if ( it == D.begin()+ileft.size() )
								it -= 1;
							i = it-D.begin();
						}
						else
						{
							i = libmaus2::random::Random::rand64() % ileft.size();
						}
					}
					else
					{
						i = libmaus2::random::Random::rand64() % ileft.size();
					}
					S.insert(static_cast<double>(V[ileft[i]]));
					std::swap(ileft[i],ileft[ileft.size()-1]);
					ileft.pop_back();
				}
				
				// centre vector
				std::vector<double> R(S.begin(),S.end());

				std::vector<uint64_t> I(n);
				std::vector<uint64_t> H(R.size());
				double preve = std::numeric_limits<double>::max();
				for ( uint64_t l = 0; l < maxloops; ++l )
				{
					uint64_t low = 0;
					
					// erase histogram
					std::fill(H.begin(),H.end(),0);
					// error
					double e = 0;
					// find cluster for each value in V
					for ( uint64_t i = 0; i < n; ++i )
					{
						I[i] = findClosest(R,V[i]);
						H[I[i]] += 1;
						e += (V[i]-R[I[i]])*(V[i]-R[I[i]]);
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
							R[i] /= H[i];
						else
							// randomly choose new centre if previous one did not attract any points
							R[i] = V[libmaus2::random::Random::rand64() % n];
					}
						
					std::sort(R.begin(),R.end());
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
				std::vector<double> R = kmeansCore(V,n,k,pp,maxloops,ethres);
				double e = error(V,n,R);
								
				for ( uint64_t i = 1; i < iterations; ++i )
				{
					std::vector<double> RC = kmeansCore(V,n,k,pp,maxloops,ethres);
					double re = error(V,n,RC);
					if ( re < e )
					{
						e = re;
						R = RC;
					}
				}
				
				return R;
			}
		};	
	}
}
#endif
