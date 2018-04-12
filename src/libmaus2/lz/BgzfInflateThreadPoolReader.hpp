/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if ! defined(LIBMAUS2_LZ_BGZFINFLATETHREADPOOLREADER_HPP)
#define LIBMAUS2_LZ_BGZFINFLATETHREADPOOLREADER_HPP

#include <libmaus2/lz/BgzfInputContext.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateThreadPoolReader
		{
			typedef BgzfInflateThreadPoolReader this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			std::istream & istr;
			libmaus2::parallel::SimpleThreadPool::unique_ptr_type PSTP;
			libmaus2::parallel::SimpleThreadPool & STP;

			libmaus2::lz::BgzfInputContext BIC;

			bool eof;
			libmaus2::lz::BgzfInflateReadPackageDispatcher::BgzfInflateReadPackageResult res;
			bool resvalid;
			char const * pc;
			char const * pe;
			uint64_t gc;
			std::ostream * copystr;

			BgzfInflateThreadPoolReader(std::istream & ristr, uint64_t const numthreads, std::ostream * rcopystr = 0)
			: istr(ristr), PSTP(new libmaus2::parallel::SimpleThreadPool(numthreads)), STP(*PSTP), BIC(istr,STP), eof(false), res(), resvalid(false), pc(0), pe(0), gc(0),
			  copystr(rcopystr)
			{
				BIC.init();
			}
			BgzfInflateThreadPoolReader(std::istream & ristr, libmaus2::parallel::SimpleThreadPool & rSTP, std::ostream * rcopystr = 0)
			: istr(ristr), PSTP(), STP(rSTP), BIC(istr,STP), eof(false), res(), resvalid(false), pc(0), pe(0), gc(0),
			  copystr(rcopystr)
			{
				BIC.init();
			}
			~BgzfInflateThreadPoolReader()
			{
				if ( PSTP )
				{
					STP.terminate();
					STP.join();
					PSTP.reset();
				}
			}

			bool loadBlock()
			{
				if ( resvalid )
				{
					BIC.returnBlock(res);
					resvalid = false;
				}

				if ( eof )
				{
					return false;
				}

				while ( !STP.isInPanicMode() )
				{
					if ( BIC.outq.timeddeque(res) )
					{
						resvalid = true;

						pc = res.ablock->begin();
						pe = res.ablock->begin() + res.ubytes;

						if ( copystr )
						{
							copystr->write(pc,res.ubytes);
							if ( ! *copystr )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "[E] BgzfInflateThreadPoolReader::loadBlock: copy stream failed" << std::endl;
								lme.finish();
								throw lme;
							}
						}

						eof = eof || res.eof;

						return true;
					}
				}

				assert ( STP.isInPanicMode() );

				#if 0
				while ( !BIC.idle() )
				{
					sleep(1);
				}
				#endif

				if ( PSTP )
				{
					STP.terminate();
					STP.join();
					PSTP.reset();
				}

				libmaus2::exception::LibMausException lme;
				lme.getStream() << "BgzfInflateThreadPoolReader::loadBlock(): thread pool is in panic mode" << std::endl;
				lme.finish();
				throw lme;
			}

			uint64_t read(char * p, uint64_t s)
			{
				uint64_t const r = readInternal(p,s);
				return r;
			}

			uint64_t readInternal(char * p, uint64_t s)
			{
				uint64_t r = 0;

				while ( s )
				{
					while ( pc == pe )
					{
						bool const ok = loadBlock();

						if ( ! ok )
						{
							gc = r;
							return r;
						}
					}

					uint64_t const av = pe-pc;
					uint64_t const tocopy = std::min(av,s);
					std::copy(
						pc,
						pc+tocopy,
						p
					);

					pc += tocopy;
					p += tocopy;
					r += tocopy;
					s -= tocopy;
				}

				gc = r;
				return r;
			}

			uint64_t gcount() const
			{
				return gc;
			}

			int get()
			{
				char c;

				uint64_t const n = read(&c,1);

				if ( !n )
					return -1;
				else
					return static_cast<int>(static_cast<unsigned char>(c));
			}
		};
	}
}
#endif
