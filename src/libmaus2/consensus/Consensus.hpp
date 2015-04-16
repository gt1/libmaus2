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
#if ! defined(LIBMAUS2_CONSENSUS_HPP)
#define LIBMAUS2_CONSENSUS_HPP

#include <libmaus2/fastx/FastQElement.hpp>
#include <libmaus2/util/Object.hpp>

#include <iostream>
#include <vector>
#include <cassert>

typedef int (*libmaus2_consensus_ConsensusComputationBase_computeConsensusX_wrapperC_type)(void const * thisptr, void * R, void const * V, int const verbose, void * ostr);
typedef int (*libmaus2_consensus_ConsensusComputationBase_construct_wrapperC_type)(void ** P);
typedef int (*libmaus2_consensus_ConsensusComputationBase_destruct_wrapperC_type)(void * P);

extern int libmaus2_consensus_ConsensusComputationBase_computeConsensusA_wrapper(void const * thisptr, void * R, void const * V, int const verbose, void * ostr);
extern int libmaus2_consensus_ConsensusComputationBase_computeConsensusQ_wrapper(void const * thisptr, void * R, void const * V, int const verbose, void * ostr);
extern int libmaus2_consensus_ConsensusComputationBase_construct_wrapper(void ** P);
extern int libmaus2_consensus_ConsensusComputationBase_destruct_wrapper(void * P);

extern "C" {
	extern int libmaus2_consensus_ConsensusComputationBase_computeConsensusA_wrapperC(void const * thisptr, void * R, void const * V, int const verbose, void * ostr);
	extern int libmaus2_consensus_ConsensusComputationBase_computeConsensusQ_wrapperC(void const * thisptr, void * R, void const * V, int const verbose, void * ostr);
	extern int libmaus2_consensus_ConsensusComputationBase_construct_wrapperC(void ** P);
	extern int libmaus2_consensus_ConsensusComputationBase_destruct_wrapperC(void * P);
}

namespace libmaus2
{
	namespace consensus
	{
		struct ConsensusComputationBase : public ::libmaus2::util::Object<ConsensusComputationBase>
		{
			typedef ConsensusComputationBase this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
		
			private:
			::libmaus2::util::ObjectBase::unique_ptr_type const mscoring;
			void computeConsensusA(void * R, void const * V, int const verbose, void * ostr) const;
			void computeConsensusQ(void * R, void const * V, int const verbose, void * ostr) const;
			static void computeConsensusA(void const * thisptr, void * R, void const * V, int const verbose, void * ostr);
			static void computeConsensusQ(void const * thisptr, void * R, void const * V, int const verbose, void * ostr);

			friend int ::libmaus2_consensus_ConsensusComputationBase_computeConsensusA_wrapper(void const * thisptr, void * R, void const * V, int const verbose, void * ostr);
			friend int ::libmaus2_consensus_ConsensusComputationBase_computeConsensusQ_wrapper(void const * thisptr, void * R, void const * V, int const verbose, void * ostr);
			friend int ::libmaus2_consensus_ConsensusComputationBase_construct_wrapper(void ** P);

			ConsensusComputationBase();
						
			public:
			static std::string computeConsensus(void const * obj, std::vector<std::string> const & V, int const verbose = false, std::ostream * ostr = 0);
			static std::string computeConsensus(void const * obj, std::vector< ::libmaus2::fastx::FastQElement > const & V, int const verbose = false, std::ostream * ostr = 0);
			static void construct(void ** P);
			static void destruct(void * P);
		};
		
		struct ConsensusComputation
		{
			typedef ConsensusComputation this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			// ::libmaus2::util::ObjectBase::unique_ptr_type obj;
			void * obj;
			
			ConsensusComputation() : obj(0)
			{
				ConsensusComputationBase::construct(&obj);
				// std::cerr << "Received pointer " << obj << " at top" << std::endl;
			}
			~ConsensusComputation()
			{
				//std::cerr << "Destructing...";
				ConsensusComputationBase::destruct(obj);
				// std::cerr << "done." << std::endl;
			}
			std::string computeConsensus(std::vector<std::string> const & V, int const verbose = false, std::ostream * ostr = 0)
			{
				if ( ! V.size() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Consensus of empty string set is undefined." << std::endl;
					se.finish();
					throw se;
				}
			
				return ConsensusComputationBase::computeConsensus(obj,V,verbose,ostr);
			}
			std::string computeConsensus(std::vector< ::libmaus2::fastx::FastQElement > const & V, int const verbose = false, std::ostream * ostr = 0)
			{
				if ( ! V.size() )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Consensus of empty string set is undefined." << std::endl;
					se.finish();
					throw se;
				}

				// std::cerr << "Sending ptr " << obj << " from top " << std::endl;
				return ConsensusComputationBase::computeConsensus(obj,V,verbose,ostr);			
			}
		};
	}
}
#endif
