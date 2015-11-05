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

#if ! defined(OVERLAPCOMPUTATIONBLOCKREQUEST_HPP)
#define OVERLAPCOMPUTATIONBLOCKREQUEST_HPP

#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/StringSerialisation.hpp>
#include <libmaus2/fastx/FastInterval.hpp>
#include <libmaus2/util/GenericSerialisation.hpp>
#include <fstream>

namespace libmaus2
{
	namespace lcs
	{
		struct OverlapComputationBlockRequest
		{
			typedef OverlapComputationBlockRequest this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			uint64_t blockid;
			uint64_t numblocks;
			uint64_t subblockid;
			uint64_t numsubblocks;
			::libmaus2::fastx::FastInterval FI;
			::libmaus2::fastx::FastInterval SUBFI;
			std::vector < ::libmaus2::fastx::FastInterval > sublo;
			std::vector < std::string > inputfiles;
			uint64_t numedges;
			std::string serverhostname;
			unsigned short serverport;
			bool const isfastq;

			unsigned int mintracelength;
			int64_t minscore;
			uint64_t scorewindowsize;
			int64_t windowminscore;
			double maxindelfrac;
			double maxsubstfrac;

			OverlapComputationBlockRequest()
			: blockid(0), numblocks(0), subblockid(0), numsubblocks(0), FI(), SUBFI(), sublo(), inputfiles(),  numedges(0), \
			  serverhostname(), serverport(0), isfastq(0), mintracelength(40), minscore(0),
			  scorewindowsize(0),
			  windowminscore(0),
			  maxindelfrac(0.01), maxsubstfrac(0.03) {}

			OverlapComputationBlockRequest(
				uint64_t const rblockid,
				uint64_t const rnumblocks,
				uint64_t const rsubblockid,
				uint64_t const rnumsubblocks,
				::libmaus2::fastx::FastInterval const & rFI,
				::libmaus2::fastx::FastInterval const & rSUBFI,
				std::vector < ::libmaus2::fastx::FastInterval > const & rsublo,
				std::vector < std::string > const & rinputfiles,
				uint64_t const & rnumedges,
				std::string const & rserverhostname,
				unsigned short const & rserverport,
				bool const & risfastq,
				unsigned int const rmintracelength,
				int64_t const rminscore,
				uint64_t const rscorewindowsize,
				int64_t const rwindowminscore,
				double const rmaxindelfrac,
				double const rmaxsubstfrac
			) : blockid(rblockid), numblocks(rnumblocks), subblockid(rsubblockid), numsubblocks(rnumsubblocks),
			    FI(rFI), SUBFI(rSUBFI), sublo(rsublo), inputfiles(rinputfiles), numedges(rnumedges),
			    serverhostname(rserverhostname), serverport(rserverport), isfastq(risfastq),
			    mintracelength(rmintracelength), minscore(rminscore),
			    scorewindowsize(rscorewindowsize), windowminscore(rwindowminscore),
			    maxindelfrac(rmaxindelfrac), maxsubstfrac(rmaxsubstfrac)
			{}

			OverlapComputationBlockRequest(std::istream & in)
			:
			 blockid(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 numblocks(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 subblockid(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 numsubblocks(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 FI(::libmaus2::fastx::FastInterval::deserialise(in)),
			 SUBFI(::libmaus2::fastx::FastInterval::deserialise(in)),
			 sublo(::libmaus2::fastx::FastInterval::deserialiseVector(in)),
			 inputfiles(::libmaus2::util::StringSerialisation::deserialiseStringVector(in)),
			 numedges(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 serverhostname(::libmaus2::util::StringSerialisation::deserialiseString(in)),
			 serverport(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 isfastq(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 mintracelength(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 minscore(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 scorewindowsize(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 windowminscore(::libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			 maxindelfrac(::libmaus2::util::GenericSerialisation::deserialise< std::istream, double >(in)),
			 maxsubstfrac(::libmaus2::util::GenericSerialisation::deserialise< std::istream, double >(in))
			{

			}

			void serialise(std::string const & filename)
			{
				libmaus2::aio::OutputStreamInstance ostr(filename);
				assert ( ostr );
				serialise(ostr);
				ostr.flush();
				assert ( ostr );
			}

			void serialise(std::ostream & out)
			{
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,blockid);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,numblocks);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,subblockid);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,numsubblocks);
				::libmaus2::fastx::FastInterval::serialise(out,FI);
				::libmaus2::fastx::FastInterval::serialise(out,SUBFI);
				::libmaus2::fastx::FastInterval::serialiseVector(out,sublo);
				::libmaus2::util::StringSerialisation::serialiseStringVector(out,inputfiles);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,numedges);
				::libmaus2::util::StringSerialisation::serialiseString(out,serverhostname);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,serverport);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,isfastq);

				::libmaus2::util::NumberSerialisation::serialiseNumber(out,mintracelength);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,minscore);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,scorewindowsize);
				::libmaus2::util::NumberSerialisation::serialiseNumber(out,windowminscore);

				::libmaus2::util::GenericSerialisation::serialise(out,maxindelfrac);
				::libmaus2::util::GenericSerialisation::serialise(out,maxsubstfrac);
			}
		};
	}
}
#endif
