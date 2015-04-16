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

#include <libmaus/autoarray/AutoArray2d.hpp>
#include <iostream>

int main()
{
	try
	{
		uint64_t const n = 5;
		uint64_t const m = 7;
		libmaus::autoarray::AutoArray2d<uint64_t> A(n,m);
		libmaus::autoarray::AutoArray< libmaus::autoarray::AutoArray<uint64_t> > B(n);
		for ( uint64_t i = 0; i < n; ++i )
			B[i] = libmaus::autoarray::AutoArray<uint64_t>(m);
		bool ok = true;

		for ( uint64_t k = 0; ok && k < 10; ++k )
		{
			for ( uint64_t i = 0; i < n; ++i )
				for ( uint64_t j = 0; j < m; ++j )
				{
					A.at(i,j) = i*m+j;
					B.at(i).at(j) = i*m+j;
				}
			for ( uint64_t i = 0; i < n; ++i )
				for ( uint64_t j = 0; j < m; ++j )
				{
					ok = ok && (A.at(i,j) == i*m+j);
					assert ( B.at(i).at(j) == A.at(i,j) );
				}

			for ( uint64_t i = 0; i < n; ++i )
			{
				A.prefixSums(i);
				B[i].prefixSums();
			}

			for ( uint64_t i = 0; i < n; ++i )
				for ( uint64_t j = 0; j < m; ++j )
				{
					assert ( B.at(i).at(j) == A.at(i,j) );
				}
		}
		
		std::cerr << (ok ? "ok" : "failed") << std::endl;
	}
	catch(std::exception const & ex)
	{
	
	}
}
