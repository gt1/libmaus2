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

#if defined(LIBMAUS_HAVE_SEQAN)
#include <libmaus2/consensus/ScoringMatrix.hpp>
#include <seqan/align.h>
#include <seqan/graph_msa.h>
#include <seqan/score.h>

extern "C"
{
	int libmaus2_consensus_ConsensusComputationBase_computeConsensusA_wrapperC(void const * thisptr, void * R, void const * V, int const verbose, void * ostr)
	{
		return libmaus2_consensus_ConsensusComputationBase_computeConsensusA_wrapper(thisptr,R,V,verbose,ostr);
	}

	int libmaus2_consensus_ConsensusComputationBase_computeConsensusQ_wrapperC(void const * thisptr, void * R, void const * V, int const verbose, void * ostr)
	{
		return libmaus2_consensus_ConsensusComputationBase_computeConsensusQ_wrapper(thisptr,R,V,verbose,ostr);
	}
	int libmaus2_consensus_ConsensusComputationBase_construct_wrapperC(void ** vP)
	{
		return libmaus2_consensus_ConsensusComputationBase_construct_wrapper(vP);
	}
	int libmaus2_consensus_ConsensusComputationBase_destruct_wrapperC(void * vP)
	{
		return libmaus2_consensus_ConsensusComputationBase_destruct_wrapper(vP);
	}
}

int libmaus2_consensus_ConsensusComputationBase_construct_wrapper(void ** vP)
{
	try
	{
		*vP = 0;
		libmaus2::consensus::ConsensusComputationBase * base = new libmaus2::consensus::ConsensusComputationBase;
		*vP = base;
		// std::cerr << "Set pointer to " << base << " == " << (*vP) << std::endl;
		return 0;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		*vP = 0;
		return -1;
	}
}

int libmaus2_consensus_ConsensusComputationBase_destruct_wrapper(void * vP)
{
	try
	{
		libmaus2::consensus::ConsensusComputationBase * P = reinterpret_cast<libmaus2::consensus::ConsensusComputationBase *>(vP);
		// std::cerr << "Deleting ptr " << P << std::endl;
		delete P;
		return 0;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return -1;
	}
}

int libmaus2_consensus_ConsensusComputationBase_computeConsensusA_wrapper(void const * thisptr, void * R, void const * V, int const verbose, void * ostr)
{
	try
	{
		libmaus2::consensus::ConsensusComputationBase::computeConsensusA(thisptr,R,V,verbose,ostr);
		return 0;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return -1;
	}
}

int libmaus2_consensus_ConsensusComputationBase_computeConsensusQ_wrapper(void const * thisptr, void * R, void const * V, int const verbose, void * ostr)
{
	try
	{
		libmaus2::consensus::ConsensusComputationBase::computeConsensusQ(thisptr,R,V,verbose,ostr);
		return 0;
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return -1;
	}
}


void libmaus2::consensus::ConsensusComputationBase::computeConsensusA(void const * thisptr, void * R, void const * V, int const verbose, void * ostr)
{
	libmaus2::consensus::ConsensusComputationBase const * obj = reinterpret_cast<libmaus2::consensus::ConsensusComputationBase const *>(thisptr);	
	//std::cerr << "A using ptr " << thisptr << " == " << obj << std::endl;
	obj->computeConsensusA(R,V,verbose,ostr);
}
void libmaus2::consensus::ConsensusComputationBase::computeConsensusQ(void const * thisptr, void * R, void const * V, int const verbose, void * ostr)
{
	libmaus2::consensus::ConsensusComputationBase const * obj = reinterpret_cast<libmaus2::consensus::ConsensusComputationBase const *>(thisptr);
	// std::cerr << "Q using ptr " << thisptr << " == " << obj << std::endl;
	obj->computeConsensusQ(R,V,verbose,ostr);
}


static inline char mapChar(char const c)
{
	switch ( c )
	{
		case 'A': case 'a': return 0;
		case 'C': case 'c': return 1;
		case 'G': case 'g': return 2;
		case 'T': case 't': return 3;
		case 'N': case 'n': return 4;
		default:  return 5;
	}
}
static inline char remapChar(char const c)
{
	static char const * m = "ACGTN-";
	return m[static_cast<int>(c)];
}

libmaus2::consensus::ConsensusComputationBase::ConsensusComputationBase() : mscoring(new ::libmaus2::consensus::ScoringMatrix)
{
	// mscoring.showMatrix();
}


