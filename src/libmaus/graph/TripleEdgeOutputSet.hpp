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

#if ! defined(TRIPLEEDGEOUTPUTSET_HPP)
#define TRIPLEEDGEOUTPUTSET_HPP

#include <string>
#include <libmaus/graph/TripleEdgeBufferSet.hpp>
#include <libmaus/util/unique_ptr.hpp>

namespace libmaus
{
	namespace graph
	{
		/**
		 * output set for one hash interval
		 **/
		struct TripleEdgeOutputSet
		{
			typedef TripleEdgeOutputSet this_type;
			typedef ::libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
		
			::libmaus::util::TempFileNameGenerator & tmpgen;
			// std::string const filenameprefix;
			uint64_t const numreads;
			TripleEdgeBufferSet & tbs;

			static uint64_t const bufsize = 1024*1024;

			typedef TripleEdgeOutput output_type;
			typedef ::libmaus::util::unique_ptr<output_type>::type output_ptr_type;

			::libmaus::autoarray::AutoArray < output_ptr_type > outputs;

			static uint64_t byteSize(uint64_t numparts)
			{
				return 2*sizeof(uint64_t) + 2*sizeof(void*) + numparts * output_type::byteSize(bufsize);
			}

			std::string getNextFileName()
			{
				return tmpgen.getFileName();
			}

			TripleEdgeOutputSet(
				::libmaus::util::TempFileNameGenerator & rtmpgen, 
				uint64_t const rnumreads, 
				::libmaus::graph::TripleEdgeBufferSet & rtbs)
			: tmpgen(rtmpgen), numreads(rnumreads), tbs(rtbs), outputs(tbs.numparts) 
			{
				for ( uint64_t i = 0; i < tbs.numparts; ++i )
				{
					std::string const filename = getNextFileName();
					outputs[i] = UNIQUE_PTR_MOVE(output_ptr_type ( new output_type (filename, bufsize) ) );
				}
			}

			void flush()
			{
				for ( uint64_t i = 0; i < outputs.getN(); ++i )
				{
					output_type & output = *(outputs[i]);
					#if defined(NOUNIFY)
					output.flush();
					#else
					output.flushUnified();
					#endif
					std::string const filename = output.filename;
					outputs[i].reset();
					// std::cerr << "Registering " << filename << std::endl;
					tbs.registerFileName(i, filename);
				}
			}

			static inline uint64_t hash(uint64_t const a, uint64_t const numparts, uint64_t const numreads)
			{			
				return (a * numparts)/numreads;
			}

			inline uint64_t hash(uint64_t const a) const
			{			
				return hash(a,tbs.numparts,numreads);
			}
			
			static ::libmaus::autoarray::AutoArray<uint64_t> getHashStarts(uint64_t const numparts, uint64_t const numreads)
			{
				::libmaus::autoarray::AutoArray<uint64_t> HS(numparts+1,false);
				HS[numparts] = numreads;
				
				for ( int64_t i = static_cast<int64_t>(numreads)-1; i >= 0; --i )
					HS [ hash(i,numparts,numreads) ] = i;
					
				return HS;
			}

			void write ( ::libmaus::graph::TripleEdge const & triple )
			{
				// uint64_t const h = (i*i*tbs.numparts)/(numreads*numreads);
				uint64_t const h = hash(triple.a);
				assert ( h < tbs.numparts );
				output_type & output = *(outputs[h]);

				#if defined(NOUNIFY)
				if ( output.write ( triple ) )
				#else
				if ( output.writeUnified ( triple ) )
				#endif
				{
					output.flush();
					tbs.registerFileName(h, output.filename );
					outputs[h].reset();
					outputs[h] = UNIQUE_PTR_MOVE(output_ptr_type ( new output_type ( getNextFileName(), bufsize ) ) );
				}
			}
		};
	}
}
#endif
