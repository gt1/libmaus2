/*
    libmaus2
    Copyright (C) 2009-2016 German Tischler
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
#if ! defined(LIBMAUS2_LZ_CONCATBLOCKINFLATE_HPP)
#define LIBMAUS2_LZ_CONCATBLOCKINFLATE_HPP

#include <libmaus2/lz/Inflate.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct ConcatBlockInflate
		{
			typedef ConcatBlockInflate this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			std::vector<std::string> const filenames;
			uint64_t fileptr;
			std::vector < uint64_t > sizevec;

			BlockInflate::unique_ptr_type BI;

			uint64_t n;

			template<typename filename_container_type>
			ConcatBlockInflate(filename_container_type const & rfilenames, uint64_t offset)
			: filenames(rfilenames.begin(),rfilenames.end()), fileptr(0),
			  sizevec(BlockInflate::computeSizeVector(filenames)), n(0)
			{
				for ( uint64_t i = 0; i < sizevec.size(); ++i )
				{
					n += sizevec[i];
				}

				while ( fileptr < sizevec.size() && offset >= sizevec[fileptr] )
				{
					offset -= sizevec[fileptr];
					fileptr++;
				}

				if ( fileptr < sizevec.size() )
				{
					assert ( offset < sizevec[fileptr] );
					BlockInflate::unique_ptr_type tBI(new BlockInflate(filenames[fileptr],offset));
					BI = UNIQUE_PTR_MOVE(tBI);
					// BI->ignore(offset);
				}
				else
				{
					#if 0
					std::cerr << "fileptr=" << fileptr << " offset=" << offset
						<< " sizevec.size()=" << sizevec.size()
						<< " filenames.size()=" << filenames.size()
						<< std::endl;
					#endif
				}
			}
			template<typename filename_container_type>
			static uint64_t size(filename_container_type const & rfilenames)
			{
				this_type CBI(rfilenames,0);
				return CBI.n;
			}

			template<typename filename_container_type>
			static std::vector<uint64_t> getSplitPoints(filename_container_type const & V, uint64_t const segments, uint64_t const numthreads)
			{
				std::vector<std::string> const VV(V.begin(),V.end());
				uint64_t const n = BlockInflate::computeSize(VV);
				std::vector < uint64_t > points(segments);

				/*
				 * compute the split points
				 */
				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
				#endif
				for ( int64_t i = 0; i < static_cast<int64_t>(segments); ++i )
				{
					uint64_t const scanstart = std::min ( i * ((n+segments-1)/segments), n );
					// std::cerr << "scanstart " << scanstart << std::endl;
					this_type CD(VV,scanstart);
					uint64_t offset = 0;
					while ( (scanstart+offset < n) && (CD.get() != 0) )
						offset++;
					points[i] = scanstart + offset;
					// std::cerr << "offset " << offset << std::endl;
				}

				/*
				 * check that positions lead to terminators
				 */
				#if defined(_OPENMP)
				#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
				#endif
				for ( int64_t i = 0; i < static_cast<int64_t>(segments); ++i )
					if ( points[i] != n )
					{
						this_type CD(VV,points[i]);
						int const val = CD.get();
						if ( val != 0 )
						{
							::libmaus2::exception::LibMausException se;
							se.getStream() << "Failed to set correct split point at " << points[i]
								<< " expected 0 got " << val
								<< " point " << i << " points.size()=" << points.size()
								<< std::endl;
							se.finish();
							throw se;
						}
					}

				/*
				 * add end of file
				 */
				points.push_back(n);

				return points;
			}

			uint64_t read(uint8_t * p, uint64_t n)
			{
				uint64_t red = 0;

				while ( n && BI )
				{
					uint64_t const lred = BI->read ( p, n );
					p += lred;
					n -= lred;
					red += lred;

					if ( ! lred )
					{
						fileptr++;
						BI.reset();
						if ( fileptr < filenames.size() )
						{
							BlockInflate::unique_ptr_type tBI(new BlockInflate(filenames[fileptr]));
							BI = UNIQUE_PTR_MOVE(tBI);
						}
					}
				}

				return red;
			}

			int get()
			{
				uint8_t c;
				uint64_t const red = read(&c,1);
				// std::cerr << "c=" << c << " red=" << red << std::endl;
				if ( red )
					return c;
				else
					return -1;
			}
		};
	}
}
#endif
