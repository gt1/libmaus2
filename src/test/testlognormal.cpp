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
#include <libmaus2/random/LogNormalRandom.hpp>
#include <libmaus2/fastx/StreamFastAReader.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus2::fastx::StreamFastAReaderWrapper SF(std::cin);
		libmaus2::fastx::StreamFastAReaderWrapper::pattern_type pattern;
		std::vector<double> V;
		while ( SF.getNextPatternUnlocked(pattern) )
			V.push_back(pattern.spattern.size());

		std::pair<double,double> est = libmaus2::random::LogNormalRandom::estimateParameters(V);

		double const mu = est.first;
		double const sigma = est.second;

		std::cerr << "mu=" << ::std::exp(mu + sigma*sigma/2.0) << std::endl;

		for ( double x = 0; x <= 1.0; x += 0.001 )
			std::cout << x << "\t" << libmaus2::random::LogNormalRandom::search(x,sigma,mu) << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
