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
#include <libmaus2/clustering/HashCreation.hpp>

#include <libmaus2/consensus/Consensus.hpp>
#include <libmaus2/fastx/FastAReader.hpp>
#include <libmaus2/fastx/SocketFastAReader.hpp>

#if defined(LIBMAUS_HAVE_SEQAN)
void computeConsensus()
{
	::libmaus2::consensus::ConsensusComputation::unique_ptr_type CC(new ::libmaus2::consensus::ConsensusComputation);
	
	typedef ::libmaus2::fastx::SocketFastAReader reader_type;
	typedef reader_type::pattern_type pattern_type;
	
	::libmaus2::network::SocketBase sockbase(STDIN_FILENO);
	reader_type sockreader(&sockbase);

	std::vector<std::string> V;
	pattern_type pattern;
	
	while ( sockreader.getNextPatternUnlocked(pattern) )
	{
		V.push_back(pattern.spattern);
		std::cerr << V.back() << std::endl;
	}
	
	try
	{
		std::string const consensus = CC->computeConsensus(V,7,&(std::cerr));
		std::cout << "consensus: " << consensus << std::endl;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	CC.reset();		
}
#endif


int main()
{
	#if defined(LIBMAUS_HAVE_SEQAN)
	computeConsensus();
	#else
	std::cerr << "libmaus2 is compiled without SeqAN support. Consensus computation is thus not present." << std::endl;
	#endif
	           
	return 0;
}
