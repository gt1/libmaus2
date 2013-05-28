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

#include <libmaus/autoarray/AutoArray.hpp>

/**
 * constructor copying current values of AutoArray memory usage
 **/
libmaus::autoarray::AutoArrayMemUsage::AutoArrayMemUsage()
: memusage(AutoArray_memusage), peakmemusage(AutoArray_peakmemusage), maxmem(AutoArray_maxmem)
{
}

/**
 * copy constructor
 * @param o object copied
 **/
libmaus::autoarray::AutoArrayMemUsage::AutoArrayMemUsage(libmaus::autoarray::AutoArrayMemUsage const & o)
: memusage(o.memusage), peakmemusage(o.peakmemusage), maxmem(o.maxmem)
{
}

/**
 * assignment operator
 * @param o object copied
 * @return *this
 **/
libmaus::autoarray::AutoArrayMemUsage & libmaus::autoarray::AutoArrayMemUsage::operator=(AutoArrayMemUsage const & o)
{
	memusage = o.memusage;
	peakmemusage = o.peakmemusage;
	maxmem = o.maxmem;
	return *this;
}

/**
 * return true iff *this == o
 * @param o object to be compared
 * @return true iff *this == o
 **/
bool libmaus::autoarray::AutoArrayMemUsage::operator==(libmaus::autoarray::AutoArrayMemUsage const & o) const
{
	return
		memusage == o.memusage
		&&
		peakmemusage == o.peakmemusage
		&&
		maxmem == o.maxmem;
}
/**
 * return true iff *this is different from o
 * @param o object to be compared
 * @return true iff *this != o
 **/
bool  libmaus::autoarray::AutoArrayMemUsage::operator!=(libmaus::autoarray::AutoArrayMemUsage const & o) const
{
	return ! ((*this)==o);
}

std::ostream & operator<<(std::ostream & out, libmaus::autoarray::AutoArrayMemUsage const & aamu)
{
	#if defined(_OPENMP)
	libmaus::autoarray::AutoArray_lock.lock();
	#endif
	out << "AutoArrayMemUsage("
		"memusage=" << static_cast<double>(aamu.memusage)/(1024.0*1024.0) << "," <<
		"peakmemusage=" << static_cast<double>(aamu.peakmemusage)/(1024.0*1024.0) << "," <<
		"maxmem=" << static_cast<double>(aamu.maxmem)/(1024.0*1024.0) << ")";
	#if defined(_OPENMP)
	libmaus::autoarray::AutoArray_lock.unlock();
	#endif			
	return out;
}