static uint64_t getMaxRowLength(seqan::Align< seqan::String<seqan::Dna5> > const & align)
{
	int64_t maxviewpos = -1;
	// iterate over rows
	for ( uint64_t r = 0; r < length(rows(align)); ++r )
	{
		// get row
		seqan::Gaps< seqan::String<seqan::Dna5>, seqan::ArrayGaps > const & arow = rows(align)[r];

		for ( uint64_t i = 0; i < length(source(arow)); ++i )
		{
			int64_t const viewpos = toViewPosition(arow, i) ;
			maxviewpos = std::max(maxviewpos,viewpos);
		}
	}
	
	return static_cast<uint64_t>(maxviewpos+1);
}

void libmaus2::consensus::ConsensusComputationBase::computeConsensusA(void * vR, void const * vV, int const verbose, void * vostr) const
{
	std::string * R = reinterpret_cast<std::string *>(vR);
	std::vector< std::string > const * V = reinterpret_cast< std::vector< std::string > const * >(vV);
	std::ostream * ostr = reinterpret_cast<std::ostream *>(vostr);

	ScoringMatrix::base_type const & scoring = dynamic_cast<ScoringMatrix const &>(*mscoring);

	seqan::Align< seqan::String<seqan::Dna5> > align;
	seqan::resize(rows(align), V->size());

	for ( uint64_t i = 0; i < V->size(); ++i )
		seqan::assignSource(row(align, i),(*V)[i]);	

	// compute multiple alignment
	seqan::globalMsaAlignment(align, scoring);
	
	if ( ostr && (verbose & 1) )
		(*ostr) << align;

	// get maximum row length
	uint64_t const rowlen = getMaxRowLength(align);

	// compute frequencies per column	
	std::vector < int > cons(6*rowlen);

	// iterate over rows
	for ( uint64_t r = 0; r < length(rows(align)); ++r )
	{
		seqan::Gaps< seqan::String<seqan::Dna5>, seqan::ArrayGaps > const & arow = rows(align)[r];

		if ( ostr && (verbose & 2) )
			(*ostr) << arow << "\t";
		
		uint64_t exp = 0;
		for ( uint64_t i = 0; i < length(source(arow)); ++i )
		{
			uint64_t const viewpos = toViewPosition(arow, i) ;
			
			while ( exp < viewpos )
			{
				if ( exp < rowlen )
					cons[(exp * 6) + mapChar('-')] ++;
				exp++;
			}
		
			if ( ostr && (verbose & 2) )
				(*ostr) << viewpos
					<< "(" << source(arow)[i] << ")"
					<< (((i+1)<length(source(arow)))?",":"")
				;

			if ( exp < rowlen )
			{
				cons[(exp*6) + mapChar(source(arow)[i])]++;
			}
			else
			{
				std::cerr << "[[!!OVERFLOW!!]]" << std::endl;
			}
			
			exp++;
		}
		
		while ( exp < rowlen )
		{		
			cons[(exp * 6) + mapChar('-')] ++;
			exp++;
		}
		
		if ( ostr && (verbose & 2) )
			(*ostr) << std::endl;
	}
	
	// compute pre consensus by majority vote
	// (use lex. smallest character with maximum freq per column)
	std::string preconsensus(rowlen,'-');
	uint64_t conslen = 0;
	for ( uint64_t c = 0; c < rowlen; ++c )
	{
		int sum = 0;
		int maxval = -1;
		int maxchar = -1;
		
		for ( uint64_t i = 0; i < 6; ++i )
		{
			if ( cons[c*6+i] )
			{
				if ( ostr && (verbose & 4) )
					(*ostr) << remapChar(i) << ":" << cons[c*6+i] << ";";
				
				if ( cons[c*6+i] > maxval )
				{
					maxval = cons[c*6+i];
					maxchar = i;
				}
			}
				
			sum += cons[c*6+i];
		}
		
		assert ( sum == static_cast<int>(length(rows(align))) );
		
		preconsensus[c] = remapChar(maxchar);
		
		if ( maxchar >= 0 && maxchar <= 4 )
			conslen++;
		
		if ( ostr && (verbose & 4) )
			(*ostr) << std::endl;
	}
	
	// build final consensus from pre consensus by dropping - columns
	std::string consensus(conslen,'\0');
	
	for ( uint64_t i = 0, j = 0; i < preconsensus.size(); ++i )
		if ( preconsensus[i] != '-' )
			consensus[j++] = preconsensus[i];

	*R = consensus;
}

