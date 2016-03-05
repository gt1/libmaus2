/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#include <libmaus2/util/SplayTree.hpp>
#include <set>
#include <libmaus2/random/Random.hpp>

template<typename value_type>
bool addElement(libmaus2::util::SplayTree<value_type> & ST, std::set<value_type> & S, value_type const & v)
{
	ST.insert(v);

	if ( ! ST.check() )
		return false;

	S.insert(v);

	std::vector<value_type> V;
	ST.extract(V);

	#if 0
	for ( uint64_t i = 0; i < V.size(); ++i )
		std::cerr << V[i] << ";";
	std::cerr << std::endl;
	#endif

	if ( std::set<value_type>(V.begin(),V.end()) != S )
		return false;

	if ( S.size() )
		if ( ST.getKey(ST.root) != v )
			return false;

	return true;
}

template<typename value_type>
bool eraseElement(libmaus2::util::SplayTree<value_type> & ST, std::set<value_type> & S, value_type const & v)
{
	ST.erase(v);

	if ( ! ST.check() )
	{
		assert ( false );
		return false;
	}

	S.erase(v);

	std::vector<value_type> V;
	ST.extract(V);

	#if 0
	for ( uint64_t i = 0; i < V.size(); ++i )
		std::cerr << V[i] << ";";
	std::cerr << std::endl;
	#endif

	if ( std::set<value_type>(V.begin(),V.end()) != S )
	{
		std::cerr << "sequence mismatch" << std::endl;
		return false;
	}

	return true;
}

void test1()
{
	typedef int value_type;
	libmaus2::util::SplayTree<value_type> ST;

	std::set<value_type> S;

	addElement(ST,S,5);
	addElement(ST,S,3);
	addElement(ST,S,7);
	addElement(ST,S,2);
	addElement(ST,S,1);
	addElement(ST,S,6);

	bool ok = true;

	for ( value_type i = 0; ok && i < 2048; ++i )
	{
		ok = ok && addElement(ST,S,i);
		ok = ok && addElement(ST,S,4096-i);
	}

	assert ( ok );

	eraseElement(ST,S,7);
	eraseElement(ST,S,5);
	eraseElement(ST,S,1);
	eraseElement(ST,S,6);
	eraseElement(ST,S,3);
	eraseElement(ST,S,2);

	for ( value_type i = 0; ok && i < 2048; ++i )
	{
		ok = ok && eraseElement(ST,S,i);
		ok = ok && eraseElement(ST,S,4096-i);
	}
}

void test2()
{
	typedef uint64_t value_type;
	libmaus2::util::SplayTree<value_type> ST;

	libmaus2::random::Random::setup();

	std::vector<value_type> V;
	for ( uint64_t i = 0; i < 4096; ++i )
		V.push_back(libmaus2::random::Random::rand64());

	std::set<value_type> S;

	bool ok = true;
	for ( uint64_t i = 0; ok && i < V.size(); ++i )
	{
		bool lok = addElement(ST,S,V[i]);

		if ( ! lok )
		{
			std::vector<uint64_t> prefix;
			for ( uint64_t j = 0; j <= i; ++j )
				prefix.push_back(V[j]);
			std::sort(prefix.begin(),prefix.end());

			std::cerr << "inserted sequence: ";
			for ( uint64_t j = 0; j <= i; ++j )
				std::cerr << std::lower_bound(prefix.begin(),prefix.end(),V[j])-prefix.begin() << ";";
			std::cerr << std::endl;

			assert ( lok );
		}

		ok = ok && lok;
	}

	for ( std::set<value_type>::const_iterator ita = S.begin(); ita != S.end(); ++ita )
	{
		value_type const v = *ita;
		assert ( ST.find(v) != -1 );
		assert ( ST.getKey(ST.find(v)) == v );

		std::set<value_type>::const_iterator itn = ita;
		itn++;

		if ( itn != S.end() )
		{
			assert ( ST.getNext(ST.find(v)) != -1 );
			assert ( ST.getKey(ST.getNext(ST.find(v))) == *itn );
		}
		else
		{
			assert ( ST.getNext(ST.find(v)) == -1 );
		}

		if ( ita == S.begin() )
			assert ( ST.getPrev(ST.find(v)) == -1 );
		else
		{
			std::set<value_type>::const_iterator itp = ita;
			--itp;
			assert ( ST.getPrev(ST.find(v)) != -1 );
			assert ( ST.getKey(ST.getPrev(ST.find(v))) == *itp );
		}
	}

	for ( uint64_t i = 0; ok && i < V.size(); ++i )
	{
		bool const lok = eraseElement(ST,S,V[i]);
		assert ( lok );
		ok = ok && lok;
	}
}

void test3()
{
	typedef uint64_t value_type;
	libmaus2::util::SplayTree<value_type> ST;
	value_type const T[] = { 22,1,21,19,2,11,32,28,26,29,10,17,12,6,13,14,5,3,23,16,24,18,4,8,31,15,9,7,25,0,27,30,20 };
	std::set<value_type> S;

	for ( uint64_t i = 0; i < sizeof(T)/sizeof(T[0]); ++i )
	{
		bool const ok = addElement(ST,S,T[i]);
		assert ( ok );
	}
}

int main()
{
	try
	{
		test1();
		test3();

		#if defined(_OPENMP)
		#pragma omp parallel for
		#endif
		for ( uint64_t i = 0; i < 128; ++i )
		{
			test2();
			std::cerr.put('.');
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
