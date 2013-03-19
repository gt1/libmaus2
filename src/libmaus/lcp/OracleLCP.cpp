/**
    libmaus
    Copyright (C) 2008 Simon Puglisi
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
**/

#include "OracleLCP.hpp"

static const uint32_t cover0[] = {0};
static const uint32_t cover1[] = {0,1};
static const uint32_t cover2[] = {0,1,2};
static const uint32_t cover3[] = {0,1,2,4};
static const uint32_t cover4[] = {0,1,2,5,8};
static const uint32_t cover5[] = {0,1,2,3,7,11,19};   //{0,7,8,10,14,19,23};
static const uint32_t cover6[] = {0,1,2,5,14,16,34,42,59};
static const uint32_t cover7[] = {0,1,3,7,17,40,55,64,75,85,104,109,117};
static const uint32_t cover8[] = {0,1,3,7,
                                  12,20,30,44,
                                  65,80,89,96,
                                  114,122,128,150,
                                  196,197,201,219};
const uint32_t * libmaus::lcp::OracleLCPBase::_covers[] = { cover0, cover1, cover2, cover3, cover4, cover5, cover6, cover7, cover8 };
const uint32_t libmaus::lcp::OracleLCPBase::max_precomputed_cover = sizeof(_covers)/sizeof(_covers[0])-1;
const uint32_t libmaus::lcp::OracleLCPBase::_cover_sizes[max_precomputed_cover+1] = {
   sizeof(cover0)/sizeof(cover0[0]),
   sizeof(cover1)/sizeof(cover1[0]),
   sizeof(cover2)/sizeof(cover2[0]),
   sizeof(cover3)/sizeof(cover3[0]),
   sizeof(cover4)/sizeof(cover4[0]),
   sizeof(cover5)/sizeof(cover5[0]),
   sizeof(cover6)/sizeof(cover6[0]),
   sizeof(cover7)/sizeof(cover7[0]),
   sizeof(cover8)/sizeof(cover8[0])
};

