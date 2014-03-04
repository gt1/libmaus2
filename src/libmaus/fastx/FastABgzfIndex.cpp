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

#include <libmaus/fastx/FastABgzfIndex.hpp>

::std::ostream & libmaus::fastx::operator<<(::std::ostream & out, ::libmaus::fastx::FastABgzfIndex const & F)
{
	out << "FastABgzfIndex(\n";
	out << "\tblocksize=" << F.blocksize << "\n";
	for ( uint64_t i = 0; i < F.entries.size(); ++i )
		out << "\tseq[" << i << "]=" << F.entries[i] << "\n";
	out << ")\n";
	return out;
}
