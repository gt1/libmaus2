/**
    libmaus
    Copyright (C) 2007-2012 Simon Gog  All Right Reserved.
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

#include <libmaus/util/NearestNeighbourDictionary.hpp>

uint64_t libmaus::util::NearestNeighbourDictionary::byteSize() const
{
	return
		m_abs_samples->byteSize() +
		m_differences->byteSize() +
		m_contains_abs_sample->byteSize() +
		2*sizeof(uint64_t);
}

uint64_t libmaus::util::NearestNeighbourDictionary::size() const
{
	return m_size;
}

/*
 * constructor. the code in of the constructor is imported from Simon Gog's SDSL library
 */
libmaus::util::NearestNeighbourDictionary::NearestNeighbourDictionary(::libmaus::bitio::BitVector const & v)
{
	assert ( nndblocksize );
	
	uint64_t max_distance_between_two_ones = 0;
	uint64_t ones = 0; // counter for the ones in v
	// get maximal distance between to ones in the bit vector
	// speed this up by broadword computing
	for (uint64_t i=0, last_one_pos_plus_1=0; i < v.size(); ++i) 
	{
		if ( (v)[i]) 
		{
			if (i+1-last_one_pos_plus_1 > max_distance_between_two_ones)
				max_distance_between_two_ones = i+1-last_one_pos_plus_1;
			last_one_pos_plus_1 = i+1;
			++ones;
		}
	}
	m_ones = ones;
	m_size = v.size();
	// initialize absolute samples m_abs_samples[0]=0
	::libmaus::bitio::CompactArray::unique_ptr_type tm_abs_samples(new ::libmaus::bitio::CompactArray( m_ones/nndblocksize + 1, ::libmaus::math::numbits(v.size()-1) ));
	m_abs_samples = UNIQUE_PTR_MOVE(tm_abs_samples);
	// initialize different values
	::libmaus::bitio::CompactArray::unique_ptr_type tm_differences(new ::libmaus::bitio::CompactArray( m_ones - m_ones/nndblocksize, ::libmaus::math::numbits(max_distance_between_two_ones) ));
	m_differences = UNIQUE_PTR_MOVE(tm_differences);
	// initialize m_contains_abs_sample
	::libmaus::bitio::IndexedBitVector::unique_ptr_type tm_contains_abs_sample(new ::libmaus::bitio::IndexedBitVector( (v.size()+nndblocksize-1)/nndblocksize ));
	m_contains_abs_sample = UNIQUE_PTR_MOVE(tm_contains_abs_sample);

	ones = 0;
	for (uint64_t i=0, last_one_pos=0; i < v.size(); ++i) 
	{
		if ( (v)[i]) 
		{
			++ones;
			if ((ones % nndblocksize) == 0) 
			{ 
				// insert absolute samples
				assert ( ones/nndblocksize < m_abs_samples->size() );
				(*m_abs_samples)[ones/nndblocksize] = i;
				assert ( i/nndblocksize < m_contains_abs_sample->size() );
				(*m_contains_abs_sample)[i/nndblocksize] = 1;
			}
			else
			{
				assert ( ones - ones/nndblocksize - 1 < m_differences->size() );
				(*m_differences)[ones - ones/nndblocksize - 1] = i - last_one_pos;
			}
			last_one_pos = i;
		}
	}
	m_contains_abs_sample->setupIndex();
}

/**
 * rank excluding index idx
 **/	
uint64_t libmaus::util::NearestNeighbourDictionary::rankm1(uint64_t idx) const
{
	assert(idx <= m_size);
	
	if ( idx == m_size )
		return m_ones;

	uint64_t r = m_contains_abs_sample->rankm1(idx/nndblocksize);
	uint64_t result = r*nndblocksize;
	uint64_t i = (*m_abs_samples)[r];
	while (++result <= m_ones) 
	{
		if ((result % nndblocksize) == 0) 
		{
			i = (*m_abs_samples)[result/nndblocksize];
		}
		else
		{
			i = i+(*m_differences)[result - result/nndblocksize-1];
		}
		if (i >= idx)
			return result-1;
	}
	return result-1;
}

/* ranks start at 1 */
uint64_t libmaus::util::NearestNeighbourDictionary::selectp(uint64_t i) const
{
	assert(i > 0 and i <= m_ones);
	uint64_t j = i/nndblocksize;
	uint64_t result = (*m_abs_samples)[j];
	j = j*nndblocksize - j;
	
	for (uint64_t end = j + (i%nndblocksize); j < end; ++j) 
	{
		result += (*m_differences)[j];
	}
	return result;
}

