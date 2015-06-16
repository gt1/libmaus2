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
#if ! defined(LIBMAUS2_QUANTISATION_QUANTISER_HPP)
#define LIBMAUS2_QUANTISATION_QUANTISER_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/util/StringSerialisation.hpp>
#include <libmaus2/util/utf8.hpp>
#include <vector>
#include <cmath>
#include <set>
#include <map>

namespace libmaus2
{
	namespace quantisation
	{
		struct Quantiser;
		std::ostream & operator<<(std::ostream & out, Quantiser const & Q);
		
		struct Quantiser
		{
			friend std::ostream & operator<<(std::ostream & out, Quantiser const & Q);
		
			typedef Quantiser this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			private:
			std::vector<double> const centres;
			
			public:
			Quantiser(std::vector<double> const & rcentres) : centres(rcentres) {}
			
			uint64_t byteSize() const
			{
				return centres.size() * sizeof(double);
			}
			
			template<typename stream_type>
			void serialise(stream_type & out) const
			{
				::libmaus2::util::UTF8::encodeUTF8(centres.size(),out);
				for ( uint64_t i = 0; i < centres.size(); ++i )
					::libmaus2::util::NumberSerialisation::serialiseDouble(out,centres[i]);
			}
			
			template<typename stream_type>
			static std::vector<double> decodeCentreVector(stream_type & in)
			{
				uint64_t const len = ::libmaus2::util::UTF8::decodeUTF8(in);
				std::vector<double> centres;
				for ( uint64_t i = 0; i < len; ++i )
					centres.push_back(::libmaus2::util::NumberSerialisation::deserialiseDouble(in));
				return centres;
			}
			
			template<typename stream_type>
			Quantiser(stream_type & in) : centres(decodeCentreVector(in))
			{
			}
			
			static double dissimilarity(double const v1, double const v2)
			{
				return std::abs(v1-v2);
			}
			
			double silhouette(std::vector<double> const V) const
			{
				std::map < uint64_t, std::vector<double> > M;
				
				for ( uint64_t i = 0; i < V.size(); ++i )
					M[(*this)[V[i]] ].push_back(V[i]);
					
				std::set<uint64_t> levelset;
				for ( std::map < uint64_t, std::vector<double> >::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					levelset.insert(ita->first);
				
				double s = 0;
				uint64_t sn = 0;	
				for ( std::map < uint64_t, std::vector<double> >::const_iterator ita = M.begin(); ita != M.end(); ++ita )
				{
					uint64_t const thiscluster = ita->first;
					std::vector<double> const & thisvalvec = ita->second;
					
					for ( uint64_t x0 = 0; x0 < thisvalvec.size(); ++x0 )
					{
						double const v_x_0 = thisvalvec[x0];
						double a_x_0 = 0.0; 
						
						for ( uint64_t x1 = 0; x1 < thisvalvec.size(); ++x1 )
							if ( x1 != x0 )
								a_x_0 += dissimilarity(v_x_0,thisvalvec[x1]);
						
						a_x_0 /= (thisvalvec.size() > 1) ? (thisvalvec.size()-1) : 1.0;
						
						double b_x_0 = 0.0;
						uint64_t b_x_0_n = 0;
						for ( std::map<uint64_t, std::vector<double> >::const_iterator sita = M.begin(); sita != M.end(); ++sita )
							if ( sita->first != thiscluster )
							{
								std::vector<double> const & othervalvec = sita->second;
								for ( uint64_t x1 = 0; x1 < othervalvec.size(); ++x1 )
									b_x_0 += dissimilarity(v_x_0,othervalvec[x1]);
								b_x_0_n += othervalvec.size();
							}	
							
						b_x_0 /= b_x_0_n ? b_x_0_n : 1.0;
						
						double const s_x_0 = (b_x_0 - a_x_0) / std::max(a_x_0,b_x_0);
						
						s += s_x_0;
					}

					sn += thisvalvec.size();
				}
				
				s /= sn;
					
				return s;
			}
			
			uint64_t operator()(double const val) const
			{
				std::vector<double>::const_iterator it = 
					std::lower_bound(centres.begin(),centres.end(),val);
					
				if ( it == centres.end() ) it -= 1;
				
				std::vector<double>::const_iterator ita = (it == centres.begin()) ? it : (it-1);
				std::vector<double>::const_iterator itb = it;		
				std::vector<double>::const_iterator itc = ((it+1) != centres.end()) ? it+1 : it;
				std::vector<double>::const_iterator itv;
				
				if ( std::abs(*ita-val) < std::abs(*itb-val) )
				{
					// check ita, itc
					if ( std::abs(*ita-val) < std::abs(*itc-val) )
						itv = ita;
					else
						itv = itc;
				}
				else
				{
					// check itb, itc
					if ( std::abs(*itb-val) < std::abs(*itc-val) )
						itv = itb;
					else
						itv = itc;
				}
				
				#if 0
				std::cerr << "::Q:: " << val << " -> " << *itv 
					<< " level=" << itv-centres.begin()
					<< std::endl;
				#endif
			
				return itv-centres.begin();
			}
			
			double operator[](uint64_t const q) const
			{
				return centres[q];
			}
			
			uint64_t size() const
			{
				return centres.size();
			}
		};
		
		inline std::ostream & operator<<(std::ostream & out, Quantiser const & Q)
		{
			out << "Quantiser(\n";
			
			for ( uint64_t i = 0; i < Q.centres.size(); ++i )
				out << "\tcentres[" << i << "]=" << Q.centres[i] << ";\n";
			
			out << ")\n";
			return out;
		}
	}
}
#endif
