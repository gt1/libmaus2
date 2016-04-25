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
#if ! defined(RLENCODER_HPP)
#define RLENCODER_HPP

#include <libmaus2/huffman/IndexWriter.hpp>
#include <libmaus2/huffman/HuffmanEncoderFile.hpp>
#include <libmaus2/util/Histogram.hpp>
#include <libmaus2/huffman/CanonicalEncoder.hpp>
#include <libmaus2/huffman/IndexLoader.hpp>
#include <libmaus2/math/bitsPerNum.hpp>
#include <libmaus2/huffman/PairAddSecond.hpp>
#include <libmaus2/huffman/IndexEntry.hpp>
#include <libmaus2/aio/FileRemoval.hpp>
#include <libmaus2/huffman/IndexDecoderDataArray.hpp>
#include <libmaus2/huffman/RLRun.hpp>

namespace libmaus2
{
	namespace huffman
	{
		template<typename _bit_writer_type>
		struct RLEncoderBaseTemplate : public IndexWriter
		{
			typedef _bit_writer_type bit_writer_type;

			bit_writer_type & writer;
			uint64_t const numsyms;

			typedef std::pair<uint64_t,uint64_t> rl_pair;
			typedef rl_pair run_type;
			::libmaus2::autoarray::AutoArray < rl_pair > rlbuffer;

			rl_pair * const pa;
			rl_pair *       pc;
			rl_pair * const pe;

			uint64_t cursym;
			uint64_t curcnt;

			::libmaus2::util::Histogram globalhist;
			std::vector < IndexEntry > index;

			bool indexwritten;

			RLEncoderBaseTemplate(bit_writer_type & rwriter, uint64_t const rnumsyms, uint64_t const bufsize = 4ull*1024ull*1024ull)
			: writer(rwriter), numsyms(rnumsyms), rlbuffer(bufsize), pa(rlbuffer.begin()), pc(pa), pe(rlbuffer.end()), cursym(0), curcnt(0),
			  indexwritten(false)
			{
				// std::cerr << "Writing RL file of length " << numsyms << std::endl;
				writer.writeElias2((numsyms != std::numeric_limits<uint64_t>::max()) ? numsyms : 0);
			}
			~RLEncoderBaseTemplate()
			{
				flush();
			}


			void flush()
			{
				assert ( pc != pe );

				if ( curcnt )
				{
					*(pc++) = rl_pair(cursym,curcnt);
					curcnt = 0;
				}

				implicitFlush();

				// std::cerr << "Index size " << index.size()*2*sizeof(uint64_t) << " for " << this << std::endl;;

				if ( ! indexwritten )
				{
					writer.flushBitStream();
					uint64_t const indexpos = writer.getPos();

					writeIndex(writer,index,indexpos,numsyms);

					indexwritten = true;
				}

				writer.flush();
			}

			void implicitFlush()
			{
				if ( pc != pa )
				{
					writer.flushBitStream();
					uint64_t const blockpos = writer.getPos();
					uint64_t numsyms = 0;

					::libmaus2::util::Histogram symhist;
					::libmaus2::util::Histogram cnthist;
					for ( rl_pair * pi = pa; pi != pc; ++pi )
					{
						symhist(pi->first);
						cnthist(pi->second);
						globalhist(pi->second);
						numsyms += pi->second;
					}

					if ( ! numsyms )
					{
						::libmaus2::exception::LibMausException se;
						se.getStream() << "numsyms=" << numsyms << " for pc-pa=" << pc-pa << " in RLEncoder::implicitFlush()" << std::endl;
						se.finish();
						throw se;
					}

					index.push_back(IndexEntry(blockpos,pc-pa,numsyms));

					std::vector < std::pair<uint64_t,uint64_t > > const symfreqs = symhist.getFreqSymVector();
					std::vector < std::pair<uint64_t,uint64_t > > const cntfreqs = cnthist.getFreqSymVector();

					assert ( ! ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(symfreqs) );

					::libmaus2::huffman::CanonicalEncoder symenc(symhist.getByType<int64_t>());
					::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type esccntenc;
					::libmaus2::huffman::CanonicalEncoder::unique_ptr_type cntenc;

					bool const cntesc = ::libmaus2::huffman::EscapeCanonicalEncoder::needEscape(cntfreqs);
					if ( cntesc )
					{
						::libmaus2::huffman::EscapeCanonicalEncoder::unique_ptr_type tesccntenc(new ::libmaus2::huffman::EscapeCanonicalEncoder(cntfreqs));
						esccntenc = UNIQUE_PTR_MOVE(tesccntenc);
					}
					else
					{
						::libmaus2::huffman::CanonicalEncoder::unique_ptr_type tcntenc(new ::libmaus2::huffman::CanonicalEncoder(cnthist.getByType<int64_t>()));
						cntenc = UNIQUE_PTR_MOVE(tcntenc);
					}

					// std::cerr << "Writing block of size " << (pc-pa) << std::endl;

					writer.writeElias2(pc-pa);
					writer.writeBit(cntesc);

					symenc.serialise(writer);
					if ( esccntenc )
						esccntenc->serialise(writer);
					else
						cntenc->serialise(writer);

					writer.flushBitStream();

					for ( rl_pair * pi = pa; pi != pc; ++pi )
						symenc.encode(writer,pi->first);

					writer.flushBitStream();

					if ( cntesc )
						for ( rl_pair * pi = pa; pi != pc; ++pi )
							esccntenc->encode(writer,pi->second);
					else
						for ( rl_pair * pi = pa; pi != pc; ++pi )
							cntenc->encode(writer,pi->second);

					writer.flushBitStream();
				}

				pc = pa;
			}

