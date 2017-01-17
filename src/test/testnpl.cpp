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

#include <libmaus2/lcs/NPL.hpp>

int main()
{
	try
	{
		{
			std::string a = "AACDE";
			std::string b = "AABCXDEAJICAJIJIA";
			libmaus2::lcs::NPL np;
			np.np(a.begin(),a.end(),b.begin(),b.end());

			std::cerr << np.traceToString() << std::endl;
		}

		{
			std::string b = "AACDE";
			std::string a = "AABCXDEAJICAJIJIA";
			libmaus2::lcs::NPL np;
			np.np(a.begin(),a.end(),b.begin(),b.end());

			std::cerr << np.traceToString() << std::endl;
		}

	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
