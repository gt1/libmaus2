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

#include <libmaus2/lcs/OverlapComputation.hpp>

/* minimum overlap */
unsigned long const ::libmaus2::lcs::OverlapComputation::defaultmintracelength =
	::libmaus2::lcs::OverlapComputation::getDefaultMinTraceLength();
/* max allowed indels in overlap */
double const ::libmaus2::lcs::OverlapComputation::defaultmaxindelfrac =
	::libmaus2::lcs::OverlapComputation::getDefaultMaxIndelFrac();
/* max allowed substitutions in overlap */
double const ::libmaus2::lcs::OverlapComputation::defaultmaxsubstfrac =
	::libmaus2::lcs::OverlapComputation::getDefaultMaxSubstFrac();
/* minimum overlap */
int64_t const ::libmaus2::lcs::OverlapComputation::defaultminscore =
	::libmaus2::lcs::OverlapComputation::getDefaultMinScore();
