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
#if ! defined(LIBMAUS2_BAMBAM_BAMINDEX_HPP)
#define LIBMAUS2_BAMBAM_BAMINDEX_HPP

#include <libmaus2/bambam/BamIndexRef.hpp>
#include <libmaus2/util/PushBuffer.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/util/OutputFileNameTools.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/bambam/BamAlignmentReg2Bin.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamIndex
		{
			typedef BamIndex this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			int min_shift;
			int depth;

			libmaus2::autoarray::AutoArray<libmaus2::bambam::BamIndexRef> refs;

			template<typename stream_type, typename value_type, unsigned int length>
			static value_type getLEInteger(stream_type & stream)
			{
				value_type v = 0;

				for ( uint64_t i = 0; i < length; ++i )
					if ( stream.peek() == stream_type::traits_type::eof() )
					{
						libmaus2::exception::LibMausException ex;
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
				)
				{
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "[E] Failed to read magic (first four bytes) in BamIndex::init." << std::endl;
					ex.finish();
					throw ex;
				}

				bool const isbai = (magic[0] == 'B') && (magic[1] == 'A') && (magic[2] == 'I') && (magic[3] == '\1');
				bool const iscsi = (magic[0] == 'C') && (magic[1] == 'S') && (magic[2] == 'I') && (magic[3] == '\1');
				bool const isknown = (isbai || iscsi);

				if ( !isknown )
				{
					libmaus2::exception::LibMausException ex;
					ex.getStream() << "[E] BamIndex::init: unknown file format (neither bai nor csi)" << std::endl;
					ex.finish();
					throw ex;
				}

				if ( isbai )
				{
					min_shift = 14;
					depth = 5;

					uint32_t const numref = getLEInteger<stream_type,uint32_t,4>(stream);

					refs = libmaus2::autoarray::AutoArray<libmaus2::bambam::BamIndexRef>(numref);

					for ( uint64_t i = 0; i < numref; ++i )
					{
						uint32_t const distbins = getLEInteger<stream_type,uint32_t,4>(stream);

						#if 0
						std::cerr << "chr " << i << " distbins " << distbins << std::endl;
						#endif

						if ( distbins )
						{
							refs[i].bin = libmaus2::autoarray::AutoArray<libmaus2::bambam::BamIndexBin>(distbins,false);

							libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > pi(distbins,false);
							libmaus2::autoarray::AutoArray<libmaus2::bambam::BamIndexBin> prebins(distbins,false);

							for ( uint64_t j = 0; j < distbins; ++j )
							{
								uint32_t const bin = getLEInteger<stream_type,uint32_t,4>(stream);
								uint32_t const chunks = getLEInteger<stream_type,uint32_t,4>(stream);

								// std::cerr << "chr " << i << " bin " << bin << " chunks " << chunks << std::endl;

								prebins[j].bin = bin;
								prebins[j].chunks = libmaus2::autoarray::AutoArray<libmaus2::bambam::BamIndexBin::Chunk>(chunks,false);

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
							refs[i].lin.intervals = libmaus2::autoarray::AutoArray<uint64_t>(lins,false);

							for ( uint64_t j = 0; j < lins; ++j )
								refs[i].lin.intervals[j] = getLEInteger<stream_type,uint64_t,8>(stream);
						}
					}
				}
				else // if ( iscsi )
				{
					assert ( iscsi );

					min_shift = getLEInteger<stream_type,uint32_t,4>(stream);
					depth = getLEInteger<stream_type,uint32_t,4>(stream);

					uint64_t const l_aux = getLEInteger<stream_type,uint32_t,4>(stream);
					for ( uint64_t i = 0; i < l_aux; ++i )
					{
						int const c = stream.get();
						if ( c < 0 )
						{
							libmaus2::exception::LibMausException ex;
							ex.getStream() << "[E] BamIndex::init: found EOF/error while reading aux data" << std::endl;
							ex.finish();
							throw ex;
						}
					}

					uint32_t const numref = getLEInteger<stream_type,uint32_t,4>(stream);

					refs = libmaus2::autoarray::AutoArray<libmaus2::bambam::BamIndexRef>(numref);

					for ( uint64_t i = 0; i < numref; ++i )
					{
						// number of distinct bins
						uint32_t const distbins = getLEInteger<stream_type,uint32_t,4>(stream);

						#if 0
						std::cerr << "chr " << i << " distbins " << distbins << std::endl;
						#endif

						if ( distbins )
						{
							refs[i].bin = libmaus2::autoarray::AutoArray<libmaus2::bambam::BamIndexBin>(distbins,false);

							libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > pi(distbins,false);
							libmaus2::autoarray::AutoArray<libmaus2::bambam::BamIndexBin> prebins(distbins,false);

							for ( uint64_t j = 0; j < distbins; ++j )
							{
								// bin id
								uint32_t const bin = getLEInteger<stream_type,uint32_t,4>(stream);
								// virtual file offset of the first overlapping record
								/* uint64_t const loffset = */ getLEInteger<stream_type,uint64_t,8>(stream);
								// number of chunks
								uint32_t const chunks = getLEInteger<stream_type,uint32_t,4>(stream);

								// std::cerr << "chr " << i << " bin " << bin << " chunks " << chunks << std::endl;

								prebins[j].bin = bin;
								prebins[j].chunks = libmaus2::autoarray::AutoArray<libmaus2::bambam::BamIndexBin::Chunk>(chunks,false);

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
					}

				}
			}

			public:
			BamIndex() {}
			BamIndex(std::istream & in) { init(in); }
			BamIndex(std::string const & fn)
			{

				std::string const clipped = libmaus2::util::OutputFileNameTools::clipOff(fn,".bam");

				if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(clipped + ".csi") )
				{
					libmaus2::aio::InputStream::unique_ptr_type Pin(
						libmaus2::aio::InputStreamFactoryContainer::constructUnique(clipped + ".csi")
					);
					libmaus2::aio::InputStream & CIS = *Pin;
					init(CIS);
				}
				else if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(fn + ".csi") )
				{
					libmaus2::aio::InputStream::unique_ptr_type Pin(
						libmaus2::aio::InputStreamFactoryContainer::constructUnique(fn + ".csi")
					);
					libmaus2::aio::InputStream & CIS = *Pin;
					init(CIS);
				}
				else if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(clipped + ".bai") )
				{
					libmaus2::aio::InputStream::unique_ptr_type Pin(
						libmaus2::aio::InputStreamFactoryContainer::constructUnique(clipped + ".bai")
					);
					libmaus2::aio::InputStream & CIS = *Pin;
					init(CIS);
				}
				else if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(fn + ".bai") )
				{
					libmaus2::aio::InputStream::unique_ptr_type Pin(
						libmaus2::aio::InputStreamFactoryContainer::constructUnique(fn + ".bai")
					);
					libmaus2::aio::InputStream & CIS = *Pin;
					init(CIS);
				}
				else if ( libmaus2::aio::InputStreamFactoryContainer::tryOpen(fn) )
				{
					libmaus2::aio::InputStream::unique_ptr_type Pin(
						libmaus2::aio::InputStreamFactoryContainer::constructUnique(fn)
					);
					libmaus2::aio::InputStream & CIS = *Pin;
					init(CIS);
				}
				else
				{
					libmaus2::exception::LibMausException se;
					se.getStream() << "BamIndex::BamIndex(std::string const &): Cannot find index " << fn << std::endl;
					se.finish();
					throw se;
				}
			}

			// get bin i for ref-seq ref
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
				libmaus2::autoarray::AutoArray<BamIndexBin const *> & bins
			) const
			{
				libmaus2::autoarray::AutoArray<int> Abins;
				uint64_t const bino = libmaus2::bambam::BamAlignmentReg2Bin::reg2bins(beg,end,Abins,min_shift,depth);

				libmaus2::util::PushBuffer<BamIndexBin const *> PB;
				PB.A = bins;

				for ( uint64_t i = 0; i < bino; ++i )
				{
					BamIndexBin const * p = getBin(refid,Abins[i]);

					if ( p )
						PB.push(p);
				}


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
				libmaus2::autoarray::AutoArray<BamIndexBin const *> bins;
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

			libmaus2::autoarray::AutoArray<libmaus2::bambam::BamIndexRef> const & getRefs() const
			{
				return refs;
			}
		};
	}
}
#endif
