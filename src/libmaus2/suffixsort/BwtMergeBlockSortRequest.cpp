/**
    libmaus2
    Copyright (C) 2009-2015 German Tischler
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
**/
#include <libmaus2/suffixsort/BwtMergeBlockSortRequest.hpp>

std::ostream & libmaus2::suffixsort::operator<<(std::ostream & out, libmaus2::suffixsort::BwtMergeBlockSortRequest const & o)
{
	out << "BwtMergeBlockSortRequest(";
	out << o.inputtype;
	out << ",";
	out << o.fn;
	out << ",";
	out << o.fs;
	out << ",";
	out << o.rlencoderblocksize;
	out << ",";
	out << o.isasamplingrate;
	out << ",";
	out << o.blockstart;
	out << ",";
	out << o.cblocksize;
	out << ",";
	out << "{";
	for ( uint64_t i = 0; i < o.zreqvec.size(); ++i )
	{
		out << o.zreqvec[i].getZAbsPos();
		if ( i+1 < o.zreqvec.size() )
			out << ";";
	}
	out << "}";
	out << ",";
	out << o.computeTermSymbolHwt;
	out << ",";
	out << o.lcpnext;
	out << ")";

	return out;
}
