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
#include <libmaus2/sorting/SortingBufferedOutputFile.hpp>

struct Obj
{
	uint64_t z;

	Obj() {}
	Obj(uint64_t const rz) : z(rz) {}
	Obj(std::istream & in)
	{
		z = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
		for ( uint64_t i = 0; i < z; ++i )
			in.get();
	}

	std::ostream & serialise(std::ostream & out) const
	{
		libmaus2::util::NumberSerialisation::serialiseNumber(out,z);
		for ( uint64_t i = 0; i < z; ++i )
			out.put(0);
		return out;
	}

	std::istream & deserialise(std::istream & in)
	{
		*this = Obj(in);
		return in;
	}

	bool operator<(Obj const & obj) const
	{
		return z < obj.z;
	}

	bool operator==(Obj const & obj) const
	{
		return z == obj.z;
	}
};

void checkSortingSerialised()
{
	std::string const sofile = "mem:so";
	libmaus2::sorting::SerialisingSortingBufferedOutputFile<Obj>::unique_ptr_type SSBOF(
		new libmaus2::sorting::SerialisingSortingBufferedOutputFile<Obj>(sofile));

	uint64_t const n = 1024 * 512 + 10410;
	std::vector<Obj> V;
	for ( uint64_t i = 0; i < n; ++i )
	{
		Obj obj(i%7);
		V.push_back(obj);
		SSBOF->put(obj);
	}
	std::sort(V.begin(),V.end());

	libmaus2::sorting::SerialisingSortingBufferedOutputFile<Obj>::merger_ptr_type Pmerger(SSBOF->getMerger());

	Obj pre;
	bool prevalid = false;
	Obj obj;
	uint64_t c = 0;
	while ( Pmerger->getNext(obj) )
	{
		//std::cerr << obj.z << std::endl;

		assert (
			(!prevalid)
			||
			(!(obj < pre))
		);
		assert ( obj == V[c] );
		prevalid = true;
		c += 1;
	}

	assert ( c == n );
}

int main(int /* argc */, char * /* argv */[])
{
	try
	{
		checkSortingSerialised();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