void libmaus2::consensus::ConsensusComputationBase::computeConsensusQ(
	void * vR, void const * vV, int const verbose, void * vostr) const
{
	std::string * R = reinterpret_cast<std::string *>(vR);
	std::vector< ::libmaus2::fastx::FastQElement > const * V = reinterpret_cast< std::vector< ::libmaus2::fastx::FastQElement > const * >(vV);
	std::ostream * ostr = reinterpret_cast<std::ostream *>(vostr);

	ScoringMatrix::base_type const & scoring = dynamic_cast<ScoringMatrix const &>(*mscoring);

	seqan::Align< seqan::String<seqan::Dna5> > align;
	seqan::resize(rows(align), V->size());

	for ( uint64_t i = 0; i < V->size(); ++i )
		seqan::assignSource(row(align, i),(*V)[i].query);

	// compute multiple alignment
	seqan::globalMsaAlignment(align, scoring);
	
	if ( ostr && (verbose & 1) )
		(*ostr) << align;

	// get maximum row length of multiple alignment
	uint64_t const rowlen = getMaxRowLength(align);

	// compute frequencies per column	
	std::vector < int    > cons    (6*rowlen);
	// accumulated (non normalized) "probabilities" of base correctness
	std::vector < double > consprob(6*rowlen);

	// iterate over rows
	double const nprob = ::libmaus2::fastx::Phred::probCorrect(40);
	for ( uint64_t r = 0; r < length(rows(align)); ++r )
	{
		seqan::Gaps< seqan::String<seqan::Dna5>, seqan::ArrayGaps > const & arow = rows(align)[r];
		::libmaus2::fastx::FastQElement const & E = (*V)[r];
		assert ( length(source(arow)) == E.query.size() );

		if ( ostr && (verbose & 2) )
			(*ostr) << arow << "\t";
		
		uint64_t exp = 0;
		for ( uint64_t i = 0; i < length(source(arow)); ++i )
		{
			// position of character in multiple alignment
			uint64_t const viewpos = toViewPosition(arow, i) ;
			
			while ( exp < viewpos )
			{
				if ( exp < rowlen )
				{
					cons    [(exp * 6) + mapChar('-')] ++;
					consprob[(exp * 6) + mapChar('-')] += nprob;
				}
				exp++;
			}
		
			if ( ostr && (verbose & 2) )
				(*ostr) << viewpos
					<< "(" << source(arow)[i] << ")"
					<< (((i+1)<length(source(arow)))?",":"")
				;

			if ( exp < rowlen )
			{
				assert ( source(arow)[i] == E.query[i] );
				cons    [(exp*6) + mapChar(source(arow)[i])]++;
				double const corprob = E.getBaseCorrectnessProbability(i);
				consprob[(exp*6) + mapChar(source(arow)[i])] += corprob;
			}
			else
			{
				std::cerr << "[[!!OVERFLOW!!]]" << std::endl;
			}
			
			exp++;
		}
		
		while ( exp < rowlen )
		{		
			cons    [(exp * 6) + mapChar('-')] ++;
			consprob[(exp * 6) + mapChar('-')] += nprob;
			exp++;
		}
		
		if ( ostr && (verbose & 2) )
			(*ostr) << std::endl;
	}
	
	// compute pre consensus by majority vote
	// (use lex. smallest character with maximum acc. correctness probability)
	std::string preconsensus(rowlen,'-');
	uint64_t conslen = 0;
	for ( uint64_t c = 0; c < rowlen; ++c )
	{
		int sum = 0;
		double maxval = -1;
		int maxchar = -1;

		for ( uint64_t i = 0; i < 6; ++i )
			sum += cons[c*6+i];

		for ( uint64_t i = 0; i < 6; ++i )
			consprob [ c*6 + i ] /= sum;

		assert ( sum == static_cast<int>(length(rows(align))) );
			
		for ( uint64_t i = 0; i < 6; ++i )
		{
			if ( cons[c*6+i] )
			{
				if ( ostr && (verbose & 4) )
					(*ostr) << remapChar(i) << ":" << cons[c*6+i] << ":" << consprob[c*6+i] << ";";
				
				if ( consprob[c*6+i] > maxval )
				{
					maxval = consprob[c*6+i];
					maxchar = i;
				}
			}		
		}
				
		preconsensus[c] = remapChar(maxchar);
		
		if ( maxchar >= 0 && maxchar <= 4 )
			conslen++;
		
		if ( ostr && (verbose & 4) )
			(*ostr) << std::endl;
	}
	
	// build final consensus from pre consensus by dropping - columns
	std::string consensus(conslen,'\0');
	
	for ( uint64_t i = 0, j = 0; i < preconsensus.size(); ++i )
		if ( preconsensus[i] != '-' )
			consensus[j++] = preconsensus[i];

	*R = consensus;
}
#endif
