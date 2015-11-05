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
#if ! defined(LIBMAUS2_BAMBAM_CHROMOSOMEVECTORMERGE_HPP)
#define LIBMAUS2_BAMBAM_CHROMOSOMEVECTORMERGE_HPP

#include <libmaus2/bambam/Chromosome.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct ChromosomeVectorMerge
		{
			typedef ChromosomeVectorMerge this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			struct ChromosomeIndexComparator : public libmaus2::bambam::StrCmpNum
			{
				std::vector < std::vector<Chromosome> const * > const & V;
				bool const compareNonSNLN;

				ChromosomeIndexComparator(std::vector < std::vector<Chromosome> const * > const & rV, bool const rcompareNonSNLN = false)
				: V(rV), compareNonSNLN(rcompareNonSNLN) {}

				bool operator()(Chromosome const & CA, Chromosome const & CB) const
				{
					std::pair<char const *, char const *> namea = CA.getName();
					std::pair<char const *, char const *> nameb = CB.getName();
					int const namecmpres = strcmpnum(
						namea.first,
						namea.second,
						nameb.first,
						nameb.second
					);

					if ( namecmpres )
					{
						return namecmpres < 0;
					}
					else if ( CA.getLength() != CB.getLength() )
						return CA.getLength() < CB.getLength();
					else if ( compareNonSNLN )
					{
						// bool const res = libmaus2::util::StringMapCompare::compare(CA.M,CB.M);
						bool const res = libmaus2::util::StringMapCompare::compareSortedStringPairVectors(
							CA.getSortedKeyValuePairs(),
							CB.getSortedKeyValuePairs());
						#if 0
						std::cerr << "comparing for " << CA.name << "," << CA.len << std::endl;
						std::cerr << CA.createLine() << std::endl;
						std::cerr << CB.createLine() << std::endl;
						std::cerr << "CA.M.size()=" << CA.M.size() << std::endl;
						std::cerr << "CB.M.size()=" << CB.M.size() << std::endl;
						std::cerr << "result " << res << std::endl;
						#endif
						return res;
					}
					else
					{
						return false;
					}
				}

				bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
				{
					return (*this)(V[A.first]->at(A.second),V[B.first]->at(B.second));
				}

				bool equal(Chromosome const & CA, Chromosome const & CB) const
				{
					if ( (*this)(CA,CB) )
					{
						// std::cerr << "A<B" << std::endl;
						return false;
					}
					else if ( (*this)(CB,CA) )
					{
						// std::cerr << "B<A" << std::endl;
						return false;
					}
					else
						return true;
				}

				bool equal(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
				{
					return equal(V[A.first]->at(A.second),V[B.first]->at(B.second));
				}
			};

			std::vector < Chromosome > chromosomes;
			std::vector < std::vector< uint64_t > > chromosomesmap;
			bool topological;

			ChromosomeVectorMerge(std::vector < std::vector<Chromosome> const * > const & V, bool const compareNonSNLN = false)
			: chromosomes(), chromosomesmap(), topological(false)
			{
				ChromosomeIndexComparator comp(V,compareNonSNLN);

				/* first check whether all vectors contain the same reference sequences */
				bool samesize = true;
				for ( uint64_t i = 1; samesize && i < V.size(); ++i )
					if ( V[i]->size() != V[0]->size() )
						samesize = false;
				bool samevecs = samesize;
				for ( uint64_t i = 1; samevecs && i < V.size(); ++i )
					for ( uint64_t j = 0; samevecs && j < V[0]->size(); ++j )
						if ( ! comp.equal(V[0]->at(j),V[i]->at(j)) )
							samevecs = false;

				// simple case, all equal
				if ( samevecs )
				{
					if ( V.size() )
						chromosomes = *V[0];
					std::vector<uint64_t> cmap;
					for ( uint64_t i = 0; i < chromosomes.size(); ++i )
						cmap.push_back(i);
					for ( uint64_t i = 0; i < V.size(); ++i )
						chromosomesmap.push_back(cmap);

					topological = true;
				}
				// otherwise try to perform topological sorting
				else
				{
					// find node ids
					std::set < Chromosome, ChromosomeIndexComparator > cset(comp);
					for ( uint64_t i = 0; i < V.size(); ++i )
						for ( uint64_t j = 0; j < V[i]->size(); ++j )
							cset.insert(V[i]->at(j));
					std::vector<Chromosome> cvec(cset.begin(),cset.end());
					typedef std::vector<Chromosome>::const_iterator it;

					// compute edges based on node ids
					std::map < uint64_t, std::set<uint64_t> > edges;
					std::map < uint64_t, std::set<uint64_t> > redges;
					for ( uint64_t i = 0; i < V.size(); ++i )
						for ( uint64_t j = 1; j < V[i]->size(); ++j )
						{
							Chromosome const & prev = V[i]->at(j-1);
							Chromosome const & next = V[i]->at(j);

							std::pair<it,it> const prevp = std::equal_range(cvec.begin(),cvec.end(),prev,comp);
							assert ( prevp.first != prevp.second );
							uint64_t const prevr = prevp.first - cvec.begin();

							std::pair<it,it> const nextp = std::equal_range(cvec.begin(),cvec.end(),next,comp);
							assert ( nextp.first != nextp.second );
							uint64_t const nextr = nextp.first - cvec.begin();

							edges [ prevr ] .insert ( nextr );
							redges [ nextr ] .insert (prevr );
						}

					// find start vertices with no incoming edges
					std::vector<uint64_t> S;
					for ( uint64_t i = 0; i < cvec.size(); ++i )
						if ( redges.find(i) == redges.end() )
							S.push_back(i);

					// try to construct topological sorting
					std::vector<uint64_t> topsort;
					while ( S.size() )
					{
						uint64_t const n = S.back();
						S.pop_back();

						topsort.push_back(n);

						if ( edges.find(n) != edges.end() )
						{
							// edge targets
							std::set<uint64_t> const & T = edges.find(n)->second;

							std::vector < uint64_t > ltgt;
							for ( std::set<uint64_t>::const_iterator ita = T.begin(); ita != T.end(); ++ita )
								ltgt.push_back(*ita);

							for ( uint64_t i = 0; i < ltgt.size(); ++i )
							{
								// edge target
								uint64_t const m = ltgt[i];

								// erase edge from graph
								// forward
								edges [ n ] . erase ( edges[n].find(m) );
								// reverse
								redges [ m ] . erase ( redges[m].find(n) );

								// push m if it has no more incoming edges
								if ( redges.find(m)->second.size() == 0 )
									S.push_back(m);

								// remove empty lists
								if ( edges.find(n)->second.size() == 0 )
									edges.erase(edges.find(n));
								if ( redges.find(m)->second.size() == 0 )
									redges.erase(redges.find(m));
							}
						}
					}

					// we found a topological sorting iff all graph edges have been erased
					topological = (edges.size() == 0);

					// if we have no valid sort order than choose lexicographical order
					if ( ! topological )
					{
						topsort.resize(0);
						for ( uint64_t i = 0; i < cvec.size(); ++i )
							topsort.push_back(i);
					}

					assert ( topsort.size() == cvec.size() );

					// compute inverse
					std::vector<uint64_t> rtopsort(topsort.size());
					for ( uint64_t i = 0; i < topsort.size(); ++i )
						rtopsort[topsort[i]] = i;

					for ( uint64_t i = 0; i < topsort.size(); ++i )
						chromosomes.push_back(cvec[topsort[i]]);

					for ( uint64_t i = 0; i < V.size(); ++i )
					{
						std::vector < uint64_t > lchromosomemap;
						for ( uint64_t j = 0; j < V[i]->size(); ++j )
						{
							Chromosome const & inchr = V[i]->at(j);

							std::pair<it,it> const inchrp = std::equal_range(cvec.begin(),cvec.end(),inchr,comp);
							assert ( inchrp.first != inchrp.second );
							// index on cvec
							uint64_t const inchrr = inchrp.first - cvec.begin();
							// index on chromosomes
							uint64_t const outchrr = rtopsort[inchrr];

							assert ( comp.equal(inchr,cvec[inchrr]) );
							assert ( comp.equal(inchr,chromosomes[outchrr]) );

							lchromosomemap.push_back(outchrr);
						}

						chromosomesmap.push_back(lchromosomemap);
					}

					// make sure reference sequence ids are unique
					std::set < std::string > seenids;
					for ( uint64_t i = 0; i < chromosomes.size(); ++i )
					{
						Chromosome & chr = chromosomes[i];

						while ( seenids.find(chr.getNameString()) != seenids.end() )
							chr.setName(chr.getNameString() + '\'');

						// std::cerr << chr.createLine() << std::endl;

						seenids.insert(chr.getNameString());
					}
				}
			}
		};
	}
}
#endif
