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
#if ! defined(LIBMAUS2_HUFFMAN_SYMCOUNTENCODER_HPP)
#define LIBMAUS2_HUFFMAN_SYMCOUNTENCODER_HPP

#include <libmaus2/huffman/SymCountRun.hpp>
#include <libmaus2/huffman/IndexWriter.hpp>
#include <libmaus2/huffman/HuffmanEncoderFile.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/huffman/IndexLoader.hpp>
#include <libmaus2/math/bitsPerNum.hpp>
#include <libmaus2/huffman/PairAddSecond.hpp>
#include <libmaus2/huffman/IndexEntry.hpp>
#include <libmaus2/aio/FileRemoval.hpp>

#include <libmaus2/gamma/GammaEncoderBase.hpp>

namespace libmaus2
{
	namespace huffman
	{
		template<typename _bit_writer_type>
		struct SymCountEncoderBaseTemplate : public IndexWriter
		{
			typedef _bit_writer_type bit_writer_type;

			bit_writer_type & writer;

			::libmaus2::autoarray::AutoArray < SymCountRun > symcntruns;
			SymCountRun * const ra;
			SymCountRun *       rc;
			SymCountRun * const re;
			SymCountRun currun;

			std::vector < IndexEntry > index;

			bool indexwritten;

			SymCountEncoderBaseTemplate(bit_writer_type & rwriter, uint64_t const bufsize = 64*1024ull)
			: writer(rwriter),
			  symcntruns(bufsize),
			  ra(symcntruns.begin()),
			  rc(symcntruns.begin()),
			  re(symcntruns.end()),
			  currun(std::numeric_limits<int64_t>::min(),0,false,0),
			  indexwritten(false)
			{
			}
			~SymCountEncoderBaseTemplate()
			{
				flush();
			}

			void encode(SymCount const & SC)
			{
				if ( SC == currun )
				{
					currun.rlen += 1;
				}
				else
				{
					if ( currun.rlen )
					{
						*(rc++) = currun;
						if ( rc == re )
							implicitFlush();
					}

					currun = SC;
					currun.rlen = 1;
				}
			}

			void encodeRun(SymCountRun const & SC)
			{
				if ( static_cast<SymCount const &>(SC) == static_cast<SymCount const &>(currun) )
				{
					currun.rlen += SC.rlen;
				}
				else
				{
					if ( currun.rlen )
					{
						*(rc++) = currun;
						if ( rc == re )
							implicitFlush();
					}

					currun = SC;
				}
			}

			void flush()
			{
				assert ( rc != re );

				if ( currun.rlen )
				{
					*(rc++) = currun;
					currun.rlen = 0;
					currun.sym = std::numeric_limits<int64_t>::min();
				}

				implicitFlush();

				// std::cerr << "Index size " << index.size()*2*sizeof(uint64_t) << " for " << this << std::endl;;

				if ( ! indexwritten )
				{
					writer.flushBitStream();
					uint64_t const indexpos = writer.getPos();

					writeIndex(writer,index,indexpos);

					indexwritten = true;
				}

				writer.flush();
			}

			std::pair<uint64_t,uint64_t> computeSymRunsAndHist(
				::libmaus2::util::Histogram & symhist,
				::libmaus2::util::Histogram & symcnthist
			)
			{
				// compute symbol runs
				uint64_t numsyms = 0;
				for ( SymCountRun * t = ra; t != rc; ++t )
				{
					int64_t const refsym = t->sym;
					uint64_t const lnumsyms = t->rlen;

					symhist(refsym);
					symcnthist(lnumsyms);
					numsyms += lnumsyms;
				}
				assert ( numsyms );

				return std::pair<uint64_t,uint64_t>(rc-ra,numsyms);
			}

			std::pair<uint64_t,uint64_t> computeCntRunsAndHist(::libmaus2::util::Histogram & cntcnthist)
			{
				uint64_t numcntsyms = 0;

				// compute count runs
				for ( SymCountRun * t = ra; t != rc; ++t )
				{
					cntcnthist(t->rlen);
					numcntsyms += t->rlen;
				}

				return std::pair<uint64_t,uint64_t>(rc-ra,numcntsyms);
			}

			// encode sequence of run symbols
			void encodeSymSeq(::libmaus2::util::Histogram & symhist)
			{
				// get symbol/freq vectors
				std::vector < std::pair<uint64_t,uint64_t > > const symfreqs = symhist.getFreqSymVector();
				assert ( ! ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(symfreqs) );

				::libmaus2::huffman::CanonicalEncoder symenc(symhist.getByType<int64_t>());
				// serialise symbol encoder
				symenc.serialise(writer);

				// write symbol values
				for ( SymCountRun * t = ra; t != rc; ++t )
					symenc.encode(writer,t->sym);
			}

