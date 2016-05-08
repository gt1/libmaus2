/*
    libmaus2
    Copyright (C) 2016 German Tischler

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
#if ! defined(LIBMAUS2_GAMMA_GAMMAFLAGGEDINTERVALDECODER_HPP)
#define LIBMAUS2_GAMMA_GAMMAFLAGGEDINTERVALDECODER_HPP

#include <libmaus2/huffman/IndexDecoderDataArray.hpp>
#include <libmaus2/gamma/GammaDecoder.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/gamma/FlaggedInterval.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct GammaFlaggedIntervalDecoder
		{
			typedef GammaFlaggedIntervalDecoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			struct BlockEntry
			{
				uint64_t low;
				uint64_t high;
				uint64_t offset;

				BlockEntry()
				{

				}

				BlockEntry(uint64_t rlow, uint64_t rhigh, uint64_t roffset) : low(rlow), high(rhigh), offset(roffset) {}
			};

			struct IndexSingleAccessor
			{
				typedef IndexSingleAccessor this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

				std::string const & fn;
				mutable libmaus2::aio::InputStreamInstance ISI;

				typedef libmaus2::util::ConstIterator<this_type,BlockEntry> const_iterator;

				uint64_t indexoffset;
				uint64_t numblocks;

				uint64_t filelow;
				uint64_t filehigh;

				IndexSingleAccessor(std::string const & rfn)
				: fn(rfn), ISI(fn)
				{
					ISI.clear();
					ISI.seekg(-static_cast<int64_t>(2*sizeof(uint64_t)), std::ios::end);
					numblocks = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					indexoffset = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);

					if ( numblocks )
					{
						filelow = get(0).low;
						filehigh = get(numblocks-1).high;
					}
				}

				BlockEntry get(uint64_t i) const
				{
					ISI.clear();
					ISI.seekg(indexoffset + 3*sizeof(uint64_t)*i);
					BlockEntry B;
					B.low = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					B.high = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					B.offset = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					return B;
				}

				BlockEntry operator[](uint64_t i) const
				{
					return get(i);
				}

				const_iterator begin() const
				{
					return const_iterator(this,0);
				}

				const_iterator end() const
				{
					return const_iterator(this,numblocks);
				}

				struct HighComp
				{
					bool operator()(BlockEntry const & A, BlockEntry const & B) const
					{
						return A.high < B.high;
					}
				};

				uint64_t getBlockForValue(uint64_t const v) const
				{
					const_iterator it = ::std::lower_bound(begin(),end(),BlockEntry(0,v,0),HighComp());
					return it-begin();
				}
			};

			struct IndexAccessor
			{
				typedef IndexAccessor this_type;

				std::vector<std::string> Vfn;
				std::vector< std::pair<uint64_t,uint64_t> > intv;

				IndexAccessor(std::vector<std::string> const & rVfn)
				: Vfn(rVfn)
				{
					uint64_t o = 0;
					for ( uint64_t i = 0; i < Vfn.size(); ++i )
					{
						IndexSingleAccessor ISA(Vfn[i]);
						if ( ISA.numblocks )
						{
							Vfn[o++] = Vfn[i];
							intv.push_back(std::pair<uint64_t,uint64_t>(ISA.filelow,ISA.filehigh));
						}
					}
					Vfn.resize(o);
				}

				struct PairSecondComp
				{
					bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
					{
						return A.second < B.second;
					}
				};

				uint64_t getFileForValue(uint64_t const v) const
				{
					std::vector< std::pair<uint64_t,uint64_t> >::const_iterator it =
						::std::lower_bound(intv.begin(),intv.end(),std::pair<uint64_t,uint64_t>(0,v),PairSecondComp());
					return it - intv.begin();
				}
			};

			IndexAccessor IA;
			IndexSingleAccessor::unique_ptr_type PISA;
			// std::vector<std::string> Vfn;
			// libmaus2::huffman::IndexDecoderDataArray::unique_ptr_type Pindex;
			// libmaus2::huffman::IndexDecoderDataArray & index;

			uint64_t fileptr;
			uint64_t blockptr;

			libmaus2::aio::InputStreamInstance::unique_ptr_type istr;

			typedef uint64_t gamma_data_type;
			typedef libmaus2::aio::SynchronousGenericInput<gamma_data_type> stream_type;
			typedef stream_type::unique_ptr_type stream_ptr_type;
			stream_ptr_type PSGI;

			typedef libmaus2::gamma::GammaDecoder<stream_type> gamma_decoder_type;
			typedef gamma_decoder_type::unique_ptr_type gamma_decoder_ptr_type;
			gamma_decoder_ptr_type PG;

			libmaus2::autoarray::AutoArray < FlaggedInterval > Aintv;
			FlaggedInterval * pa;
			FlaggedInterval * pc;
			FlaggedInterval * pe;

			void openNewFile()
			{
				if ( fileptr < IA.intv.size() ) // file ptr valid, does file exist?
				{
					IndexSingleAccessor::unique_ptr_type TISA(new IndexSingleAccessor(IA.Vfn[fileptr]));
					PISA = UNIQUE_PTR_MOVE(TISA);

					assert ( blockptr < PISA->numblocks ); // check block pointer
					// open new input file stream

					libmaus2::aio::InputStreamInstance::unique_ptr_type tistr(
						new libmaus2::aio::InputStreamInstance(IA.Vfn[fileptr]));
					istr = UNIQUE_PTR_MOVE(tistr);

					// seek to position and check if we succeeded
					uint64_t const pos = (*PISA)[blockptr].offset;
					uint64_t const bitsperword = 8*sizeof(uint64_t);

					uint64_t const fullwordsoffset = pos / bitsperword;
					uint64_t const bytesoffset = fullwordsoffset * sizeof(uint64_t);
					uint64_t const bitoffset = pos - bitsperword * fullwordsoffset;
					assert ( bitoffset < 64 );

					istr->clear();
					istr->seekg(bytesoffset,std::ios::beg);

					stream_ptr_type TSGI(new stream_type(*istr,8*1024,std::numeric_limits<uint64_t>::max(),false /* check mod */));
					PSGI = UNIQUE_PTR_MOVE(TSGI);

		                        gamma_decoder_ptr_type TG(new gamma_decoder_type(*PSGI));
		                        PG = UNIQUE_PTR_MOVE(TG);

					if ( bitoffset )
						PG->decodeWord(bitoffset);
				}
			}

			bool fillBuffer()
			{
				while ( pc == pe )
				{
					bool newfile = false;

					// check if we need to open a new file
					while ( fileptr < IA.intv.size() && blockptr == PISA->numblocks )
					{
						fileptr++;
						blockptr = 0;
						newfile = true;
					}

					// we have reached the end, no more blocks
					if ( fileptr == IA.intv.size() )
						return false;
					// open new file if necessary
					if ( newfile )
						openNewFile();

					uint64_t const numintv = PG->decode()+1;
					Aintv.ensureSize(numintv);

					pa = Aintv.begin();
					pc = Aintv.begin();
					pe = Aintv.begin() + numintv;

					uint64_t firstlow = PG->decode();
					uint64_t firstwidth = PG->decode();
					pa[0].from = firstlow;
					pa[0].to = firstlow + firstwidth;

					for ( uint64_t i = 1; i < numintv; ++i )
					{
						pa[i].from = pa[i-1].to + PG->decode();
						pa[i].to = pa[i].from + PG->decode();
					}

					uint64_t numbits = 0;
					while ( numbits < numintv )
					{
						FlaggedInterval::interval_type const type = static_cast<FlaggedInterval::interval_type>(PG->decodeWord(2));
						uint64_t const len = PG->decode() + 1;

						for ( uint64_t i = 0; i < len; ++i )
							pa[numbits++].type = type;
					}

					blockptr += 1;
				}

				return true;
			}

			bool getNext(FlaggedInterval & P)
			{
				if ( pc != pe )
				{
					P = *(pc++);
					return true;
				}
				else
				{
					fillBuffer();

					if ( pc == pe )
						return false;

					P = *(pc++);
					return true;
				}
			}

			GammaFlaggedIntervalDecoder(std::vector<std::string> const & rVfn, uint64_t const voffset)
			: IA(rVfn),
			  fileptr(0),
			  Aintv(), pa(Aintv.begin()), pc(Aintv.begin()), pe(Aintv.begin())
			{
				fileptr = IA.getFileForValue(voffset);

				if ( fileptr < IA.Vfn.size() )
				{
					IndexSingleAccessor::unique_ptr_type TISA(new IndexSingleAccessor(IA.Vfn[fileptr]));
					PISA = UNIQUE_PTR_MOVE(TISA);

					blockptr = PISA->getBlockForValue(voffset);
					assert ( blockptr < PISA->numblocks );

					// std::cerr << "voffset=" << voffset << " filteptr=" << fileptr << " blockptr=" << blockptr << std::endl;

					openNewFile();

					bool done = false;

					while ( !done )
					{
						bool const ok = fillBuffer();

						assert ( ok );

						while (
							pc != pe &&
							voffset != pc->from &&
							voffset >= pc->to
						)
							++pc;

						if ( pc != pe )
							done = true;
					}
				}
			}

			static bool isEmpty(std::string const & filename)
			{
				IndexSingleAccessor ISA(filename);
				return ISA.numblocks == 0;
			};
		};
	}
}
#endif