			void encode(uint64_t const sym)
			{
				if ( sym == cursym )
				{
					curcnt++;
				}
				else if ( curcnt )
				{
					*(pc++) = rl_pair(cursym,curcnt);

					if ( pc == pe )
						implicitFlush();

					cursym = sym;
					curcnt = 1;
				}
				else
				{
					assert ( sym != cursym );
					cursym = sym;
					curcnt = 1;
				}
			}

			void encodeRun(RLRun const & R)
			{
				if ( R.sym == cursym )
					curcnt += R.rlen;
				else if ( curcnt )
				{
					*(pc++) = rl_pair(cursym,curcnt);

					if ( pc == pe )
						implicitFlush();

					cursym = R.sym;
					curcnt = R.rlen;
				}
				else
				{
					assert ( R.sym != cursym );
					cursym = R.sym;
					curcnt = R.rlen;
				}
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
		struct RLEncoderTemplate :
			public _huffmanencoderfile_type,
			public RLEncoderBaseTemplate< _huffmanencoderfile_type >
		{
			typedef _huffmanencoderfile_type huffmanencoderfile_type;
			typedef RLEncoderTemplate<_huffmanencoderfile_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			RLEncoderTemplate(std::string const & filename, uint64_t const bufsize)
			: huffmanencoderfile_type(filename), RLEncoderBaseTemplate< huffmanencoderfile_type >(*this,std::numeric_limits<uint64_t>::max(),bufsize)
			{

			}

			RLEncoderTemplate(std::string const & filename, uint64_t const n, uint64_t const bufsize)
			: huffmanencoderfile_type(filename), RLEncoderBaseTemplate< huffmanencoderfile_type >(*this,n,bufsize)
			{

			}

			RLEncoderTemplate(std::string const & filename, unsigned int const /* albits */, uint64_t const n, uint64_t const bufsize)
			: huffmanencoderfile_type(filename), RLEncoderBaseTemplate< huffmanencoderfile_type >(*this,n,bufsize)
			{

			}

			~RLEncoderTemplate()
			{
				flush();
			}
			void flush()
			{
				RLEncoderBaseTemplate< huffmanencoderfile_type >::flush();
				huffmanencoderfile_type::flush();
			}

			static void append(
				std::ostream & out, std::string const & filename,
				std::vector< IndexEntry > & index
			)
			{
				uint64_t const indexpos = IndexLoader::getIndexPos(filename);
				libmaus2::autoarray::AutoArray< IndexEntry > subindex = IndexLoader::loadIndex(filename);

				if ( subindex.size() )
				{
					uint64_t const datapos = subindex[0].pos;
					uint64_t const datalen = indexpos-datapos;

					// shift all towards zero
					for ( uint64_t i = 0; i < subindex.size(); ++i )
						subindex[i].pos -= datapos;

					uint64_t const ndatapos = out.tellp();

					for ( uint64_t i = 0; i < subindex.size(); ++i )
						subindex[i].pos += ndatapos;

					libmaus2::aio::InputStreamInstance istr(filename);
					istr.seekg(datapos,std::ios::beg);
					::libmaus2::util::GetFileSize::copy ( istr, out, datalen, 1 );

					for ( uint64_t i = 0; i < subindex.size(); ++i )
						index.push_back(subindex[i]);
				}
			}

			template<typename out_type>
			static void appendTemplate(
				out_type & out, std::string const & filename,
				std::vector< IndexEntry > & index
			)
			{
				uint64_t const indexpos = IndexLoader::getIndexPos(filename);
				libmaus2::autoarray::AutoArray< IndexEntry > subindex = IndexLoader::loadIndex(filename);

				if ( subindex.size() )
				{
					uint64_t const datapos = subindex[0].pos;
					uint64_t const datalen = indexpos-datapos;

					// shift all towards zero
					for ( uint64_t i = 0; i < subindex.size(); ++i )
						subindex[i].pos -= datapos;

					uint64_t const ndatapos = out.tellp();

					for ( uint64_t i = 0; i < subindex.size(); ++i )
						subindex[i].pos += ndatapos;

					libmaus2::aio::InputStreamInstance istr(filename);
					istr.seekg(datapos,std::ios::beg);
					::libmaus2::util::GetFileSize::copyIterator ( istr, out.getIterator(), datalen, 1 );

					for ( uint64_t i = 0; i < subindex.size(); ++i )
						index.push_back(subindex[i]);
				}
			}

			static uint64_t getLength(std::vector<std::string> const & filenames)
			{
				uint64_t n = 0;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
					n += getLength(filenames[i]);
				return n;
			}

			static uint64_t getLength(std::string const & filename)
			{
				libmaus2::huffman::IndexDecoderData IDD(filename);
				return IDD.vacc;
			}

			static void concatenate(std::vector<std::string> const & filenames, std::string const & outfilename, bool const removeinput = false)
			{
				uint64_t const n = getLength(filenames);
				huffmanencoderfile_type writer(outfilename);

				writer.writeElias2(n);
				writer.flushBitStream();

				std::vector< IndexEntry > index;
				for ( uint64_t i = 0; i < filenames.size(); ++i )
				{
					appendTemplate(writer,filenames[i],index);
					if ( removeinput )
						libmaus2::aio::FileRemoval::removeFile(filenames[i]);
				}

				writer.flushBitStream();

				uint64_t const indexpos = writer.getPos();
				RLEncoderBaseTemplate< huffmanencoderfile_type >::writeIndex(writer,index,indexpos,n);

				writer.flush();
			}
		};

		typedef RLEncoderTemplate<HuffmanEncoderFileStd> RLEncoderStd;
	}
}
#endif
