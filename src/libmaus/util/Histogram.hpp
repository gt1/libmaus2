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
			Histogram(std::map<uint64_t,uint64_t> const & rall, uint64_t const lowsize = 256) : all(rall), low(lowsize) {}
			Histogram(uint64_t const lowsize = 256) : low(lowsize) {}
			
			uint64_t getLowSize() const
			{
				return low.size();
			}
			
			uint64_t median() const
			{
				std::map<uint64_t,uint64_t> M = get();
				uint64_t sum = 0;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					sum += ita->second;
				uint64_t const sum2 = sum/2;
				
				if ( ! M.size() )
					return 0;

				sum = 0;				
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
				{
					if ( sum2 >= sum && sum2 < sum+ita->second )
						return ita->first;
					sum += ita->second;
				}
				
				return M.rbegin()->first;
			}

			double avg() const
			{
				std::map<uint64_t,uint64_t> M = get();
				uint64_t sum = 0;
				uint64_t div = 0;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
				{
					sum += ita->first * ita->second;
					div += ita->second;
				}
				
				if ( sum )
					return static_cast<double>(sum)/div;
				else
					return 0.0;
			}
			
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
			
			std::map<uint64_t,uint64_t> get() const
			{
				std::map<uint64_t,uint64_t> R = all;
				
				for ( uint64_t i = 0; i < low.size(); ++i )
					if ( low[i] )
						R[i] += low[i];

				return R;
			}
			
			std::vector<uint64_t> getKeyVector()
			{
				std::map<uint64_t,uint64_t> const M = get();
				std::vector<uint64_t> V;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					V.push_back(ita->first);
				return V;
			}
			
			uint64_t getTotal()
			{		
				std::map<uint64_t,uint64_t> const M = get();
				uint64_t total = 0;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					total += ita->first*ita->second;
				return total;
			}

			uint64_t getNumPoints()
			{		
				std::map<uint64_t,uint64_t> const M = get();
				uint64_t total = 0;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
					total += ita->second;
				return total;
			}
			
			template<typename type>
			std::ostream & printType(std::ostream & out)
			{
				std::map<uint64_t,uint64_t> const F = get();
				
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = F.begin(); ita != F.end();
					++ita )
					out << static_cast<type>(ita->first) << "\t" << ita->second << std::endl;
				return out;
			}

			std::ostream & print(std::ostream & out)
			{
				std::map<uint64_t,uint64_t> const F = get();
				
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = F.begin(); ita != F.end();
					++ita )
					out << ita->first << "\t" << ita->second << std::endl;
				return out;
			}
			
			std::ostream & printFrac(std::ostream & out, double const frac = 1)
			{
				std::map<uint64_t,uint64_t> const F = get();
				
				double total = 0;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = F.begin(); ita != F.end();
					++ita )
					total += ita->second;

				double sum = 0;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = F.begin(); ita != F.end() && (sum/total) <= frac;
					++ita )
				{
					out << ita->first << "\t" << ita->second << std::endl;
					sum += ita->second;
				}
				return out;
			}
			
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

			std::vector < std::pair<uint64_t,uint64_t > > getFreqSymVector()
			{
				std::map < uint64_t, uint64_t > const kmerhistM = get();
				// copy to vector and sort
				std::vector < std::pair<uint64_t,uint64_t> > freqsyms;
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = kmerhistM.begin(); ita != kmerhistM.end(); ++ita )
				{
					freqsyms.push_back ( std::pair<uint64_t,uint64_t> (ita->second,ita->first) );
				}
				std::sort ( freqsyms.begin(), freqsyms.end() );
				std::reverse ( freqsyms.begin(), freqsyms.end() );
				
				return freqsyms;
			}
			
			void merge(::libmaus::util::Histogram const & other)
			{
				assert ( this->low.size() == other.low.size() );
				for ( uint64_t i = 0; i < low.size(); ++i )
					low[i] += other.low[i];
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = other.all.begin();
					ita != other.all.end(); ++ita )
					all [ ita->first ] += ita->second;
			}
		};
		
		struct HistogramSet
		{
			typedef HistogramSet this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			
			::libmaus::autoarray::AutoArray < Histogram::unique_ptr_type > H;
			
			HistogramSet(uint64_t const numhist, uint64_t const lowsize)
			: H(numhist)
			{
				for ( uint64_t i = 0; i < numhist; ++i )
					H [ i ] = UNIQUE_PTR_MOVE ( Histogram::unique_ptr_type ( new Histogram(lowsize) ) );
			}
			
			Histogram & operator[](uint64_t const i)
			{
				return *(H[i]);
			}
			
			void print(std::ostream & out) const
			{
				for ( uint64_t i = 0; i < H.size(); ++i )
				{
					out << "--- hist " << i << " ---" << std::endl;
					H[i]->print(out);
				}
			}
			
			Histogram::unique_ptr_type merge() const
			{
				if ( H.size() )
				{
					Histogram::unique_ptr_type hist ( new Histogram(H[0]->getLowSize()) );
					for ( uint64_t i = 0; i < H.size(); ++i )
						hist->merge(*H[i]);
					return UNIQUE_PTR_MOVE(hist);
				}
				else
				{
					return UNIQUE_PTR_MOVE(Histogram::unique_ptr_type());
				}
			}
		};
	}
}
#endif
