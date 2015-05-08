/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#include <libmaus2/dazzler/db/HitsIndexBase.hpp>

std::ostream & libmaus2::dazzler::db::operator<<(std::ostream & out, libmaus2::dazzler::db::HitsIndexBase const & H)
{
	return out << "HitsIndexBase("
		<< "ureads=" << H.ureads << ","
		<< "treads=" << H.treads << ","
		<< "cutoff=" << H.cutoff << ","
		<< "all=" << H.all << ","
		<< "freq[A]=" << H.freq[0] << ","
		<< "freq[C]=" << H.freq[1] << ","
		<< "freq[G]=" << H.freq[2] << ","
		<< "freq[T]=" << H.freq[3] << ","
		<< "maxlen=" << H.maxlen << ","
		<< "totlen=" << H.totlen << ","
		<< "nreads=" << H.nreads << ","
		<< "trimmed=" << H.trimmed << ")";
}
