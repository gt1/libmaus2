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

#include <iostream>
#include <string>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/suffixsort/divsufsort.hpp>
#include <libmaus2/util/GetFileSize.hpp>

template<typename SUF_TYPE>
::libmaus2::autoarray::AutoArray< typename SUF_TYPE::value_type > computeISA(SUF_TYPE & SUF, uint64_t const n)
{
	// compute shifted inverse of suffix array extended by one
	::libmaus2::autoarray::AutoArray< typename SUF_TYPE::value_type > ISA(n,false);
	for ( size_t i = 0; i < n; ++i ) ISA[SUF[i]] = i;
	return ISA;
}

template<typename SUF_TYPE>
::libmaus2::autoarray::AutoArray< typename SUF_TYPE::value_type > computeISU(SUF_TYPE & SUF, uint64_t const n)
{
	// compute shifted inverse of suffix array extended by one
	::libmaus2::autoarray::AutoArray< typename SUF_TYPE::value_type > ISU(n+1,false);
	for ( size_t i = 0; i < n; ++i ) ISU[SUF[i]] = i+1; ISU[n] = 0;
	return ISU;
}

// see Bannai et alia "Inferring Strings from Graphs and Arrays"
template<typename SUF_TYPE>
std::wstring sufToString(SUF_TYPE & SUF, size_t const n)
{
	::libmaus2::autoarray::AutoArray< typename SUF_TYPE::value_type > ISU = computeISU(SUF,n);
	wchar_t c = 'a';
	std::wstring s(n,0);
	s[SUF[0]] = c;
	for ( size_t i = 0; i < (n-1); ++i )
	{
		// do we need to introduce a new symbol when we go from rank i to rank i+1?
		if ( ISU[SUF[i]+1] > ISU[SUF[i+1]+1] )
			++c;
		s[SUF[i+1]] = c;
	}

	return s;
}

int main(int argc, char * argv[])
{
	if ( argc < 2 )
		return EXIT_FAILURE;

	::libmaus2::autoarray::AutoArray<uint8_t> data = ::libmaus2::util::GetFileSize::readFile(argv[1]);
	std::basic_string<wchar_t> const s(data.begin(),data.end());
	uint64_t const n = s.size();

	unsigned int const bitwidth = 64;
	typedef ::libmaus2::suffixsort::DivSufSort<bitwidth,wchar_t *,wchar_t const *,int64_t *,int64_t const *,4096> sort_type;
	typedef sort_type::saidx_t saidx_t;

	::libmaus2::autoarray::AutoArray<saidx_t> SAdiv0(n,false);

	std::cerr << "Running divsufsort...";
	sort_type::divsufsort ( reinterpret_cast<wchar_t const *>(s.c_str()) , SAdiv0.get() , n );
	std::cerr << "done." << std::endl;

	if ( n < 20 )
		for ( uint64_t r = 0; r < n; ++r )
		{
			std::wcerr << r << "\t" << SAdiv0[r] << "\t" << s.substr(SAdiv0[r]) << std::endl;
		}

	std::wstring t = sufToString(SAdiv0,n);

	// std::wcerr << t << std::endl;

	// return 0;

	::libmaus2::autoarray::AutoArray < int64_t > ISA = computeISA(SAdiv0,n);

	std::wstring x = sufToString(ISA,n);

	// std::wcerr << x << std::endl;

	::libmaus2::autoarray::AutoArray<saidx_t> ISAdiv0(n,false);
	sort_type::divsufsort ( reinterpret_cast<wchar_t const *>(x.c_str()) , ISAdiv0.get() , n );

	for ( uint64_t i = 0; i < n; ++i )
		assert ( ISAdiv0[i] == ISA[i] );
}
