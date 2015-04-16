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

#include <libmaus/util/Histogram.hpp>

libmaus::util::Histogram::Histogram(std::map<uint64_t,uint64_t> const & rall, uint64_t const lowsize) : all(rall), low(lowsize) {}
libmaus::util::Histogram::Histogram(uint64_t const lowsize) : low(lowsize) {}

uint64_t libmaus::util::Histogram::getLowSize() const
{
	return low.size();
}

uint64_t libmaus::util::Histogram::median() const
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

double libmaus::util::Histogram::avg() const
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

std::map<uint64_t,uint64_t> libmaus::util::Histogram::get() const
{
	std::map<uint64_t,uint64_t> R = all;
	
	for ( uint64_t i = 0; i < low.size(); ++i )
		if ( low[i] )
			R[i] += low[i];

	return R;
}

std::vector<uint64_t> libmaus::util::Histogram::getKeyVector()
{
	std::map<uint64_t,uint64_t> const M = get();
	std::vector<uint64_t> V;
	for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
		V.push_back(ita->first);
	return V;
}

uint64_t libmaus::util::Histogram::getTotal() const
{		
	std::map<uint64_t,uint64_t> const M = get();
	uint64_t total = 0;
	for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
		total += ita->first*ita->second;
	return total;
}

uint64_t libmaus::util::Histogram::getNumPoints() const
{		
	std::map<uint64_t,uint64_t> const M = get();
	uint64_t total = 0;
	for ( std::map<uint64_t,uint64_t>::const_iterator ita = M.begin(); ita != M.end(); ++ita )
		total += ita->second;
	return total;
}

std::ostream & libmaus::util::Histogram::print(std::ostream & out) const
{
	std::map<uint64_t,uint64_t> const F = get();
	
	for ( std::map<uint64_t,uint64_t>::const_iterator ita = F.begin(); ita != F.end();
		++ita )
		out << ita->first << "\t" << ita->second << std::endl;
	return out;
}

std::ostream & libmaus::util::Histogram::printFrac(std::ostream & out, double const frac) const
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

std::vector < std::pair<uint64_t,uint64_t > > libmaus::util::Histogram::getFreqSymVector()
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

void libmaus::util::Histogram::merge(::libmaus::util::Histogram const & other)
{
	assert ( this->low.size() == other.low.size() );
	for ( uint64_t i = 0; i < low.size(); ++i )
		low[i] += other.low[i];
	for ( std::map<uint64_t,uint64_t>::const_iterator ita = other.all.begin();
		ita != other.all.end(); ++ita )
		all [ ita->first ] += ita->second;
}
