/**
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
**/
#include <libmaus/consensus/ScoringMatrix.hpp>

#if defined(LIBMAUS_HAVE_SEQAN)

// Print a scoring scheme matrix to stdout.
void libmaus::consensus::ScoringMatrix::showMatrix(std::ostream & out) const
{
	// Print top row.
	for (unsigned i = 0; i < seqan::ValueSize<TSequenceValue>::VALUE; ++i)
		std::cout << "\t" << TSequenceValue(i);
	out << std::endl;
	
	// Print each row.
	for (unsigned i = 0; i < seqan::ValueSize<TSequenceValue>::VALUE; ++i) {
		std::cout << TSequenceValue(i);
		for (unsigned j = 0; j < seqan::ValueSize<TSequenceValue>::VALUE; ++j) {
			std::cout << "\t" << score(*this, TSequenceValue(i), TSequenceValue(j));
		}
		out << std::endl;
	}
}

libmaus::consensus::ScoringMatrix::ScoringMatrix() : base_type(gap_open_penalty,gap_extend_penalty)
{
	/* keeping base: good(1), changing base: bad(-1) */
	seqan::setScore(*this,'A','A',1);
	seqan::setScore(*this,'A','C',-1);
	seqan::setScore(*this,'A','G',-1);
	seqan::setScore(*this,'A','T',-1);

	seqan::setScore(*this,'C','A',-1);
	seqan::setScore(*this,'C','C', 1);
	seqan::setScore(*this,'C','G',-1);
	seqan::setScore(*this,'C','T',-1);

	seqan::setScore(*this,'G','A',-1);
	seqan::setScore(*this,'G','C',-1);
	seqan::setScore(*this,'G','G', 1);
	seqan::setScore(*this,'G','T',-1);

	seqan::setScore(*this,'T','A',-1);
	seqan::setScore(*this,'T','C',-1);
	seqan::setScore(*this,'T','G',-1);
	seqan::setScore(*this,'T','T', 1);

	/* changing N to base: ok(0) */
	seqan::setScore(*this,'N','A', 0);
	seqan::setScore(*this,'N','C', 0);
	seqan::setScore(*this,'N','G', 0);
	seqan::setScore(*this,'N','T', 0);

	/* changing base to N: bad(-1) */
	seqan::setScore(*this,'A','N', -1);
	seqan::setScore(*this,'C','N', -1);
	seqan::setScore(*this,'G','N', -1);
	seqan::setScore(*this,'T','N', -1);
	seqan::setScore(*this,'N','N',  0);
}
#endif
