/*
    libmaus
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
#include <libmaus/consensus/Consensus.hpp>
#include <libmaus/math/binom.hpp>

#if defined(LIBMAUS_HAVE_DL_FUNCS)
#include <libmaus/util/DynamicLoading.hpp>
#endif

#if defined(LIBMAUS_HAVE_SEQAN)

std::string libmaus::consensus::ConsensusComputationBase::computeConsensus(
	void const * obj,
	std::vector< ::libmaus::fastx::FastQElement > const & V, int const verbose, std::ostream * ostr
)
{
	#if defined(LIBMAUS_HAVE_DL_FUNCS)
	::libmaus::util::DynamicLibraryFunction<libmaus_consensus_ConsensusComputationBase_computeConsensusX_wrapperC_type> DLF(
		"libmaus_consensus_mod.so","libmaus_consensus_ConsensusComputationBase_computeConsensusQ_wrapperC");
	
	std::string R;
	int const r = DLF.func(obj,&R,&V,verbose,ostr);
	
	if ( r < 0 )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "libmaus::consensus::ConsensusComputationBase::computeConsensus failed." << std::endl;
		se.finish();
		throw se;
	}
	
	return R;
	#else
	::libmaus::exception::LibMausException se;
	se.getStream() << "libmaus::consensus::ConsensusComputationBase::computeConsensus called but dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
	#endif
}

std::string libmaus::consensus::ConsensusComputationBase::computeConsensus(
	void const * obj,
	std::vector<std::string> const & V, int const verbose, std::ostream * ostr)
{
	#if defined(LIBMAUS_HAVE_DL_FUNCS)
	::libmaus::util::DynamicLibraryFunction<libmaus_consensus_ConsensusComputationBase_computeConsensusX_wrapperC_type> DLF(
		"libmaus_consensus_mod.so","libmaus_consensus_ConsensusComputationBase_computeConsensusA_wrapperC");

	std::string R;
	int const r = DLF.func(obj,&R,&V,verbose,ostr);

	if ( r < 0 )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "libmaus::consensus::ConsensusComputationBase::computeConsensus failed." << std::endl;
		se.finish();
		throw se;
	}

	return R;
	#else
	::libmaus::exception::LibMausException se;
	se.getStream() << "libmaus::consensus::ConsensusComputationBase::computeConsensus called but dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
	#endif
}

void libmaus::consensus::ConsensusComputationBase::construct(void ** P)
{
	#if defined(LIBMAUS_HAVE_DL_FUNCS)
	::libmaus::util::DynamicLibraryFunction<libmaus_consensus_ConsensusComputationBase_construct_wrapperC_type> DLF(
		"libmaus_consensus_mod.so","libmaus_consensus_ConsensusComputationBase_construct_wrapperC");

	int const r = DLF.func(P);
	
	if ( r < 0 )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "libmaus::consensus::ConsensusComputationBase::construct failed." << std::endl;
		se.finish();
		throw se;
	}
	#else
	::libmaus::exception::LibMausException se;
	se.getStream() << "libmaus::consensus::ConsensusComputationBase::construct called but dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
	#endif
	
	// std::cerr << "Received ptr " << *P << " stored at " << P << std::endl;
}

void libmaus::consensus::ConsensusComputationBase::destruct(void * P)
{
	#if defined(LIBMAUS_HAVE_DL_FUNCS)
	::libmaus::util::DynamicLibraryFunction<libmaus_consensus_ConsensusComputationBase_destruct_wrapperC_type> DLF(
		"libmaus_consensus_mod.so","libmaus_consensus_ConsensusComputationBase_destruct_wrapperC");

	int const r = DLF.func(P);
	
	if ( r < 0 )
	{
		::libmaus::exception::LibMausException se;
		se.getStream() << "libmaus::consensus::ConsensusComputationBase::destruct failed." << std::endl;
		se.finish();
		throw se;
	}
	#else
	::libmaus::exception::LibMausException se;
	se.getStream() << "libmaus::consensus::ConsensusComputationBase::destruct called but dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
	#endif
}
#endif
