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
#if ! defined(LIBMAUS2_BAMBAM_BAMNUMERICALINDEXGENERATOR_HPP)
#define LIBMAUS2_BAMBAM_BAMNUMERICALINDEXGENERATOR_HPP

#include <libmaus2/aio/Buffer.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/bambam/EncoderBase.hpp>
#include <libmaus2/bambam/BamAlignment.hpp>
#include <libmaus2/bambam/BamHeader.hpp>
#include <libmaus2/util/GetObject.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/lz/BgzfInflate.hpp>
#include <libmaus2/lz/BgzfInflateParallel.hpp>
#include <libmaus2/bambam/BamNumericalIndexBase.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamNumericalIndexGenerator : public libmaus2::bambam::BamNumericalIndexBase
		{
			typedef BamNumericalIndexGenerator this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			/* parser state types */
			enum parsestate { state_reading_blocklen,  state_post_skip };

			libmaus2::aio::OutputStreamInstance::unique_ptr_type Pindexstr;
			std::ostream & indexstr;

			uint64_t const indexmod;

			::libmaus2::bambam::BamHeader header;

			bool haveheader;

			uint64_t cacct; // accumulated compressed bytes we have read from file
			std::pair<uint64_t,uint64_t> rinfo;

			::libmaus2::bambam::BamHeaderParserState bamheaderparsestate;

			parsestate state;

			unsigned int blocklenred;
			uint32_t blocklen;

			uint64_t alcnt;
			uint64_t alcmpstart;
			uint64_t alstart;

			::libmaus2::bambam::BamAlignment algn;
			uint8_t * copyptr;

			unsigned int const verbose;
			bool const validate;
			bool const debug;

			bool blocksetup;
			bool flushed;

			void setup()
			{
				libmaus2::util::NumberSerialisation::serialiseNumber(indexstr,0);
				libmaus2::util::NumberSerialisation::serialiseNumber(indexstr,indexmod);
			}

			public:
			BamNumericalIndexGenerator(
				std::ostream & rindexstr,
				uint64_t const rindexmod,
				unsigned int const rverbose = 0,
				bool const rvalidate = true, bool const rdebug = false
			)
			: indexstr(rindexstr), indexmod(rindexmod), header(), haveheader(false),
			  cacct(0), rinfo(), bamheaderparsestate(), state(state_reading_blocklen), blocklenred(0),
			  blocklen(0), alcnt(0), alcmpstart(0), alstart(0), algn(),
			  copyptr(0), verbose(rverbose), validate(rvalidate), debug(rdebug),
			  blocksetup(false), flushed(false)
			{
				setup();
			}

			BamNumericalIndexGenerator(
				std::string const & fn,
				uint64_t const rindexmod,
				unsigned int const rverbose = 0,
				bool const rvalidate = true, bool const rdebug = false
			)
			: Pindexstr(new libmaus2::aio::OutputStreamInstance(fn)), indexstr(*Pindexstr), indexmod(rindexmod), header(), haveheader(false),
			  cacct(0), rinfo(), bamheaderparsestate(), state(state_reading_blocklen), blocklenred(0),
			  blocklen(0), alcnt(0), alcmpstart(0), alstart(0), algn(),
			  copyptr(0), verbose(rverbose), validate(rvalidate), debug(rdebug),
			  blocksetup(false), flushed(false)
			{
				setup();
			}

			void flush()
			{
				if ( ! flushed )
				{
					indexstr.flush();
					indexstr.seekp(0,std::ios::beg);
					libmaus2::util::NumberSerialisation::serialiseNumber(indexstr,alcnt);
					indexstr.flush();
					Pindexstr.reset();

					flushed = true;
				}
			}

			~BamNumericalIndexGenerator()
			{
				flush();
			}

			//! add block
			void addBlock(uint8_t const * Bbegin, uint64_t const compsize, uint64_t const uncompsize)
			{
				// std::cerr << "addBlock(" << compsize << "," << uncompsize << ")" << std::endl;

				uint8_t const * pa = Bbegin; // buffer current pointer
				uint8_t const * pc = pa + uncompsize; // buffer end pointer

				rinfo.first = compsize;
				rinfo.second = uncompsize;

				cacct += rinfo.first; // accumulator for compressed data bytes read

				if ( (! haveheader) && (pa != pc) )
				{
					::libmaus2::util::GetObject<uint8_t const *> G(Bbegin);
					std::pair<bool,uint64_t> const P = bamheaderparsestate.parseHeader(G,uncompsize);

					// header complete?
					if ( P.first )
					{
						header.init(bamheaderparsestate);
						haveheader = true;
						pa = Bbegin + P.second;
						pc = Bbegin + uncompsize;

						// std::cerr << "header complete, " << (pa != pc) << std::endl;

						if ( pa != pc )
						{
							// start of compressed block
							alcmpstart = cacct - rinfo.first;
							// start of alignment in uncompressed block
							alstart = pa - Bbegin;

							blocksetup = true;
						}
					}
				}

				if ( (haveheader) && (pa != pc) )
				{
					if ( ! blocksetup )
					{
						// start of compressed block
						alcmpstart = cacct - rinfo.first;
						// start of alignment in uncompressed block
						alstart = pa - Bbegin;

						// std::cerr << "blocksetup (2) " << alcmpstart << " " << alstart << std::endl;

						blocksetup = true;
					}

					while ( pa != pc )
					{
						switch ( state )
						{
							/* read length of next alignment block */
							case state_reading_blocklen:
								/* if this is a little endian machine allowing unaligned access */
								#if defined(LIBMAUS2_HAVE_i386)
								if ( (!blocklenred) && ((pc-pa) >= static_cast<ptrdiff_t>(sizeof(uint32_t))) )
								{
									blocklen = *(reinterpret_cast<uint32_t const *>(pa));
									blocklenred = sizeof(uint32_t);
									pa += sizeof(uint32_t);

									state = state_post_skip;

									if ( algn.D.size() < blocklen )
										algn.D = ::libmaus2::bambam::BamAlignment::D_array_type(blocklen,false);
									algn.blocksize = blocklen;
									copyptr = algn.D.begin();
								}
								else
								#endif
								{
									while ( pa != pc && blocklenred < sizeof(uint32_t) )
										blocklen |= static_cast<uint32_t>(*(pa++)) << ((blocklenred++)*8);

									if ( blocklenred == sizeof(uint32_t) )
									{
										state = state_post_skip;

										if ( algn.D.size() < blocklen )
											algn.D = ::libmaus2::bambam::BamAlignment::D_array_type(blocklen,false);
										algn.blocksize = blocklen;
										copyptr = algn.D.begin();
									}
								}

								break;
							/* skip data after part we modify */
							case state_post_skip:
							{
								uint32_t const skip = std::min(pc-pa,static_cast<ptrdiff_t>(blocklen));
								std::copy(pa,pa+skip,copyptr);
								copyptr += skip;
								pa += skip;
								blocklen -= skip;

								if ( ! blocklen )
								{
									if ( validate )
										algn.checkAlignment();

									// finished an alignment, set up for next one
									state = state_reading_blocklen;

									blocklenred = 0;
									blocklen = 0;

									// start of next alignment:
									//   - if pc != pa, then next alignment starts in this block
									//   - if pc == pa, then next alignment starts in next block
									// block start
									uint64_t const nextalcmpstart =
										(pc != pa) ? (cacct - rinfo.first) : cacct;
									// offset in block
									uint64_t const nextalstart =
										(pc != pa) ? (pa - Bbegin) : 0;

									if ( alcnt % indexmod == 0 )
									{
										libmaus2::util::NumberSerialisation::serialiseNumber(indexstr,alcmpstart);
										libmaus2::util::NumberSerialisation::serialiseNumber(indexstr,alstart);
									}

									alcnt++;
									alcmpstart = nextalcmpstart;
									alstart = nextalstart;

									if ( verbose && (alcnt % (1024*1024) == 0) )
										std::cerr << "[V] " << alcnt/(1024*1024) << std::endl;
								}
								break;
							}
						}
					}
				}
			}

			template<typename stream_type>
			static uint64_t indexStream(stream_type & infl, std::string const indexfn, uint64_t const mod, bool const verbose)
			{
				libmaus2::lz::BgzfInflateInfo info;
				uint64_t const b = libmaus2::lz::BgzfInflate<std::istream>::getBgzfMaxBlockSize();
				libmaus2::autoarray::AutoArray<char> B(b,false);
				libmaus2::bambam::BamNumericalIndexGenerator index(indexfn,mod);

				uint64_t csize = 0;
				for ( uint64_t c = 0; ! (info = infl.readAndInfo(B.begin(),B.size())).streameof; ++c )
				{
					csize += info.compressed;
					index.addBlock(reinterpret_cast<uint8_t const *>(B.begin()),info.compressed,info.uncompressed);
					if ( verbose && (c % (16*1024) == 0) )
						std::cerr << "(" << c << ")";
				}
				if ( verbose )
					std::cerr << std::endl;

				return csize;
			}

			static void indexFile(std::string const & fn, std::string const & indexfn, uint64_t const mod, uint64_t const numthreads, bool const verbose = false)
			{
				libmaus2::aio::InputStreamInstance in(fn);
				uint64_t csize;

				std::string const tmpindexfn = indexfn + ".tmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(tmpindexfn);

				if ( numthreads <= 1 )
				{
					libmaus2::lz::BgzfInflate<std::istream> infl(in);
					csize = indexStream(infl,tmpindexfn,mod,verbose);
				}
				else
				{
					libmaus2::lz::BgzfInflateParallel infl(in,numthreads);
					csize = indexStream(infl,tmpindexfn,mod,verbose);
				}

				libmaus2::aio::OutputStreamFactoryContainer::rename(tmpindexfn,indexfn);

				bool const ok = (csize == libmaus2::util::GetFileSize::getFileSize(fn));

				if ( ! ok )
				{
					std::cerr << "[W] warning csize=" << csize << " != filesize=" << libmaus2::util::GetFileSize::getFileSize(fn) << std::endl;
				}
			}

			static void indexFileCheck(std::string const & fn, std::string const & indexfn, uint64_t const mod, uint64_t const numthreads, bool const verbose = false)
			{
				if (
					!libmaus2::util::GetFileSize::fileExists(indexfn)
					||
					libmaus2::util::GetFileSize::isOlder(indexfn,fn)
				)
					indexFile(fn,indexfn,mod,numthreads,verbose);
			}

			static std::string indexFileCheck(std::string const & fn, uint64_t const mod, uint64_t const numthreads, bool const verbose = false)
			{
				std::string const indexfn = getIndexName(fn);

				if (
					!libmaus2::util::GetFileSize::fileExists(indexfn)
					||
					libmaus2::util::GetFileSize::isOlder(indexfn,fn)
				)
					indexFile(fn,indexfn,mod,numthreads,verbose);

				return indexfn;
			}
		};
	}
}
#endif