			void encodeSymCntSeq(::libmaus2::util::Histogram & symcnthist)
			{
				// get symbol/freq vectors
				std::vector < std::pair<uint64_t,uint64_t > > const symcntfreqs = symcnthist.getFreqSymVector();

				::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type escsymcntenc;
				::libmaus2::huffman::CanonicalEncoder::unique_ptr_type symcntenc;

				bool const symcntesc = ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(symcntfreqs);
				if ( symcntesc )
				{
					::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tescsymcntenc(new ::libmaus2::huffman::EscapeCanonicalEncoder(symcntfreqs));
					escsymcntenc = UNIQUE_PTR_MOVE(tescsymcntenc);
				}
				else
				{
					::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tsymcntenc(new ::libmaus2::huffman::CanonicalEncoder(symcnthist.getByType<int64_t>()));
					symcntenc = UNIQUE_PTR_MOVE(tsymcntenc);
				}

				// store whether we use escape code
				writer.writeBit(symcntesc);

				// serialise cnt encoder
				if ( escsymcntenc )
				{
					escsymcntenc->serialise(writer);
					for ( SymCountRun * t = ra; t != rc; ++t )
						escsymcntenc->encode(writer,t->rlen);
				}
				else
				{
					symcntenc->serialise(writer);
					for ( SymCountRun * t = ra; t != rc; ++t )
						symcntenc->encode(writer,t->rlen);
				}
			}

			void encodeCntSeq()
			{
				// write cnt values (gamma code)
				for ( SymCountRun * t = ra; t != rc; ++t )
				{
					//uint64_t const v = pi->first + 1;
					uint64_t const v = t->cnt + 1;
					unsigned int const nd = libmaus2::gamma::GammaEncoderBase<uint64_t>::getLengthCode(v);
					// nd zero bits
					writer.write(0,nd);
					// v using nd+1 bits
					writer.write(v,nd+1);
				}
			}

			void encodeCntCntSeq(::libmaus2::util::Histogram & cntcnthist)
			{
				// cnt run lengths
				std::vector < std::pair<uint64_t,uint64_t > > const cntcntfreqs = cntcnthist.getFreqSymVector();

				::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type esccntcntenc;
				::libmaus2::huffman::CanonicalEncoder::unique_ptr_type cntcntenc;

				bool const cntcntesc = ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(cntcntfreqs);
				if ( cntcntesc )
				{
					::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tesccntcntenc(new ::libmaus2::huffman::EscapeCanonicalEncoder(cntcntfreqs));
					esccntcntenc = UNIQUE_PTR_MOVE(tesccntcntenc);
				}
				else
				{
					::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tcntcntenc(new ::libmaus2::huffman::CanonicalEncoder(cntcnthist.getByType<int64_t>()));
					cntcntenc = UNIQUE_PTR_MOVE(tcntcntenc);
				}

				// store whether we use escape code
				writer.writeBit(cntcntesc);

				// serialise cntcnt encoder
				if ( esccntcntenc )
				{
					esccntcntenc->serialise(writer);
					for ( SymCountRun * t = ra; t != rc; ++t )
						esccntcntenc->encode(writer,t->rlen);
				}
				else
				{
					cntcntenc->serialise(writer);
					for ( SymCountRun * t = ra; t != rc; ++t )
						cntcntenc->encode(writer,t->rlen);
				}
			}

			void encodeSBits()
			{
				// store s bits
				for ( SymCountRun * rt = ra; rt != rc; ++rt )
					// write sbit
					writer.writeBit(rt->sbit);
			}

			void encodeSyms()
			{
				::libmaus2::util::Histogram symhist;
				::libmaus2::util::Histogram symcnthist;
				computeSymRunsAndHist(symhist,symcnthist);

				// store number of symbol runs
				writer.writeElias2(rc-ra);
				encodeSymSeq(symhist);
				encodeSymCntSeq(symcnthist);
			}

			void implicitFlush()
			{
				if ( rc != ra )
				{
					writer.flushBitStream();
					uint64_t const blockpos = writer.getPos();

					uint64_t numsyms = 0;
					for ( SymCountRun * t = ra; t != rc; ++t )
						numsyms += t->rlen;

					encodeSyms();
					encodeCntSeq();
					encodeSBits();

					// push index entry
					index.push_back(IndexEntry(blockpos,rc-ra,numsyms));

					writer.flushBitStream();
				}

				rc = ra;
			}

			template<typename iterator>
			void encode(iterator a, iterator e)
			{
				for ( ; a != e; ++a )
					encode(*a);
			}
		};
	}
}

namespace libmaus2
{
	namespace huffman
	{
		template<typename _huffmanencoderfile_type>
		struct SymCountEncoderTemplate :
			public _huffmanencoderfile_type,
			public SymCountEncoderBaseTemplate< _huffmanencoderfile_type >
		{
			typedef _huffmanencoderfile_type huffmanencoderfile_type;
			typedef SymCountEncoderTemplate<_huffmanencoderfile_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			typedef SymCount value_type;

			SymCountEncoderTemplate(std::string const & filename, uint64_t const bufsize)
			: huffmanencoderfile_type(filename), SymCountEncoderBaseTemplate< huffmanencoderfile_type >(*this,bufsize)
			{

			}

			~SymCountEncoderTemplate()
			{
				flush();
			}
			void flush()
			{
				SymCountEncoderBaseTemplate< huffmanencoderfile_type >::flush();
				huffmanencoderfile_type::flush();
			}
		};

		typedef SymCountEncoderTemplate<HuffmanEncoderFileStd> SymCountEncoderStd;
	}
}
#endif
