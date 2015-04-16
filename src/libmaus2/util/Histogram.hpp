/*
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
*/

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
		/**
		 * histogram class; it has a fast low part implemented as an array
		 * and a slower upper part implemented as a sparse associative array (map)
		 **/
		struct Histogram
		{
			//! this type
			typedef Histogram this_type;
			//! unique pointer type
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			//! shared pointer type
			typedef ::libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
		
			private:
			//! complete histogram
			std::map<uint64_t,uint64_t> all;
			//! low part
			::libmaus::autoarray::AutoArray<uint64_t> low;
			
			public:
			/**
			 * constructor with initial histogram
			 *
			 * @param rall initial histogram
			 * @param lowsize size of fast low part
			 **/
			Histogram(std::map<uint64_t,uint64_t> const & rall, uint64_t const lowsize = 256);
			/**
			 * constructor for empty histogram
			 *
			 * @param lowsize size of fast low part
			 **/
			Histogram(uint64_t const lowsize = 256);
			/**
			 * copy constructor
			 **/
			Histogram(Histogram const & o)
			: all(o.all), low(o.low.size())
			{
				std::copy(o.low.begin(),o.low.end(),low.begin());
			}
			
			unique_ptr_type uclone() const
			{
				unique_ptr_type tptr(new this_type(*this));
				return UNIQUE_PTR_MOVE(tptr);
			}
			
			shared_ptr_type sclone() const
			{
				shared_ptr_type sptr(new this_type(*this));
				return sptr;
			}
			
			Histogram & operator=(Histogram const & o)
			{
				if ( this != &o )
				{
					all = o.all;
					if ( low.size() != o.low.size() )
						low = ::libmaus::autoarray::AutoArray<uint64_t>(o.low.size(),false);
					std::copy(o.low.begin(),o.low.end(),low.begin());
				}
				
				return *this;
			}
			
			/**
			 * @return get size of low part
			 **/
			uint64_t getLowSize() const;
			/**
			 * @return median
			 **/
			uint64_t median() const;
			/**
			 * @return weighted average
			 **/
			double avg() const;
			
			/**
			 * add v to frequency of i
			 *
			 * @param i index of value to increase
			 * @param v value to add
			 **/
			void add(uint64_t const i, uint64_t const v)
			{			
				if ( i < low.size() )
					low[i] += v;
				else
					all[i] += v;
			}

			/**
			 * increment frequency of i by 1
			 *
			 * @param i index whose frequency is to be incremented
			 **/
			void operator()(uint64_t const i)
			{
				if ( i < low.size() )
					low[i]++;
				else
					all[i]++;
			}
			
			/**
			 * get histogram as map
			 *
			 * @return histogram
			 **/
			std::map<uint64_t,uint64_t> get() const;
			/**
			 * get vector of keys with nonzero values
			 **/
			std::vector<uint64_t> getKeyVector();
			/**
			 * get total (sum of all frequences)
			 *
			 * @return total
			 **/
			uint64_t getTotal() const;
			/**
			 * get number of keys with nonzero values
			 *
			 * @return number of keys with nonzero values
			 **/
			uint64_t getNumPoints() const;

			/**
			 * print table with keys printed as type
			 *
			 * @param out output stream
			 * @return out
			 **/
			template<typename type>
			std::ostream & printType(std::ostream & out) const
			{
				std::map<uint64_t,uint64_t> const F = get();
				
				for ( std::map<uint64_t,uint64_t>::const_iterator ita = F.begin(); ita != F.end();
					++ita )
					out << static_cast<type>(ita->first) << "\t" << ita->second << std::endl;
				return out;
			}

			/**
			 * print table with keys as integers
			 *
			 * @param out output stream
			 * @return out
			 **/
			std::ostream & print(std::ostream & out) const;
			/**
			 * print table from low to high keys until cumulative frequency has reached frac times the toal
			 *
			 * @param out output stream
			 * @param frac fragment of histogram to be printed
			 * @return out
			 **/
			std::ostream & printFrac(std::ostream & out, double const frac = 1) const;
			
			/**
			 * fill given map M with histogram
			 *
			 * @param M map to be filled
			 **/
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
			
			/**
			 * return histogram as map with key_type as key type
			 *
			 * @return histogram as map with key_type as key type
			 **/
			template<typename key_type>
			std::map<key_type,uint64_t> getByType()
			{
				std::map<key_type,uint64_t> M;
				fill < key_type >(M);
				return M;
			}			

			/**
			 * @return vector with (freq,key) pairs
			 **/
			std::vector < std::pair<uint64_t,uint64_t > > getFreqSymVector();
			/**
			 * merge other histogram into this one
			 *
			 * @param other histogram to be merged into this one
			 **/
			void merge(::libmaus::util::Histogram const & other);
		};
	}
}
#endif
