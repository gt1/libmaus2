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
#if ! defined(LIBMAUS_UTIL_LINEACCESSOR_HPP)
#define LIBMAUS_UTIL_LINEACCESSOR_HPP

#include <libmaus/rank/ERank222B.hpp>

namespace libmaus
{
	namespace util
	{
		struct LineAccessor
		{
			typedef LineAccessor this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			typedef libmaus::rank::ERank222B rank_type;
			typedef rank_type::unique_ptr_type rank_ptr_type;
			typedef rank_type::writer_type rank_writer_type;
			typedef rank_writer_type::data_type rank_data_type;
			
			// rank word bits (64)
			static uint64_t const rank_word_bits = 8*sizeof(rank_data_type);

			// true iff text ends on a new line
			bool const endsonnewline;
			// (number of super blocks, number of lines)
			std::pair<uint64_t,uint64_t> nlns;
			// number of super blocks
			uint64_t const numlines;
			// number of lines
			uint64_t const numsuperblocks;
			// super block start pointers
			libmaus::autoarray::AutoArray<uint64_t> S;
			// line start pointers in super blocks
			libmaus::autoarray::AutoArray<uint16_t> M;
			// bit vector marking lines starting a new super block
			libmaus::autoarray::AutoArray<rank_data_type> B;
			// rank dictionary on B
			rank_ptr_type R;
			
			// super block size (2^16)
			static const uint64_t superblocksize = (1ull << 8*(sizeof(uint16_t)));
			
			struct NullCallback
			{
				void operator()(uint64_t const) {}
				void super(uint64_t const) {}		
				void bit(bool const) {}		
				void flush() {}
			};
			
			template<typename iterator>
			struct PushCallback
			{
				iterator p;
				
				PushCallback() : p() {}
				PushCallback(PushCallback<iterator> const & o) : p(o.p) {}
				PushCallback(iterator rp) : p(rp) {}
				
				PushCallback<iterator> & operator=(PushCallback<iterator> const & o)
				{
					p = o.p;
					return *this;
				}
				
				void operator()(uint64_t const v)
				{
					*(p++) = v;
				}
			};
			
			struct SuperCallback
			{
				uint64_t * s;
				rank_writer_type w;
				
				SuperCallback()
				: s(0), w(0)
				{
				
				}
				SuperCallback(uint64_t * rs, uint64_t * rb)
				: s(rs), w(rb)
				{
				
				}
				SuperCallback(SuperCallback const & o)
				: s(o.s), w(o.w) 
				{
				
				}
				
				void super(uint64_t const v)
				{
					*(s++) = v;
				}
				
				void bit(bool const b)
				{
					w.writeBit(b);
				}
				
				void flush()
				{
					w.flush();
				}
			};

			template<typename iterator, typename super_callback_type, typename mini_callback_type>
			static std::pair<uint64_t,uint64_t> countSuperBlocks(
				iterator ita, iterator ite,
				super_callback_type supercallback,
				mini_callback_type minicallback
			)
			{
				uint64_t numsuper = 0;
				uint64_t numlines = 0;

				// start of previous line
				iterator itl = ita;
				iterator itsuper = ita;
				numsuper += 1; // start of first superblock
				supercallback.super(itsuper-ita);
				supercallback.bit(true);
				minicallback(itl-itsuper);

				for ( iterator itc = ita; itc != ite; ++itc )
				{
					if ( *itc == '\n' )
					{
						// start of next line
						iterator const itn = itc+1;
						
						bool const newsuper = static_cast<int64_t>(itn - itsuper) >= static_cast<int64_t>(superblocksize);
						
						if ( newsuper )
						{
							numsuper += 1;
							itsuper = itn;
							supercallback.super(itsuper-ita);
						}

						supercallback.bit(newsuper);
						
						assert ( static_cast<int64_t>(itn-itsuper) < static_cast<int64_t>(superblocksize) );
						minicallback(itn-itsuper);
						
						itl = itn;
						
						numlines += 1;
					}
				}
						
				if ( ita != ite && ite[-1] != '\n' )
				{
					iterator const itn = ite;
					bool const newsuper = static_cast<int64_t>(itn - itsuper) >= static_cast<int64_t>(superblocksize);

					if ( newsuper )
					{
						numsuper += 1;
						itsuper = itn;
						supercallback.super(itsuper-ita);
					} 
					
					supercallback.bit(newsuper);

					assert ( static_cast<int64_t>(itn-itsuper) < static_cast<int64_t>(superblocksize) );
					minicallback(itn-itsuper);

					itl = itn;

					numlines += 1;
				}
						
				supercallback.flush();
				
				return std::pair<uint64_t,uint64_t>(numsuper,numlines);
			}

			uint64_t lineStart(uint64_t const line) const
			{
				return S[ R->rank1(line)-1 ] + M[line];
			}
			
			public:
			template<typename iterator>
			LineAccessor(iterator ita, iterator ite)
			:
			  // check whether text ends with a new line character
			  endsonnewline( ite != ita && ite[-1] == '\n' ), 
			  // count number of super blocks and number of lines
			  nlns(countSuperBlocks(ita,ite,NullCallback(),NullCallback())), numlines(nlns.second), numsuperblocks(nlns.first), 
			  // allocate super block start array
			  S(numsuperblocks,false), 
			  // allocate line start array
			  M(numlines+1,false),
			  // super block marker bit array
			  B((M.size()+rank_word_bits-1)/rank_word_bits,false)
			{
				// fill arrays
				countSuperBlocks(
					ita,ite,
					SuperCallback(S.begin(),B.begin()),
					PushCallback<uint16_t *>(M.begin())
				);
				
				// set up rank dictionary
				rank_ptr_type tR(new rank_type(B.begin(),B.size()*rank_word_bits));
				R = UNIQUE_PTR_MOVE(tR);		
			}
			
			uint64_t size() const
			{
				return numlines;
			}
			
			std::pair<uint64_t,uint64_t> lineInterval(uint64_t const line) const
			{
				uint64_t const low = lineStart(line);
				uint64_t const high = lineStart(line+1) - ((line+1 < numlines || endsonnewline) ? 1 : 0);
				return std::pair<uint64_t,uint64_t>(low,high);
			}
			
			template<typename iterator>
			std::basic_string< typename ::std::iterator_traits<iterator>::value_type > getLine(iterator ita, uint64_t const line)
			{
				std::pair<uint64_t,uint64_t> const I = lineInterval(line);
				return std::basic_string< typename ::std::iterator_traits<iterator>::value_type >(ita+I.first,ita+I.second);
			}
		};
	}
}
#endif
