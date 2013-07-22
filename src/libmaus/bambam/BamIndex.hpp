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
#if ! defined(LIBMAUS_BAMBAM_BAMINDEX_HPP)
#define LIBMAUS_BAMBAM_BAMINDEX_HPP

#include <libmaus/bambam/BamIndexRef.hpp>

namespace libmaus
{
	namespace bambam
	{
		struct BamIndex
		{
			typedef BamIndex this_type;
			typedef libmaus::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus::util::shared_ptr<this_type>::type shared_ptr_type;
			
			private:
			libmaus::autoarray::AutoArray<libmaus::bambam::BamIndexRef> refs;
			
			template<typename stream_type, typename value_type, unsigned int length>
			static value_type getLEInteger(stream_type & stream)
			{
				value_type v = 0;
				
				for ( uint64_t i = 0; i < length; ++i )
					if ( stream.peek() == stream_type::traits_type::eof() )
					{
						libmaus::exception::LibMausException ex;
						ex.getStream() << "Failed to little endian number of length " << length << " in BamIndex::getLEInteger." << std::endl;
						ex.finish();
						throw ex;		
					}
					else
					{
						v |= static_cast<value_type>(stream.get()) << (8*i);
					}
									
				return v;
			}
			
			template<typename stream_type>
			void init(stream_type & stream)
			{
				char magic[4];
				
				stream.read(&magic[0],sizeof(magic));
				
				if ( 
					! stream 
					||
					stream.gcount() != 4
					||
					magic[0] != 'B'
					||
					magic[1] != 'A'
					||
					magic[2] != 'I'
					||
					magic[3] != '\1'
				)
				{
					libmaus::exception::LibMausException ex;
					ex.getStream() << "Failed to read BAI magic BAI\\1." << std::endl;
					ex.finish();
					throw ex;
				}
				
				uint32_t const numref = getLEInteger<stream_type,uint32_t,4>(stream);
				
				refs = libmaus::autoarray::AutoArray<libmaus::bambam::BamIndexRef>(numref);
				
				for ( uint64_t i = 0; i < numref; ++i )
				{
					uint32_t const distbins = getLEInteger<stream_type,uint32_t,4>(stream);
					
					std::cerr << "chr " << i << " distbins " << distbins << std::endl;
					
					if ( distbins )
					{
						refs[i].bin = libmaus::autoarray::AutoArray<libmaus::bambam::BamIndexBin>(distbins,false);
						
						libmaus::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > pi(distbins,false);
						libmaus::autoarray::AutoArray<libmaus::bambam::BamIndexBin> prebins(distbins,false);
						
						for ( uint64_t j = 0; j < distbins; ++j )
						{
							uint32_t const bin = getLEInteger<stream_type,uint32_t,4>(stream);
							uint32_t const chunks = getLEInteger<stream_type,uint32_t,4>(stream);
							
							// std::cerr << "chr " << i << " bin " << bin << " chunks " << chunks << std::endl;
							
							prebins[j].bin = bin;
							prebins[j].chunks = libmaus::autoarray::AutoArray<libmaus::bambam::BamIndexBin::Chunk>(chunks,false);
							
							// read chunks
							for ( uint64_t k = 0; k < chunks; ++k )
							{
								prebins[j].chunks[k].first = getLEInteger<stream_type,uint64_t,8>(stream);
								prebins[j].chunks[k].second = getLEInteger<stream_type,uint64_t,8>(stream);
							}
							
							pi [ j ] = std::pair<uint64_t,uint64_t>(bin,j);
						}
						
						// sort by bin
						std::sort(pi.begin(),pi.end());
					
						// move
						for ( uint64_t j = 0; j < distbins; ++j )
							refs[i].bin[j] = prebins[pi[j].second];							
					}
					
					uint32_t const lins = getLEInteger<stream_type,uint32_t,4>(stream);
					
					if ( lins )
					{
						refs[i].lin.intervals = libmaus::autoarray::AutoArray<uint64_t>(lins,false);

						for ( uint64_t j = 0; j < lins; ++j )
							refs[i].lin.intervals[j] = getLEInteger<stream_type,uint64_t,8>(stream);
					}
				}
			}

			public:
			BamIndex() {}
			BamIndex(std::istream & in) { init(in); }
			BamIndex(libmaus::aio::CheckedInputStream & in) { init(in); }
			
			BamIndexBin const * getBin(uint64_t const ref, uint64_t const i) const
			{
				if ( ref >= refs.size() )
					return 0;
				
				BamIndexBin comp;
				comp.bin = i;
				BamIndexBin const * p = std::lower_bound(refs[ref].bin.begin(),refs[ref].bin.end(),comp);
				
				if ( p == refs[ref].bin.end() || p->bin != i )
				{
					return 0;
				}
				else
				{
					assert ( p->bin == i );
					return p;
				}
			}
		};
	}
}
#endif
