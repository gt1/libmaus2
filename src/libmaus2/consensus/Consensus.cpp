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
#include <libmaus2/consensus/Consensus.hpp>
#include <libmaus2/math/binom.hpp>

#if defined(LIBMAUS2_HAVE_DL_FUNCS)
#include <libmaus2/util/DynamicLoading.hpp>
#endif

#if defined(LIBMAUS2_HAVE_SEQAN)

std::vector<std::string> libmaus2::consensus::ConsensusComputationBase::computeMultipleAlignment(
	void const * obj,
	std::vector< std::string > const & V
)
{
	#if defined(LIBMAUS2_HAVE_DL_FUNCS)
	::libmaus2::util::DynamicLibraryFunction<libmaus2_consensus_ConsensusComputationBase_computeMultipleAlignment_wrapperC_type> DLF(
		"libmaus2_consensus_mod.so","libmaus2_consensus_ConsensusComputationBase_computeMultipleAlignment_wrapperC");
	
	std::vector<std::string> R;
	int const r = DLF.func(obj,&R,&V);
	
	if ( r < 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "libmaus2::consensus::ConsensusComputationBase::computeMultipleAlignment failed." << std::endl;
		se.finish();
		throw se;
	}
	
	return R;
	#else
	::libmaus2::exception::LibMausException se;
	se.getStream() << "libmaus2::consensus::ConsensusComputationBase::computeMultipleAlignment called but dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
	#endif
}

std::string libmaus2::consensus::ConsensusComputationBase::computeConsensus(
	void const * obj,
	std::vector< ::libmaus2::fastx::FastQElement > const & V, int const verbose, std::ostream * ostr
)
{
	#if defined(LIBMAUS2_HAVE_DL_FUNCS)
	::libmaus2::util::DynamicLibraryFunction<libmaus2_consensus_ConsensusComputationBase_computeConsensusX_wrapperC_type> DLF(
		"libmaus2_consensus_mod.so","libmaus2_consensus_ConsensusComputationBase_computeConsensusQ_wrapperC");
	
	std::string R;
	int const r = DLF.func(obj,&R,&V,verbose,ostr);
	
	if ( r < 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "libmaus2::consensus::ConsensusComputationBase::computeConsensus failed." << std::endl;
		se.finish();
		throw se;
	}
	
	return R;
	#else
	::libmaus2::exception::LibMausException se;
	se.getStream() << "libmaus2::consensus::ConsensusComputationBase::computeConsensus called but dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
	#endif
}

std::string libmaus2::consensus::ConsensusComputationBase::computeConsensus(
	void const * obj,
	std::vector<std::string> const & V, int const verbose, std::ostream * ostr)
{
	#if defined(LIBMAUS2_HAVE_DL_FUNCS)
	::libmaus2::util::DynamicLibraryFunction<libmaus2_consensus_ConsensusComputationBase_computeConsensusX_wrapperC_type> DLF(
		"libmaus2_consensus_mod.so","libmaus2_consensus_ConsensusComputationBase_computeConsensusA_wrapperC");

	std::string R;
	int const r = DLF.func(obj,&R,&V,verbose,ostr);

	if ( r < 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "libmaus2::consensus::ConsensusComputationBase::computeConsensus failed." << std::endl;
		se.finish();
		throw se;
	}

	return R;
	#else
	::libmaus2::exception::LibMausException se;
	se.getStream() << "libmaus2::consensus::ConsensusComputationBase::computeConsensus called but dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
	#endif
}

void libmaus2::consensus::ConsensusComputationBase::construct(void ** P)
{
	#if defined(LIBMAUS2_HAVE_DL_FUNCS)
	::libmaus2::util::DynamicLibraryFunction<libmaus2_consensus_ConsensusComputationBase_construct_wrapperC_type> DLF(
		"libmaus2_consensus_mod.so","libmaus2_consensus_ConsensusComputationBase_construct_wrapperC");

	int const r = DLF.func(P);
	
	if ( r < 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "libmaus2::consensus::ConsensusComputationBase::construct failed." << std::endl;
		se.finish();
		throw se;
	}
	#else
	::libmaus2::exception::LibMausException se;
	se.getStream() << "libmaus2::consensus::ConsensusComputationBase::construct called but dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
	#endif
	
	// std::cerr << "Received ptr " << *P << " stored at " << P << std::endl;
}

void libmaus2::consensus::ConsensusComputationBase::destruct(void * P)
{
	#if defined(LIBMAUS2_HAVE_DL_FUNCS)
	::libmaus2::util::DynamicLibraryFunction<libmaus2_consensus_ConsensusComputationBase_destruct_wrapperC_type> DLF(
		"libmaus2_consensus_mod.so","libmaus2_consensus_ConsensusComputationBase_destruct_wrapperC");

	int const r = DLF.func(P);
	
	if ( r < 0 )
	{
		::libmaus2::exception::LibMausException se;
		se.getStream() << "libmaus2::consensus::ConsensusComputationBase::destruct failed." << std::endl;
		se.finish();
		throw se;
	}
	#else
	::libmaus2::exception::LibMausException se;
	se.getStream() << "libmaus2::consensus::ConsensusComputationBase::destruct called but dynamic loading is not supported." << std::endl;
	se.finish();
	throw se;
	#endif
}
#endif
