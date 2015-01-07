/*
    libmaus
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#include <libmaus/random/Random.hpp>
#include <libmaus/util/ArgInfo.hpp>
#include <libmaus/bambam/BamWriter.hpp>

int main(int argc, char * argv[])
{
	try
	{
		libmaus::util::ArgInfo const arginfo(argc,argv);
		uint64_t const textlen = arginfo.getValue<int>("textlen",120);
		uint64_t const readlen = arginfo.getValue<int>("readlen",110);
		uint64_t const numreads = (textlen-readlen)+1;
		libmaus::autoarray::AutoArray<char> T(textlen,false);
		libmaus::random::Random::setup(19);
		for ( uint64_t i = 0; i < textlen; ++i )
		{
			switch ( libmaus::random::Random::rand8() % 4 )
			{
				case 0: T[i] = 'A'; break;
				case 1: T[i] = 'C'; break;
				case 2: T[i] = 'G'; break;
				case 3: T[i] = 'T'; break;
			}
		}

		::libmaus::bambam::BamHeader header;
		header.addChromosome("text",textlen);
		
		std::vector<uint64_t> P;
		for ( uint64_t i = 0; i < numreads; ++i )
			P.push_back(i);

		uint64_t const check = std::min(static_cast<uint64_t>(arginfo.getValue<int>("check",8)),P.size());		
		std::vector<uint64_t> prev(check,numreads);

		do
		{		
			std::ostringstream out;
			::libmaus::bambam::BamWriter::unique_ptr_type bamwriter(new ::libmaus::bambam::BamWriter(out,header,0,0));
			
			bool print = false;
			for ( uint64_t i = 0; i < check; ++i )
				if ( P[i] != prev[i] )
					print = true;

			if ( print )
			{
				for ( uint64_t i = 0; i < check; ++i )
				{
					std::cerr << P[i] << ";";
					prev[i] = P[i];
				}
				std::cerr << std::endl;
			}
					
			for ( uint64_t j = 0; j < P.size(); ++j )
			{
				uint64_t const i = P[j];
				
				std::ostringstream rnstr;
				rnstr << "r" << "_" << std::setw(6) << std::setfill('0') << i;
				std::string const rn = rnstr.str();
				
				std::string const read(T.begin()+i,T.begin()+i+readlen);
				// std::cerr << read << std::endl;

				bamwriter->encodeAlignment(rn,0 /* refid */,i,30, 0, 
					libmaus::util::NumberSerialisation::formatNumber(readlen,0) + "M", 
					-1,-1, -1, read, std::string(readlen,'H'));
				bamwriter->commit();
			}
			
			bamwriter.reset();
		} while ( std::next_permutation(P.begin(),P.end()) );
		
		// std::cout << out.str();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
