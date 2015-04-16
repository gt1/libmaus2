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
#include <libmaus/util/PushBuffer.hpp>
#include <libmaus/util/GetFileSize.hpp>
#include <libmaus/util/OutputFileNameTools.hpp>

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
					
					#if 0
					std::cerr << "chr " << i << " distbins " << distbins << std::endl;
					#endif
					
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
			BamIndex(std::string const & fn)
			{
				
				std::string const clipped = libmaus::util::OutputFileNameTools::clipOff(fn,".bam");
				
				if ( libmaus::util::GetFileSize::fileExists(clipped + ".bai") )
				{
					libmaus::aio::CheckedInputStream CIS(clipped + ".bai");
					init(CIS);
				}
				else if ( libmaus::util::GetFileSize::fileExists(fn + ".bai") )
				{
					libmaus::aio::CheckedInputStream CIS(fn + ".bai");
					init(CIS);
				}
				else if ( libmaus::util::GetFileSize::fileExists(fn) )
				{
					libmaus::aio::CheckedInputStream CIS(fn);
					init(CIS);
				}
				else
				{
					libmaus::exception::LibMausException se;
					se.getStream() << "BamIndex::BamIndex(std::string const &): Cannot find index " << fn << std::endl;
					se.finish();
					throw se;
				}
			}
			
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
			
			/**
			 * return list of bins for interval [beg,end)
			 *
			 * @param beg start offset
			 * @param end end offset
			 * @param bins array for storing bins
			 * @return number of bins stored
			 **/
			static uint64_t reg2bins(uint64_t beg, uint64_t end, libmaus::autoarray::AutoArray<uint16_t> & bins)
			{
				libmaus::util::PushBuffer<uint16_t> PB;
				PB.A = bins;
				
				end -= 1;

				PB.push(0);

				for (uint64_t k = 1 + (beg>>26); k <= 1 + (end>>26); ++k) PB.push(k);
				for (uint64_t k = 9 + (beg>>23); k <= 9 + (end>>23); ++k) PB.push(k);
				for (uint64_t k = 73 + (beg>>20); k <= 73 + (end>>20); ++k) PB.push(k);
				for (uint64_t k = 585 + (beg>>17); k <= 585 + (end>>17); ++k) PB.push(k);
				for (uint64_t k = 4681 + (beg>>14); k <= 4681 + (end>>14); ++k) PB.push(k);
				
				bins = PB.A;
				
				return PB.f;
			}
			
			/**
			 * return list of bins for interval [beg,end)
			 *
			 * @param refid reference id
			 * @param beg start offset
			 * @param end end offset
			 * @param bins array for storing bins
			 * @return number of bins stored
			 **/
			uint64_t reg2bins(
				uint64_t refid,
				uint64_t beg, 
				uint64_t end, 
				libmaus::autoarray::AutoArray<BamIndexBin const *> & bins
			) const
			{
				libmaus::util::PushBuffer<BamIndexBin const *> PB;
				PB.A = bins;
				
				end -= 1;

				if ( getBin(refid,0) ) PB.push(getBin(refid,0));

				for (uint64_t k = 1 + (beg>>26); k <= 1 + (end>>26); ++k) if ( getBin(refid,k) ) PB.push(getBin(refid,k));
				for (uint64_t k = 9 + (beg>>23); k <= 9 + (end>>23); ++k) if ( getBin(refid,k) ) PB.push(getBin(refid,k));
				for (uint64_t k = 73 + (beg>>20); k <= 73 + (end>>20); ++k) if ( getBin(refid,k) ) PB.push(getBin(refid,k));
				for (uint64_t k = 585 + (beg>>17); k <= 585 + (end>>17); ++k) if ( getBin(refid,k) ) PB.push(getBin(refid,k));
				for (uint64_t k = 4681 + (beg>>14); k <= 4681 + (end>>14); ++k) if ( getBin(refid,k) ) PB.push(getBin(refid,k));
				
				bins = PB.A;
				
				return PB.f;
			
			}
			
			/**
			 * return chunks for interval [beg,end)
			 *
			 * @param refid reference id
			 * @param beg start offset
			 * @param end end offset
			 * @return list of chunks
			 **/
			std::vector < std::pair<uint64_t,uint64_t> > reg2chunks(
				uint64_t const refid,
				uint64_t const beg,
				uint64_t const end) const
			{
				// get set of matching bins
				libmaus::autoarray::AutoArray<BamIndexBin const *> bins;
				uint64_t const numbins = reg2bins(refid,beg,end,bins);
				std::vector < std::pair<uint64_t,uint64_t> > C;
				
				// extract chunks
				for ( uint64_t b = 0; b < numbins; ++b )
				{
					BamIndexBin const * bin = bins[b];
					
					for ( uint64_t i = 0; i < bin->chunks.size(); ++i )
						C.push_back(bin->chunks[i]);
				}
				
				// sort chunks
				std::sort(C.begin(),C.end());
				
				// merge chunks
				uint64_t low = 0;
				uint64_t out = 0;
								
				while ( low != C.size() )
				{
					uint64_t high = low+1;

					while ( high != C.size() && C[low].second >= C[high].first )
					{
						C[low].second = std::max(C[low].second,C[high].second);
						++high;
					}
										
					C[out++] = C[low];
					
					low = high;
				}
				
				C.resize(out);
				
				return C;
			}

			libmaus::autoarray::AutoArray<libmaus::bambam::BamIndexRef> const & getRefs() const
			{
				return refs;
			}
		};
	}
}
#endif
