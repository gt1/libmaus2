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
#include <libmaus2/fastx/Phred.hpp>

double const libmaus2::fastx::Phred::phred_error[256] = {
 1,0.794328,0.630957,0.501187,0.398107,0.316228,0.251189,0.199526,0.158489,
 0.125893,0.1,0.0794328,0.0630957,0.0501187,0.0398107,0.0316228,0.0251189,
 0.0199526,0.0158489,0.0125893,0.01,0.00794328,0.00630957,0.00501187,0.00398107,
 0.00316228,0.00251189,0.00199526,0.00158489,0.00125893,0.001,0.000794328,
 0.000630957,0.000501187,0.000398107,0.000316228,0.000251189,0.000199526,
 0.000158489,0.000125893,0.0001,7.94328e-05,6.30957e-05,5.01187e-05,3.98107e-05,
 3.16228e-05,2.51189e-05,1.99526e-05,1.58489e-05,1.25893e-05,1e-05,7.94328e-06,
 6.30957e-06,5.01187e-06,3.98107e-06,3.16228e-06,2.51189e-06,1.99526e-06,
 1.58489e-06,1.25893e-06,1e-06,7.94328e-07,6.30957e-07,5.01187e-07,3.98107e-07,
 3.16228e-07,2.51189e-07,1.99526e-07,1.58489e-07,1.25893e-07,1e-07,7.94328e-08,
 6.30957e-08,5.01187e-08,3.98107e-08,3.16228e-08,2.51189e-08,1.99526e-08,
 1.58489e-08,1.25893e-08,1e-08,7.94328e-09,6.30957e-09,5.01187e-09,3.98107e-09,
 3.16228e-09,2.51189e-09,1.99526e-09,1.58489e-09,1.25893e-09,1e-09,7.94328e-10,
 6.30957e-10,5.01187e-10,3.98107e-10,3.16228e-10,2.51189e-10,1.99526e-10,
 1.58489e-10,1.25893e-10,1e-10,7.94328e-11,6.30957e-11,5.01187e-11,3.98107e-11,
 3.16228e-11,2.51189e-11,1.99526e-11,1.58489e-11,1.25893e-11,1e-11,7.94328e-12,
 6.30957e-12,5.01187e-12,3.98107e-12,3.16228e-12,2.51189e-12,1.99526e-12,
 1.58489e-12,1.25893e-12,1e-12,7.94328e-13,6.30957e-13,5.01187e-13,3.98107e-13,
 3.16228e-13,2.51189e-13,1.99526e-13,1.58489e-13,1.25893e-13,1e-13,7.94328e-14,
 6.30957e-14,5.01187e-14,3.98107e-14,3.16228e-14,2.51189e-14,1.99526e-14,
 1.58489e-14,1.25893e-14,1e-14,7.94328e-15,6.30957e-15,5.01187e-15,3.98107e-15,
 3.16228e-15,2.51189e-15,1.99526e-15,1.58489e-15,1.25893e-15,1e-15,7.94328e-16,
 6.30957e-16,5.01187e-16,3.98107e-16,3.16228e-16,2.51189e-16,1.99526e-16,
 1.58489e-16,1.25893e-16,1e-16,7.94328e-17,6.30957e-17,5.01187e-17,3.98107e-17,
 3.16228e-17,2.51189e-17,1.99526e-17,1.58489e-17,1.25893e-17,1e-17,7.94328e-18,
 6.30957e-18,5.01187e-18,3.98107e-18,3.16228e-18,2.51189e-18,1.99526e-18,
 1.58489e-18,1.25893e-18,1e-18,7.94328e-19,6.30957e-19,5.01187e-19,3.98107e-19,
 3.16228e-19,2.51189e-19,1.99526e-19,1.58489e-19,1.25893e-19,1e-19,7.94328e-20,
 6.30957e-20,5.01187e-20,3.98107e-20,3.16228e-20,2.51189e-20,1.99526e-20,
 1.58489e-20,1.25893e-20,1e-20,7.94328e-21,6.30957e-21,5.01187e-21,3.98107e-21,
 3.16228e-21,2.51189e-21,1.99526e-21,1.58489e-21,1.25893e-21,1e-21,7.94328e-22,
 6.30957e-22,5.01187e-22,3.98107e-22,3.16228e-22,2.51189e-22,1.99526e-22,
 1.58489e-22,1.25893e-22,1e-22,7.94328e-23,6.30957e-23,5.01187e-23,3.98107e-23,
 3.16228e-23,2.51189e-23,1.99526e-23,1.58489e-23,1.25893e-23,1e-23,7.94328e-24,
 6.30957e-24,5.01187e-24,3.98107e-24,3.16228e-24,2.51189e-24,1.99526e-24,
 1.58489e-24,1.25893e-24,1e-24,7.94328e-25,6.30957e-25,5.01187e-25,3.98107e-25,
 3.16228e-25,2.51189e-25,1.99526e-25,1.58489e-25,1.25893e-25,1e-25,7.94328e-26,
 6.30957e-26,5.01187e-26,3.98107e-26,3.16228e-26
};

/* program used to generate table above is below */

#if 0
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstring>

int main()
{
	std::ostringstream * lineostr = new std::ostringstream;
	*lineostr << " ";
	unsigned int const thres = 256;
	unsigned int const linelength = 80;

	std::cout << "double const phred_error[" << thres << "] = {\n";

	for ( unsigned int i = 0; i < thres; ++i )
	{
		double const base = 10;
		double const exponent = static_cast<double>(i) / 10.0;
		double const val = 1.0 / ::std::pow(base,exponent);

		std::ostringstream lostr;
		::std::setprecision(20);
		lostr << val;

		if ( lineostr->str().size() + strlen(",") + lostr.str().size() > linelength )
		{
			std::cout << lineostr->str() << std::endl;
			delete lineostr;
			lineostr = new std::ostringstream;
			(*lineostr) << " ";
		}

		(*lineostr) << val;

		if ( i+1 < thres )
		{
			(*lineostr) << ",";
		}
	}

	if ( lineostr->str().size() )
		std::cout << lineostr->str() << std::endl;

	delete lineostr;

	std::cout << "};\n";
}
#endif
