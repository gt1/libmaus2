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

#if ! defined(INFLATE_HPP)
#define INFLATE_HPP

#include <libmaus2/types/types.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <zlib.h>
#include <cassert>

#include <libmaus2/timing/RealTimeClock.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct Inflate
		{
			static unsigned int const input_buffer_size = 64*1024;

			private:
			z_stream strm;

			public:
			typedef libmaus2::aio::InputStreamInstance istr_file_type;
			typedef ::libmaus2::util::unique_ptr<istr_file_type>::type istr_file_ptr_type;

			istr_file_ptr_type pin;
			std::istream & in;
			::libmaus2::autoarray::AutoArray<uint8_t> inbuf;
			::libmaus2::autoarray::AutoArray<uint8_t,::libmaus2::autoarray::alloc_type_c> outbuf;
			uint64_t outbuffill;
			uint8_t const * op;
			int ret;

			void zreset()
			{
				if ( (ret=inflateReset(&strm)) != Z_OK )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Inflate::zreset(): inflateReset failed";
					se.finish();
					throw se;
				}

				ret = Z_OK;
				outbuffill = 0;
				op = 0;
			}

			void init(int windowSizeLog)
			{
				memset(&strm,0,sizeof(z_stream));

				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				strm.avail_in = 0;
				strm.next_in = Z_NULL;

				if ( windowSizeLog )
					ret = inflateInit2(&strm,windowSizeLog);
				else
					ret = inflateInit(&strm);

				if (ret != Z_OK)
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "inflate failed in inflateInit";
					se.finish();
					throw se;
				}
			}

			Inflate(std::istream & rin, int windowSizeLog = 0)
			: in(rin), inbuf(input_buffer_size,false), outbuf(16*1024,false), outbuffill(0), op(0)
			{
				init(windowSizeLog);
			}
			Inflate(std::string const & filename, int windowSizeLog = 0)
			: pin(new istr_file_type(filename)),
			  in(*pin), inbuf(input_buffer_size,false), outbuf(16*1024,false), outbuffill(0), op(0)
			{
				init(windowSizeLog);
			}
			~Inflate()
			{
				inflateEnd(&strm);
			}

			int get()
			{
				while ( ! outbuffill )
				{
					bool const ok = fillBuffer();
					if ( ! ok )
						return -1;

					// assert ( outbuffill );
				}

				// assert ( outbuffill );
				// std::cerr << "outbuffill=" << outbuffill << std::endl;

				int const c = *(op++);
				outbuffill--;

				return c;
			}

			uint64_t read(char * buffer, uint64_t const n)
			{
				uint64_t red = 0;

				while ( red < n )
				{
					while ( ! outbuffill )
					{
						bool const ok = fillBuffer();
						if ( ! ok )
							return red;
					}

					uint64_t const tocopy = std::min(outbuffill,n-red);
					std::copy ( op, op + tocopy, buffer + red );
					op += tocopy;
					red += tocopy;
					outbuffill -= tocopy;
				}

				return red;
			}

			bool fillBuffer()
			{
				if ( ret == Z_STREAM_END )
					return false;

				while ( (ret == Z_OK) && (! outbuffill) )
				{
					/* read a block of data */
					in.read ( reinterpret_cast<char *>(inbuf.get()), inbuf.size() );
					uint64_t const read = in.gcount();
					if ( ! read )
					{
						// std::cerr << "Found EOF in fillBuffer()" << std::endl;
						strm.avail_in = 0;
						strm.next_in = 0;
						return false;
					}
					else
					{
						// std::cerr << "Got " << read << " in fillBuffer()" << std::endl;
					}

					strm.avail_in = read;
					strm.next_in = reinterpret_cast<Bytef *>(inbuf.get());

					while ( (strm.avail_in != 0) && (ret == Z_OK) )
					{
						if ( outbuffill == outbuf.size() )
							outbuf.resize(outbuf.size() + 16*1024);

						uint64_t const avail_out = outbuf.size() - outbuffill;
						strm.avail_out = avail_out;
						strm.next_out = reinterpret_cast<Bytef *>(outbuf.get() + outbuffill);
						ret = inflate(&strm, Z_NO_FLUSH);

						if ( ret == Z_OK || ret == Z_STREAM_END )
							outbuffill += (avail_out - strm.avail_out);
					}
				}

				if ( ret != Z_OK && ret != Z_STREAM_END )
				{
					::libmaus2::exception::LibMausException se;
					se.getStream() << "Inflate::fillBuffer(): zlib state clobbered after inflate(), state is ";

					switch ( ret )
					{
						case Z_NEED_DICT: se.getStream() << "Z_NEED_DICT"; break;
						case Z_ERRNO: se.getStream() << "Z_ERRNO"; break;
						case Z_STREAM_ERROR: se.getStream() << "Z_STREAM_ERROR"; break;
						case Z_DATA_ERROR: se.getStream() << "Z_DATA_ERROR"; break;
						case Z_MEM_ERROR: se.getStream() << "Z_MEM_ERROR"; break;
						case Z_BUF_ERROR: se.getStream() << "Z_BUF_ERROR"; break;
						case Z_VERSION_ERROR: se.getStream() << "Z_VERSION_ERROR"; break;
						default: se.getStream() << "Unknown error code"; break;
					}
					se.getStream() << " output size " << outbuffill << std::endl;

					se.finish();
					throw se;
				}

				op = outbuf.begin();

				return true;
			}

			std::pair<uint8_t const *, uint8_t const *> getRest() const
			{
				return std::pair<uint8_t const *, uint8_t const *>(
					strm.next_in,strm.next_in+strm.avail_in
				);
			}

			void ungetRest()
			{
				if ( in.eof() )
					in.clear();

				for ( uint64_t i = 0; i < strm.avail_in; ++i )
					in.putback(strm.next_in[strm.avail_in-i-1]);
			}
		};

		struct BlockInflate
		{
			typedef BlockInflate this_type;
			typedef ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef libmaus2::aio::InputStreamInstance istream_type;
			typedef ::libmaus2::util::unique_ptr<istream_type>::type istream_ptr_type;

			std::vector < std::pair<uint64_t,uint64_t> > index;
			uint64_t blockptr;
			uint64_t n;

			istream_ptr_type Pistr;
			istream_type & istr;

			::libmaus2::autoarray::AutoArray<uint8_t,::libmaus2::autoarray::alloc_type_c> B;
			uint8_t * pa;
			uint8_t * pc;
			uint8_t * pe;

			static std::vector < std::pair<uint64_t,uint64_t> > loadIndex(std::string const & filename)
			{
				libmaus2::aio::InputStreamInstance istr(filename);
				std::vector < std::pair<uint64_t,uint64_t> > index;

				istr.seekg(-8,std::ios::end);
				uint64_t const indexpos = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				// std::cerr << "index at position " << indexpos << std::endl;
				istr.seekg(indexpos,std::ios::beg);
				uint64_t const indexlen = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				// std::cerr << "indexlen " << indexlen << std::endl;
				for ( uint64_t i = 0; i < indexlen; ++i )
				{
					uint64_t const blockpos = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
					uint64_t const blocklen = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
					index.push_back(std::pair<uint64_t,uint64_t>(blockpos,blocklen));
					// std::cerr << "blockpos=" << blockpos << " blocklen=" << blocklen << std::endl;
				}
				istr.seekg(0,std::ios::beg);

				return index;
			}

			static uint64_t computeSize(std::vector < std::pair<uint64_t,uint64_t> > const & index)
			{
				uint64_t n = 0;
				for ( uint64_t i = 0; i < index.size(); ++i )
					n += index[i].second;
				return n;
			}

			static uint64_t computeSize(std::string const & filename)
			{
				return computeSize(loadIndex(filename));
			}

			static uint64_t computeSize(std::vector<std::string> const & filenames)
			{
				uint64_t n = 0;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					n += computeSize(filenames[i]);
				return n;
			}

			static std::vector<uint64_t> computeSizeVector(std::vector<std::string> const & filenames)
			{
				std::vector<uint64_t> sizes;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					sizes.push_back ( computeSize(filenames[i]) );
				return sizes;
			}

			BlockInflate(std::string const & filename, uint64_t pos = 0)
			: index(loadIndex(filename)), blockptr(0), n(computeSize(index)),
			  Pistr(new istream_type(filename)), istr(*Pistr),
			  B(), pa(0), pc(0), pe(0)
			{
				// std::cerr << "pos=" << pos << std::endl;
				while ( blockptr < index.size() && pos >= index[blockptr].second )
				{
					pos -= index[blockptr].second;
					blockptr++;
				}
				// std::cerr << "pos=" << pos << std::endl;
				decodeBlock();
				assert ( pc + pos <= pe );
				pc += pos;
			}

			uint64_t read(uint8_t * p, uint64_t n)
			{
				uint64_t red = 0;

				while ( n )
				{
					// std::cerr << "n=" << n << " red=" << red << std::endl;

					if ( pc == pe )
					{
						if ( ! decodeBlock() )
							return red;
					}

					uint64_t const toread = std::min(static_cast<uint64_t>(pe-pc),n);
					std::copy ( pc, pc + toread, p );
					pc += toread;
					p += toread;
					red += toread;
					n -= toread;
				}

				return red;
			}

			int get()
			{
				if ( pc == pe )
				{
					if ( ! decodeBlock() )
						return -1;
				}
				assert ( pc != pe );
				return *(pc++);
			}

			bool decodeBlock()
			{
				if ( blockptr >= index.size() )
					return false;

				istr.clear();
				istr.seekg ( index[blockptr].first, std::ios::beg );
				uint64_t const blocklen = ::libmaus2::util::NumberSerialisation::deserialiseNumber(istr);
				if ( B.size() < blocklen )
				{
					B.resize(blocklen);
				}
				pa = B.begin();
				pc = pa;
				pe = pa + blocklen;
				Inflate inflate(istr);
				inflate.read(reinterpret_cast<char *>(pa),blocklen);

				blockptr++;
				return true;
			}

			::libmaus2::autoarray::AutoArray<uint8_t> getReverse()
			{
				::libmaus2::autoarray::AutoArray<uint8_t> A(n,false);
				uint8_t * outptr = A.begin();

				for ( uint64_t i = 0; i < index.size(); ++i )
				{
					blockptr = index.size()-i-1;
					decodeBlock();
					std::reverse(pa,pe);
					while ( pc != pe )
						*(outptr++) = *(pc++);
				}

				assert ( outptr == A.end() );

				return A;
			}
		};

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
