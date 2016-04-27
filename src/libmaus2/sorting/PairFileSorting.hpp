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
#if ! defined(LIBMAUS2_SORTING_PAIRFILESORTING_HPP)
#define LIBMAUS2_SORTING_PAIRFILESORTING_HPP

#include <map>
#include <vector>
#include <string>
#include <queue>

#if defined(_OPENMP)
#include <parallel/algorithm>
#endif

#include <libmaus2/aio/FileFragment.hpp>
#include <libmaus2/aio/ReorderConcatGenericInput.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/aio/ConcatInputStream.hpp>

#include <libmaus2/util/GetFileSize.hpp>

namespace libmaus2
{
	namespace sorting
	{
		struct PairFileSorting
		{
			template<typename first_type, typename second_type>
			struct FirstComp
			{
				bool operator()(std::pair<first_type,second_type> const & A, std::pair<first_type,second_type> const & B) const
				{
					if ( A.first != B.first )
						return A.first < B.first;
					else
						return A.second < B.second;
				}
			};

			template<typename first_type, typename second_type>
			struct SecondComp
			{
				bool operator()(std::pair<first_type,second_type> const & A, std::pair<first_type,second_type> const & B) const
				{
					if ( A.second != B.second )
						return A.second < B.second;
					else
						return A.first < B.first;
				}
			};

			template<typename first_type, typename second_type, typename third_type>
			struct Triple
			{
				first_type first;
				second_type second;
				third_type third;

				Triple() {}
				Triple(first_type rfirst, second_type rsecond, third_type rthird)
				: first(rfirst), second(rsecond), third(rthird)
				{

				}

				std::ostream & toStream(std::ostream & out) const
				{
					out
						<< "Triple("
						<< std::setw(16) << std::setfill('0') << std::hex << first << std::dec << std::setw(0) << ","
						<< std::setw(16) << std::setfill('0') << std::hex << second << std::dec << std::setw(0) << ","
						<< std::setw(16) << std::setfill('0') << std::hex << third << std::dec << std::setw(0) << ")";

					return out;
				}

				std::string toString() const
				{
					std::ostringstream ostr;
					toStream(ostr);
					return ostr.str();
				}
			};

			template<typename first_type, typename second_type, typename third_type>
			struct TripleFirstComparator
			{
				bool operator()(Triple<first_type,second_type,third_type> const & a, Triple<first_type,second_type,third_type> const & o) const
				{
					if ( a.first != o.first )
						return a.first > o.first;
					else if ( a.second != o.second )
						return a.second > o.second;
					else
						return a.third > o.third;
				}
			};

			template<typename first_type, typename second_type, typename third_type>
			struct TripleSecondComparator
			{
				bool operator()(Triple<first_type,second_type,third_type> const & a, Triple<first_type,second_type,third_type> const & o) const
				{
					if ( a.second != o.second )
						return a.second > o.second;
					else if ( a.first != o.first )
						return a.first > o.first;
					else
						return a.third > o.third;
				}
			};

			typedef Triple<uint64_t,uint64_t,uint64_t> triple_type;

			template<typename comparator_type, typename out_type, bool keepfirst, bool keepsecond>
			static void mergeTriplesTemplate(
				uint64_t const numblocks,
				std::string const & tmpfilename,
				uint64_t const elnum,
				uint64_t const lastblock,
				out_type & SGOfinal
			)
			{
				if ( numblocks )
				{
					::libmaus2::autoarray::AutoArray < ::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type > in(numblocks);

					std::priority_queue <
						triple_type,
						std::vector<triple_type>,
						comparator_type
					> Q;

					for ( uint64_t i = 0; i < numblocks; ++i )
					{
						uint64_t const rwords = (i+1==numblocks) ?
									(lastblock?(2*lastblock):(2*elnum)) : (2*elnum);

						::libmaus2::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type tini(
							new ::libmaus2::aio::SynchronousGenericInput<uint64_t>(
								tmpfilename,16*1024,2*i*elnum,
								rwords
							)
						);

						in[i] = UNIQUE_PTR_MOVE(tini);

						uint64_t a = 0, b = 0;
						bool const aok = in[i]->getNext(a);
						bool const bok = in[i]->getNext(b);
						assert ( aok );
						assert ( bok );

						triple_type triple(a,b,i);

						Q . push ( triple );
					}


					while ( Q.size() )
					{
						if ( keepfirst )
							SGOfinal.put(Q.top().first);
						if ( keepsecond )
							SGOfinal.put(Q.top().second);

						uint64_t const id = Q.top().third;

						// std::cerr << Q.top().toString() << std::endl;

						Q.pop();

						uint64_t a = 0;
						if ( in[id]->getNext(a) )
						{
							uint64_t b = 0;
							bool const bok = in[id]->getNext(b);
							assert ( bok );

							triple_type triple(a,b,id);

							Q.push(triple);
						}
					}

				}
			}

			template<typename comparator_type, typename out_type>
			static void mergeTriples(
				uint64_t const numblocks,
				std::string const & tmpfilename,
				uint64_t const elnum,
				uint64_t const lastblock,
				bool const keepfirst,
				bool const keepsecond,
				out_type & SGOfinal
			)
			{

				if ( keepfirst )
				{
					if ( keepsecond )
						mergeTriplesTemplate<comparator_type,out_type,true,true>(numblocks,tmpfilename,elnum,lastblock,SGOfinal);
					else
						mergeTriplesTemplate<comparator_type,out_type,true,false>(numblocks,tmpfilename,elnum,lastblock,SGOfinal);
				}
				else
				{
					if ( keepsecond )
						mergeTriplesTemplate<comparator_type,out_type,false,true>(numblocks,tmpfilename,elnum,lastblock,SGOfinal);
					else
						mergeTriplesTemplate<comparator_type,out_type,false,false>(numblocks,tmpfilename,elnum,lastblock,SGOfinal);
				}

				SGOfinal.flush();
			}

