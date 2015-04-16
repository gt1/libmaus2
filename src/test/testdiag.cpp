/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#include <libmaus/math/ipow.hpp>
#include <libmaus/math/numbits.hpp>

struct MultipleAlignmentMatrixElement
{
	int64_t score;
	uint8_t * bits;
	
	MultipleAlignmentMatrixElement()
	: score(std::numeric_limits<int64_t>::min()), bits(0)
	{
	
	}
	
	operator bool() const
	{
		return score != std::numeric_limits<int64_t>::min();
	}
};

struct NumberBlockDiagonal
{
	typedef MultipleAlignmentMatrixElement element_type;

	int64_t  const r; // radius
	int64_t  const w; // width
	uint64_t const k; // number of dimensions
	uint64_t const k1;
	uint64_t const next;
	unsigned int const shift;
	uint64_t const blocklen;
	uint64_t const diag;
	libmaus::autoarray::AutoArray<element_type> A;

	NumberBlockDiagonal(
		uint64_t const rr, 
		uint64_t const rk,
		uint64_t const rdiag
	)
	: r(rr), 
	  w(2*rr+1), 
	  k(rk),
	  k1(k-1),
	  next(libmaus::math::nextTwoPow(w)),
	  shift(::libmaus::math::numbits(next-1)),
	  blocklen(::libmaus::math::ipow(next,k1)),
	  diag(rdiag),
	  A(blocklen*(diag+1))
	{
		assert ( k > 0 );
	}
	
	template<typename iterator>
	element_type const & operator()(iterator I) const
	{
		uint64_t i = 0;
		int64_t const off = I[0];
		for ( uint64_t j = 1; j < k; ++j )
		{
			i <<= shift;
			i |= (static_cast<int64_t>(I[j])+r-off);
		}
		i += off * blocklen;

		assert ( i < A.size() );
		return A[i];
	}

	template<typename iterator>
	element_type & operator()(iterator I)
	{
		uint64_t i = 0;
		int64_t const off = I[0];
		for ( uint64_t j = 1; j < k; ++j )
		{
			i <<= shift;
			i |= (static_cast<int64_t>(I[j])+r-off);
		}
		i += off * blocklen;
		
		assert ( i < A.size() );
		return A[i];
	}
	
	template<typename iterator>
	bool valid(iterator it)
	{
		if ( it[0] < 0 || it[0] >= diag+1 )
			return false;
		for ( uint64_t i = 1; i < k; ++i )
		{
			int64_t const s = static_cast<int64_t>(it[i])-static_cast<int64_t>(it[0]);
			if ( s < -r || s > r )
				return false;
		}
				
		return true;
	}
};

std::ostream & operator<<(std::ostream & out, NumberBlockDiagonal const & NB)
{
	for ( uint64_t i = 0; i < NB.A.size(); ++i )
	{
		uint64_t const mod = i % NB.blocklen;
		uint64_t const div = i / NB.blocklen;
		
		bool ok = div <= NB.diag;
		for ( uint64_t j = 1; j < NB.k; ++j )
		{
			int64_t const coord = static_cast<int64_t>((mod>>((NB.k-j-1)*NB.shift)) & ((1ull<<NB.shift)-1))-NB.r+div;
			ok = ok && (coord >= -NB.r && coord <= NB.r);
		}

		if ( ok && NB.A[i] )
		{
			out << "A(";
		
			out << div << ";";
		
			for ( uint64_t j = 1; j < NB.k; ++j )
			{
				int64_t const coord = static_cast<int64_t>((mod>>((NB.k-j-1)*NB.shift)) & ((1ull<<NB.shift)-1))-NB.r+div;
				out << coord << ((j+1<NB.k)?";":"");
			}
			
			out << ")=" << NB.A[i] << '\n';
		}
	}
	
	return out;
}

void align(std::vector<std::string> const & A, uint64_t const radius)
{
	NumberBlockDiagonal NB(radius,A.size(),A[0].size());
	
	{
		std::vector<uint64_t> V(NB.k);
		NB(V.begin()).score = 0;
	}

	std::vector<int64_t> coord(NB.k);	
	uint64_t const mask = (1ull<<NB.shift)-1;
	uint64_t const loops = ::libmaus::math::ipow(NB.w,NB.k1);
	
	for ( uint64_t z = 0; z < A[0].size(); ++z )
	{
		coord[0] = z;
	
		for ( uint64_t y = 0; y < loops; ++y )
		{
			bool ok = true;

			for ( uint64_t j = 1; j < NB.k; ++j )
			{
				coord[NB.k-j] = ((y/libmaus::math::ipow(NB.w,j-1))%NB.w) -NB.r+z;				
				ok = ok && coord[NB.k-j] >= 0;
			}
			
			if ( ok )
			{
				for ( uint64_t j = 0; j < coord.size(); ++j )
					std::cerr << coord[j] << ";";
				std::cerr << std::endl;
			}
		}
	}

	#if 0		
	std::cerr << NB;
	std::cerr << "loops=" << loops << std::endl;
	#endif
}

int main()
{
	std::vector<std::string> V;
	V.push_back("AACGTA");
	V.push_back("AAACGTA");
	V.push_back("ACACGTA");
	uint64_t const radius = 5;
	align(V,radius);
	
	#if 0
	NumberBlockDiagonal NB(5,3,5);
	std::cerr << NB.r << std::endl;
	std::cerr << NB.w << std::endl;
	std::cerr << NB.k << std::endl;
	std::cerr << NB.next << std::endl;
	std::cerr << NB.shift << std::endl;
	#endif

}
