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
#if ! defined(LIBMAUS2_SCORINGMATRIX_HPP)
#define LIBMAUS2_SCORINGMATRIX_HPP

#include <libmaus2/LibMausConfig.hpp>

#if defined(LIBMAUS2_HAVE_SEQAN)
#include <iostream>
#include <vector>
#include <cassert>
#include <seqan/align.h>
#include <seqan/graph_msa.h>
#include <seqan/score.h>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/util/Object.hpp>

namespace libmaus2
{
	namespace consensus
	{
		struct ScoringMatrix :
			public seqan::Score < int, seqan::ScoreMatrix<seqan::Dna5, seqan::Default> >,
			public ::libmaus2::util::Object< ::libmaus2::consensus::ScoringMatrix >
		{
			typedef ScoringMatrix this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef seqan::Dna5 TSequenceValue;
			typedef seqan::Score < int, seqan::ScoreMatrix<TSequenceValue, seqan::Default> > base_type;

			// gap penalties
			static int const gap_open_penalty = -4;
			static int const gap_extend_penalty = -3;

			// Print a scoring scheme matrix to stdout.
			void showMatrix(std::ostream & out) const;
			// constructor
			ScoringMatrix();
		};
	}
}
#endif
#endif