			template<typename out_type>
			static void sortPairFileTemplate(
				std::vector<std::string> const & filenames,
				std::string const & tmpfilename,
				bool const second,
				bool const keepfirst,
				bool const keepsecond,
				out_type & SGOfinal,
				uint64_t const bufsize = 256*1024*1024,
				bool const
				#if defined(_OPENMP)
					parallel
				#endif
					= false,
				bool const deleteinput = false
			)
			{
				// number of pairs in input
				uint64_t numpairs = 0;
				// size of a pair in bytes
				uint64_t const plen = 2*sizeof(uint64_t);
				// get length of input
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					// length of file in bytes
					uint64_t const len = ::libmaus2::util::GetFileSize::getFileSize(filenames[i]);
					// make sure it ends size is divisible by pair length
					assert ( (len % plen) == 0 );
					// add number of pairs
					numpairs += len / plen;
				}

				// number of pairs per block
				uint64_t const pairsperblock = (bufsize + plen -1)/plen;
				// number of blocks
				uint64_t const numblocks = (numpairs + pairsperblock - 1)/pairsperblock;
				// number of full blocks
				uint64_t const fullblocks = numpairs / pairsperblock;
				// number of elements in last block
				uint64_t const lastblock = (numblocks==fullblocks) ? pairsperblock : (numpairs - fullblocks*pairsperblock);

				libmaus2::aio::ConcatInputStream::unique_ptr_type CIN(new libmaus2::aio::ConcatInputStream(filenames));
				::libmaus2::aio::SynchronousGenericOutput<uint64_t>::unique_ptr_type SGO(
					new ::libmaus2::aio::SynchronousGenericOutput<uint64_t>(tmpfilename,16*1024)
				);
				::libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > A(pairsperblock,false);

				uint64_t w = 0;
				for ( uint64_t b = 0; b < numblocks; ++b )
				{
					uint64_t const blow = b * pairsperblock;
					uint64_t const bhigh = std::min(blow+pairsperblock,numpairs);
					std::pair<uint64_t,uint64_t> * P = A.begin();
					std::pair<uint64_t,uint64_t> * Pe = P + (bhigh-blow);

					CIN->clear();
					CIN->seekg(blow * plen);
					libmaus2::aio::SynchronousGenericInput<uint64_t> SGI(*CIN,16*1024);

					bool ok = true;
					while ( P != Pe )
					{
						ok = ok && SGI.getNext(P->first);
						ok = ok && SGI.getNext(P->second);
						P++;
					}
					assert ( ok );

					#if defined(_OPENMP)
					if ( parallel )
					{
						if ( second )
							__gnu_parallel::sort(A.begin(),P,SecondComp<uint64_t,uint64_t>());
						else
							__gnu_parallel::sort(A.begin(),P,FirstComp<uint64_t,uint64_t>());
					}
					else
					#endif
					{
						if ( second )
							std::sort(A.begin(),P,SecondComp<uint64_t,uint64_t>());
						else
							std::sort(A.begin(),P,FirstComp<uint64_t,uint64_t>());
					}

					P = A.begin();
					while ( P != Pe )
					{
						SGO->put(P->first);
						SGO->put(P->second);
						P++;
						w++;
					}
				}

				assert ( w == numpairs );

				SGO->flush();
				SGO.reset();
				CIN.reset();
				A.release();

				if ( deleteinput )
					for ( uint64_t i = 0; i < filenames.size(); ++i )
						libmaus2::aio::FileRemoval::removeFile(filenames[i]);

				if ( second )
					mergeTriples< TripleSecondComparator<uint64_t,uint64_t,uint64_t>, out_type >(
						numblocks,tmpfilename,pairsperblock,lastblock,
						keepfirst,keepsecond,SGOfinal);
				else
					mergeTriples< TripleFirstComparator<uint64_t,uint64_t,uint64_t>, out_type >(
						numblocks,tmpfilename,pairsperblock,lastblock,
						keepfirst,keepsecond,SGOfinal);
			}

			static void sortPairFile(
				std::vector<std::string> const & filenames,
				std::string const & tmpfilename,
				bool const second,
				bool const keepfirst,
				bool const keepsecond,
				std::string const & outfilename,
				uint64_t const bufsize = 256*1024*1024,
				bool const parallel = false,
				bool const deleteinput = false
			)
			{
				typedef ::libmaus2::aio::SynchronousGenericOutput<uint64_t> out_type;
				out_type SGOfinal(outfilename,16*1024);
				sortPairFileTemplate<out_type>(filenames,tmpfilename,second,keepfirst,keepsecond,SGOfinal,bufsize,parallel,deleteinput);
			}

			static void sortPairFile(
				std::vector<std::string> const & filenames,
				std::string const & tmpfilename,
				bool const second,
				bool const keepfirst,
				bool const keepsecond,
				std::ostream & outstream,
				uint64_t const bufsize = 256*1024*1024,
				bool const parallel = false,
				bool const deleteinput = false
			)
			{
				typedef ::libmaus2::aio::SynchronousGenericOutput<uint64_t> out_type;
				out_type SGOfinal(outstream,16*1024);
				sortPairFileTemplate<out_type>(filenames,tmpfilename,second,keepfirst,keepsecond,SGOfinal,bufsize,parallel,deleteinput);
			}
		};
	}
}
#endif
