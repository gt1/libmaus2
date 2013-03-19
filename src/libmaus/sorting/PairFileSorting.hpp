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
#if ! defined(LIBMAUS_SORTING_PAIRFILESORTING_HPP)
#define LIBMAUS_SORTING_PAIRFILESORTING_HPP

#include <map>
#include <vector>
#include <string>
#include <queue>

#include <libmaus/aio/FileFragment.hpp>
#include <libmaus/aio/ReorderConcatGenericInput.hpp>
#include <libmaus/aio/SynchronousGenericInput.hpp>
#include <libmaus/aio/SynchronousGenericOutput.hpp>

#include <libmaus/util/GetFileSize.hpp>

namespace libmaus
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

			static void sortPairFile(
				std::vector<std::string> const & filenames, 
				std::string const & tmpfilename,
				std::string const & outfilename,
				bool second,
				uint64_t const bufsize = 256*1024*1024
			)
			{
				::std::vector < ::libmaus::aio::FileFragment > frags;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					uint64_t const len = ::libmaus::util::GetFileSize::getFileSize(filenames[i])/sizeof(uint64_t);
					::libmaus::aio::FileFragment const frag(filenames[i],0,len);
					frags.push_back(frag);
				}
				
				assert ( bufsize );
				uint64_t const elnum = (bufsize + 2*sizeof(uint64_t)-1)/(2*sizeof(uint64_t));
				::libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > A(elnum,false);
				::libmaus::aio::ReorderConcatGenericInput<uint64_t> SGI(frags,16*1024);
				::libmaus::aio::SynchronousGenericOutput<uint64_t> SGO(tmpfilename,16*1024);
				uint64_t fullblocks = 0;
				uint64_t lastblock = 0;
				uint64_t numblocks = 0;
					
				while ( SGI.peek() >= 0 )
				{
					std::pair<uint64_t,uint64_t> * P = A.begin();
					uint64_t w = 0, v = 0;

					while ( (P != A.end()) && SGI.getNext(w) )
					{
						bool const ok = SGI.getNext(v);
						assert ( ok );
						*(P++) = std::pair<uint64_t,uint64_t>(w,v);			
					}
					
					if ( second )
						std::sort(A.begin(),P,SecondComp<uint64_t,uint64_t>());
					else
						std::sort(A.begin(),P,FirstComp<uint64_t,uint64_t>());
					
					for ( ptrdiff_t i = 0; i < P-A.begin(); ++i )
					{
						SGO.put(A[i].first);
						SGO.put(A[i].second);
					}
					
					if ( P == A.end() )
						fullblocks++;
					else
						lastblock = P-A.begin();

					numblocks++;
				}
				
				SGO.flush();
				
				if ( numblocks )
				{
					typedef Triple<uint64_t,uint64_t,uint64_t> triple_type;
					::libmaus::aio::SynchronousGenericOutput<uint64_t> SGOfinal(outfilename,16*1024);
					::libmaus::autoarray::AutoArray < ::libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type > in(numblocks);
					
					if ( second )
					{
						std::priority_queue < triple_type, std::vector<triple_type>, TripleSecondComparator<uint64_t,uint64_t,uint64_t> > Q;
							
						for ( uint64_t i = 0; i < numblocks; ++i )
						{
							in[i] = UNIQUE_PTR_MOVE(
								::libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type(
									new ::libmaus::aio::SynchronousGenericInput<uint64_t>(tmpfilename,16*1024,2*i*elnum,
										(i+1==numblocks) ?  (lastblock?(2*lastblock):(2*elnum)) : (2*elnum) ) ) );
							
							int64_t const a = in[i]->get();
							int64_t const b = in[i]->get();
							assert ( a >= 0 );
							assert ( b >= 0 );
							
							Q . push ( triple_type(a,b,i) );
						}
						
					
						while ( Q.size() )
						{
							SGOfinal.put(Q.top().first);
							SGOfinal.put(Q.top().second);
							uint64_t const id = Q.top().third;
							
							Q.pop();
							
							uint64_t a = 0;
							if ( in[id]->getNext(a) )
							{
								int64_t const b = in[id]->get();
								assert ( b >= 0 );				
								Q.push(triple_type(a,b,id));
							}
						}
					}
					else
					{
						std::priority_queue < triple_type, std::vector<triple_type>, TripleFirstComparator<uint64_t,uint64_t,uint64_t> > Q;
							
						for ( uint64_t i = 0; i < numblocks; ++i )
						{
							in[i] = UNIQUE_PTR_MOVE(
								::libmaus::aio::SynchronousGenericInput<uint64_t>::unique_ptr_type(
									new ::libmaus::aio::SynchronousGenericInput<uint64_t>(tmpfilename,16*1024,2*i*elnum,
										(i+1==numblocks) ?  (lastblock?(2*lastblock):(2*elnum)) : (2*elnum) ) ) );
							
							int64_t const a = in[i]->get();
							int64_t const b = in[i]->get();
							assert ( a >= 0 );
							assert ( b >= 0 );
							
							Q . push ( triple_type(a,b,i) );
						}
						
					
						while ( Q.size() )
						{
							SGOfinal.put(Q.top().first);
							SGOfinal.put(Q.top().second);
							uint64_t const id = Q.top().third;
							
							Q.pop();
							
							uint64_t a = 0;
							if ( in[id]->getNext(a) )
							{
								int64_t const b = in[id]->get();
								assert ( b >= 0 );				
								Q.push(triple_type(a,b,id));
							}
						}
					
					}
					
					SGOfinal.flush();
				}

				if ( numblocks )
				{
					::libmaus::aio::SynchronousGenericInput<uint64_t> SGIcheck(outfilename,16*1024);
					std::pair<uint64_t,uint64_t> prev(0,0);
					uint64_t a = 0;
				
					while ( SGIcheck.getNext(a) )
					{
						int64_t const b = SGIcheck.get();
						assert ( b >= 0 );
					
						// std::cerr << a << "," << b << std::endl;
							
						prev = std::pair<uint64_t,uint64_t>(a,b);
					}
				}
			}
		};
	}
}
#endif
