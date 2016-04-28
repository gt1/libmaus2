/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#include <libmaus2/gamma/GammaPDEncoder.hpp>
#include <libmaus2/gamma/GammaPDDecoder.hpp>
#include <libmaus2/random/Random.hpp>

int main()
{
	try
	{
		{
			libmaus2::random::Random::setup();

			std::vector<uint64_t> V;
			std::vector<std::string> Vfn;

			uint64_t numfiles = 20;
			uint64_t const blocksize = 63;
			uint64_t const base = 1041;
			uint64_t const mod = 3141;

			for ( uint64_t z = 0; z < numfiles; ++z )
			{
				std::ostringstream ostr;
				ostr << "mem://gamma_pd" << z;
				std::string const fn = ostr.str();
				Vfn.push_back(fn);
				libmaus2::gamma::GammaPDEncoder::unique_ptr_type Penc(new libmaus2::gamma::GammaPDEncoder(fn,blocksize));
				uint64_t n = base + (libmaus2::random::Random::rand64() % mod);
				std::cerr << "n=" << n << std::endl;
				for ( uint64_t i = 0; i < n; ++i )
				{
					V.push_back(libmaus2::random::Random::rand64() % 257);
					Penc->encode(V.back());
				}
				Penc.reset();
			}

			for ( uint64_t j = 0; j <= V.size(); ++j )
			{
				std::cerr << "j=" << j << "/" << V.size() << std::endl;

				libmaus2::gamma::GammaPDDecoder Pdec(Vfn,j);

				uint64_t v = 0;
				uint64_t i = j;
				while ( Pdec.decode(v) )
				{
					// std::cerr << v << " " << V[i] << std::endl;
					assert ( v == V[i] );

					i += 1;
				}
				assert ( i == V.size() );
			}
		}
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
