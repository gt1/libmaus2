/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#include <libmaus2/math/Matrix.hpp>

int main()
{
	try
	{
		using libmaus2::math::Matrix;

		Matrix<double> A(3,3);

		A(0,0) = A(1,1) = A(2,2) = 1;

		Matrix<double> B(3,3);
		B(0,0) = 1;
		B(0,1) = 2;
		B(0,2) = 3;
		B(1,0) = 4;
		B(1,1) = 5;
		B(1,2) = 6;
		B(2,0) = 7;
		B(2,1) = 8;
		B(2,2) = 9;

		std::cout << A;
		std::cout << B;
		std::cout << A*B;

		Matrix<double> C(2,3);
		C(0,0) = 1;
		C(0,1) = 2;
		C(0,2) = 3;
		C(1,0) = 4;
		C(1,1) = 5;
		C(1,2) = 6;

		Matrix<double> D(3,2);
		D(0,0) = 7;
		D(0,1) = 8;
		D(1,0) = 9;
		D(1,1) = 10;
		D(2,0) = 11;
		D(2,1) = 12;

		std::cout << C << std::endl;
		std::cout << D << std::endl;
		std::cout << C*D << std::endl;

		Matrix<double> E(3,3);
		E(0,0) = 3;
		E(0,1) = 0;
		E(0,2) = 2;
		E(1,0) = 2;
		E(1,1) = 0;
		E(1,2) = -2;
		E(2,0) = 0;
		E(2,1) = 1;
		E(2,2) = 1;

		Matrix<double> EI = E.inverse(1e-6);

		std::cout << E*EI << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
