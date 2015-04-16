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

#if ! defined(HISTOGRAMSET_HPP)
#define HISTOGRAMSET_HPP

#include <libmaus2/util/Histogram.hpp>

namespace libmaus2
{
	namespace util
	{
		/**
		 * set of histograms class
		 **/
		struct HistogramSet
		{
			//! this type
			typedef HistogramSet this_type;
			//! unique pointer type
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			
			//! histograms array
			::libmaus2::autoarray::AutoArray < Histogram::unique_ptr_type > H;
			
			/**
			 * constructor
			 *
			 * @param numhist number of histograms
			 * @param lowsize lower size of histograms to be constructed
			 **/
			HistogramSet(uint64_t const numhist, uint64_t const lowsize);
			
			/**
			 * get histogram for i
			 *
			 * @param i index
			 * @return histogram for i
			 **/
			Histogram & operator[](uint64_t const i)
			{
				return *(H[i]);
			}
			
			/**
			 * print histogram set to out
			 *
			 * @param out output stream
			 **/
			void print(std::ostream & out) const;
			/**
			 * merge all histograms to a single one and return it
			 *
			 * @return merged histogram in a unique pointer
			 **/
			Histogram::unique_ptr_type merge() const;
		};
	}
}
#endif
