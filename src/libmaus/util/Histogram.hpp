/**
    libmaus
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
**/

#if ! defined(HISTOGRAM_HPP)
#define HISTOGRAM_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/parallel/OMPLock.hpp>

#include <map>
#include <iostream>

namespace libmaus
{
	namespace util
	{
		struct Histogram
		{
			typedef Histogram this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			private:
			std::map<uint64_t,uint64_t> all;
			::libmaus::autoarray::AutoArray<uint64_t> low;
			
			public:
			Histogram(std::map<uint64_t,uint64_t> const & rall, uint64_t const lowsize = 256);
			Histogram(uint64_t const lowsize = 256);
			
			uint64_t getLowSize() const;
			uint64_t median() const;
			double avg() const;
			
			void add(uint64_t const i, uint64_t const v)
			{			
				if ( i < low.size() )
					low[i] += v;
				else
					all[i] += v;
			}

			void operator()(uint64_t const i)
			{
				if ( i < low.size() )
					low[i]++;
				else
					all[i]++;
			}
			
			std::map<uint64_t,uint64_t> get() const;
			std::vector<uint64_t> getKeyVector();
			uint64_t getTotal();
			uint64_t getNumPoints();

			template<typename type>
			std::ostream & printType(std::ostream & out)
			{
				std::map<uint64_t,uint64_t> const F = get();
				
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = F.begin(); ita != F.end();
					++ita )
					out << static_cast<type>(ita->first) << "\t" << ita->second << std::endl;
				return out;
			}

			std::ostream & print(std::ostream & out);
			std::ostream & printFrac(std::ostream & out, double const frac = 1);
			
			template<typename key_type>
			void fill (std::map<key_type,uint64_t> & M)
			{
				std::map<uint64_t,uint64_t> const U = get();
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = U.begin();
					ita != U.end(); ++ita )
				{
					M [ ita->first ] = ita->second;
				}
			}
			template<typename key_type>
			std::map<key_type,uint64_t> getByType()
			{
				std::map<key_type,uint64_t> M;
				fill < key_type >(M);
				return M;
			}			

			std::vector < std::pair<uint64_t,uint64_t > > getFreqSymVector();
			void merge(::libmaus::util::Histogram const & other);
		};
	}
}
#endif
