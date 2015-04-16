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

#include <libmaus/util/ForkProcessLSFSim.hpp>
#include <iostream>

int main()
{
	::libmaus::util::ForkProcessLSFSim sim(
		std::string("/bin/cat"),
		std::string("localjob"),
		std::string("localproject"),
		std::string("local"),
		1,
		64,
		std::string("configure"),
		std::string("deb.out"),
		std::string("deb.err"),
		0,0
	);
	
	sim.wait();

	if ( sim.finishedOk() )	
		std::cerr << "finished ok: " << sim.finishedOk() << std::endl;
	else
		std::cerr << "failed: " << std::endl << sim.failmessage << std::endl;
}
