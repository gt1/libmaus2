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

#include <libmaus2/huffman/SymCount.hpp>

namespace libmaus2
{
	namespace huffman
	{
		template<typename _bit_writer_type>
		struct SymCountEncoderBaseTemplate : public IndexWriter
		{
			typedef _bit_writer_type bit_writer_type;

			bit_writer_type & writer;

			::libmaus2::autoarray::AutoArray < SymCount > symcntbuffer;
			::libmaus2::autoarray::AutoArray < std::pair<int64_t,uint64_t> > symrlbuffer;
			::libmaus2::autoarray::AutoArray < std::pair<int64_t,uint64_t> > cntrlbuffer;

			SymCount * const pa;
			SymCount *       pc;
			SymCount * const pe;

			std::vector < IndexEntry > index;

			bool indexwritten;

			SymCountEncoderBaseTemplate(bit_writer_type & rwriter, uint64_t const bufsize = 4ull*1024ull*1024ull)
			: writer(rwriter), symcntbuffer(bufsize), pa(symcntbuffer.begin()), pc(pa), pe(symcntbuffer.end()),
			  indexwritten(false)
			{
			}
			~SymCountEncoderBaseTemplate()
			{
				flush();
			}


			void flush()
			{
				assert ( pc != pe );

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

			std::pair<uint64_t,uint64_t> computeSymRunsAndHist(::libmaus2::util::Histogram & symhist, ::libmaus2::util::Histogram & symcnthist)
			{
				// compute symbol runs
				SymCount * t = pa;
				uint64_t numsymruns = 0;
				uint64_t numsyms = 0;
				while ( t != pc )
				{
					int64_t const refsym = t->sym;
					SymCount * tl = t++;
					while ( t != pc && t->sym == refsym )
						++t;

					symhist(refsym);
					uint64_t const lnumsyms = t-tl;
					symcnthist(lnumsyms);
					numsyms += lnumsyms;

					symrlbuffer.push(numsymruns,std::pair<int64_t,uint64_t>(refsym,lnumsyms));
				}
				assert ( numsyms );

				return std::pair<uint64_t,uint64_t>(numsymruns,numsyms);
			}

			std::pair<uint64_t,uint64_t> computeCntRunsAndHist(::libmaus2::util::Histogram & cntcnthist)
			{
				uint64_t numcntruns = 0;
				uint64_t numcntsyms = 0;

				// compute count runs
				SymCount * t = pa;
				while ( t != pc )
				{
					int64_t const refcnt = t->cnt;
					SymCount * tl = t++;
					while ( t != pc && t->cnt == refcnt )
						++t;

					uint64_t const locc = t-tl;
					cntcnthist(locc);
					numcntsyms += locc;

					cntrlbuffer.push(numcntruns,std::pair<int64_t,uint64_t>(refcnt,locc));
				}

				return std::pair<uint64_t,uint64_t>(numcntruns,numcntsyms);
			}

			// encode sequence of run symbols
			void encodeSymSeq(::libmaus2::util::Histogram & symhist, uint64_t const numsymruns)
			{
				// get symbol/freq vectors
				std::vector < std::pair<uint64_t,uint64_t > > const symfreqs = symhist.getFreqSymVector();
				assert ( ! ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(symfreqs) );

				::libmaus2::huffman::CanonicalEncoder symenc(symhist.getByType<int64_t>());
				// serialise symbol encoder
				symenc.serialise(writer);

				// write symbol values
				for ( std::pair<int64_t,uint64_t> * pi = symrlbuffer.begin(); pi != symrlbuffer.begin()+numsymruns; ++pi )
					symenc.encode(writer,pi->first);
			}

			void encodeSymCntSeq(::libmaus2::util::Histogram & symcnthist, uint64_t const numsymruns)
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
					for ( std::pair<int64_t,uint64_t> * pi = symrlbuffer.begin(); pi != symrlbuffer.begin()+numsymruns; ++pi )
						escsymcntenc->encode(writer,pi->second);
				}
				else
				{
					symcntenc->serialise(writer);
					for ( std::pair<int64_t,uint64_t> * pi = symrlbuffer.begin(); pi != symrlbuffer.begin()+numsymruns; ++pi )
						symcntenc->encode(writer,pi->second);
				}
			}

			void encodeCntSeq(uint64_t const numcntruns)
			{
				// write cnt values (gamma code)
				for ( std::pair<int64_t,uint64_t> * pi = cntrlbuffer.begin(); pi != cntrlbuffer.begin()+numcntruns; ++pi )
				{
					uint64_t const v = pi->first + 1;
					unsigned int const nd = libmaus2::gamma::GammaEncoderBase<uint64_t>::getLengthCode(v);
					// nd zero bits
					writer.write(0,nd);
					// v using nd+1 bits
					writer.write(v,nd+1);
				}
			}

			void encodeCntCntSeq(::libmaus2::util::Histogram & cntcnthist, uint64_t const numcntruns)
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
					for ( std::pair<int64_t,uint64_t> * pi = cntrlbuffer.begin(); pi != cntrlbuffer.begin()+numcntruns; ++pi )
						esccntcntenc->encode(writer,pi->second);
				}
				else
				{
					cntcntenc->serialise(writer);
					for ( std::pair<int64_t,uint64_t> * pi = cntrlbuffer.begin(); pi != cntrlbuffer.begin()+numcntruns; ++pi )
						cntcntenc->encode(writer,pi->second);
				}
			}

			void encodeSBits()
			{
				// store s bits
				for ( SymCount * pt = pa; pt != pc; ++pt )
				{
					// write sbit
					writer.writeBit(pt->sbit);
				}
			}

			std::pair<uint64_t,uint64_t> encodeSyms()
			{
				::libmaus2::util::Histogram symhist;
				::libmaus2::util::Histogram symcnthist;
				std::pair<uint64_t,uint64_t> const Psym = computeSymRunsAndHist(symhist,symcnthist);
				uint64_t const numsymruns = Psym.first;

				// store number of symbol runs
				writer.writeElias2(numsymruns);
				encodeSymSeq(symhist,numsymruns);
				encodeSymCntSeq(symcnthist,numsymruns);

				return Psym;
			}

			std::pair<uint64_t,uint64_t> encodeCnts()
			{
				::libmaus2::util::Histogram cntcnthist;
				std::pair<uint64_t,uint64_t> const Pcnt = computeCntRunsAndHist(cntcnthist);
				uint64_t const numcntruns = Pcnt.first;

				// store number of cnt runs
				writer.writeElias2(numcntruns);
				encodeCntSeq(numcntruns);
				encodeCntCntSeq(cntcnthist,numcntruns);

				return Pcnt;
			}

			void implicitFlush()
			{
				if ( pc != pa )
				{
					writer.flushBitStream();
					uint64_t const blockpos = writer.getPos();

					std::pair<uint64_t,uint64_t> const IS = encodeSyms();
					std::pair<uint64_t,uint64_t> const IC = encodeCnts();
					encodeSBits();
					assert ( IS.second == IC.second );

					// push index entry
					index.push_back(IndexEntry(blockpos,IS.first,IS.second));

					writer.flushBitStream();
				}

				pc = pa;
			}

			void encode(SymCount const & SC)
			{
				*(pc++) = SC;
				if ( pc == pe )
					implicitFlush();
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
