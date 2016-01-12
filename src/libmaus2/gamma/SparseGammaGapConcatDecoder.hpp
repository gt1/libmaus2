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
#if ! defined(LIBMAUS2_GAMMA_SPARSEGAMMAGAPCONCATDECODER_HPP)
#define LIBMAUS2_GAMMA_SPARSEGAMMAGAPCONCATDECODER_HPP

#include <libmaus2/gamma/SparseGammaGapFileIndexMultiDecoder.hpp>

namespace libmaus2
{
	namespace gamma
	{

		template<typename _data_type>
		struct SparseGammaGapConcatDecoderTemplate
		{
			typedef _data_type data_type;
			typedef SparseGammaGapConcatDecoderTemplate<data_type> this_type;

			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef libmaus2::aio::SynchronousGenericInput<data_type> stream_type;

			SparseGammaGapFileIndexMultiDecoder::unique_ptr_type Pindex;
			SparseGammaGapFileIndexMultiDecoder & index;

			std::vector<std::string> const filenames;
			uint64_t fileptr;
			libmaus2::aio::InputStreamInstance::unique_ptr_type CIS;
			typename stream_type::unique_ptr_type SGI;
			typename libmaus2::gamma::GammaDecoder<stream_type>::unique_ptr_type gdec;
			std::pair<data_type,data_type> p;

			struct iterator
			{
				this_type * owner;
				uint64_t v;

				iterator()
				: owner(0), v(0)
				{

				}
				iterator(this_type * rowner)
				: owner(rowner), v(owner->decode())
				{

				}

				uint64_t operator*() const
				{
					return v;
				}

				iterator operator++(int)
				{
					iterator copy = *this;
					v = owner->decode();
					return copy;
				}

				iterator operator++()
				{
					v = owner->decode();
					return *this;
				}
			};

			void openNextFile()
			{
				if ( fileptr < filenames.size() )
				{
					// std::cerr << "opening file " << fileptr << std::endl;

					gdec.reset();
					SGI.reset();
					CIS.reset();

					libmaus2::aio::InputStreamInstance::unique_ptr_type tCIS(new libmaus2::aio::InputStreamInstance(filenames[fileptr++]));
					CIS = UNIQUE_PTR_MOVE(tCIS);

					typename stream_type::unique_ptr_type tSGI(new stream_type(*CIS,8*1024));
					SGI = UNIQUE_PTR_MOVE(tSGI);

					typename libmaus2::gamma::GammaDecoder<stream_type>::unique_ptr_type tgdec(new libmaus2::gamma::GammaDecoder<stream_type>(*SGI));
					gdec = UNIQUE_PTR_MOVE(tgdec);

					p.first = gdec->decode();
					p.second = gdec->decode();
				}
				else
				{
					p.first = 0;
					p.second = 0;
				}
			}

			SparseGammaGapConcatDecoderTemplate(std::vector<std::string> const & rfilenames, uint64_t const ikey = 0)
			: Pindex(new SparseGammaGapFileIndexMultiDecoder(rfilenames)), index(*Pindex), filenames(index.getFileNames())
			{
				seek(ikey);
			}

			SparseGammaGapConcatDecoderTemplate(SparseGammaGapFileIndexMultiDecoder & rindex, uint64_t const ikey = 0)
			: Pindex(), index(rindex), filenames(index.getFileNames())
			{
				seek(ikey);
			}


			bool hasNextKey() const
			{
				return p.second != 0;
			}


			void seek(uint64_t const ikey)
			{
				p.first = 0;
				p.second = 0;

				// std::cerr << "seeking to " << ikey << std::endl;

				std::pair<uint64_t,uint64_t> const P = index.getBlockIndex(ikey);
				fileptr = P.first;

				// std::cerr << "fileptr=" << fileptr << " blockptr=" << P.second << std::endl;

				if ( fileptr < filenames.size() )
				{
					uint64_t const curfileid = fileptr++;
					std::string const fn = filenames[curfileid];

					libmaus2::aio::InputStreamInstance::unique_ptr_type tCIS(new libmaus2::aio::InputStreamInstance(fn));
					CIS = UNIQUE_PTR_MOVE(tCIS);

					SparseGammaGapFileIndexDecoder & indexdec = index.getSingleDecoder(curfileid); // (*CIS);
					uint64_t const minkey = indexdec.getMinKey();

					// std::cerr << "minkey=" << minkey << std::endl;

					if ( ikey < minkey )
					{
						// this should only happen for the first file
						assert ( curfileid == 0 ); // value has been incremented above
						assert ( indexdec.getBlockIndex(ikey) == 0 );
						assert ( indexdec.get(indexdec.getBlockIndex(ikey)).ibitoff == 0 );

						// seek to front of file
						CIS->clear();
						CIS->seekg(0);

						typename stream_type::unique_ptr_type tSGI(new stream_type(*CIS,8*1024));
						SGI = UNIQUE_PTR_MOVE(tSGI);

						typename libmaus2::gamma::GammaDecoder<stream_type>::unique_ptr_type tgdec(new libmaus2::gamma::GammaDecoder<stream_type>(*SGI));
						gdec = UNIQUE_PTR_MOVE(tgdec);

						p.first = gdec->decode() - ikey;
						p.second = gdec->decode();
					}
					else
					{
						uint64_t const block = indexdec.getBlockIndex(ikey);
						uint64_t const ibitoff = indexdec.get(block).ibitoff;
						uint64_t offset = ikey-indexdec.get(block).ikey;
						uint64_t const word = ibitoff / (sizeof(data_type)*CHAR_BIT);
						uint64_t const wbitoff = ibitoff - word*(sizeof(data_type)*CHAR_BIT);

						// seek to word where we start
						CIS->clear();
						CIS->seekg(word * sizeof(data_type));

						// set up word reader
						typename stream_type::unique_ptr_type tSGI(new stream_type(*CIS,8*1024));
						SGI = UNIQUE_PTR_MOVE(tSGI);

						// set up gamma decoder
						typename libmaus2::gamma::GammaDecoder<stream_type>::unique_ptr_type tgdec(new libmaus2::gamma::GammaDecoder<stream_type>(*SGI));
						gdec = UNIQUE_PTR_MOVE(tgdec);
						// discard wbitoff bits
						if ( wbitoff )
							gdec->decodeWord(wbitoff);

						// read pair and discard difference part
						p.first = gdec->decode();
						p.first = 0;
						p.second = gdec->decode();

						// we seeked to a block start, this should not be zero
						assert ( GammaDecoderBase<data_type>::isNonNull(p.second) );

						// std::cerr << "offset " << offset << std::endl;

						// skip value if we do not stay on the start of the block
						if ( offset )
						{
							// skip one value
							offset -= 1;

							// read next pair
							p.first = gdec->decode();
							p.second = gdec->decode();
						}

						// std::cerr << "p=" << p.first << "," << p.second << std::endl;

						while ( data_type(offset) >= p.first+1 )
						{
							if ( p.second == 0 )
							{
								if ( fileptr == filenames.size() )
									break;
								else
									openNextFile();
							}
							else
							{
								offset -= SparseGammaGapDecoderNumberCast<data_type>::cast(p.first)+1;

								p.first = gdec->decode();
								p.second = gdec->decode();
							}
						}

						assert ( !GammaDecoderBase<data_type>::isNonNull(p.second) || data_type(offset) <= p.first );

						for ( ; offset; --offset )
							decode();

						// make sure p.second is not 0 if there is still more data
						// (not necessary for decode(), but confusing for others)
						while ( p.second == 0 && fileptr != filenames.size() )
							openNextFile();
					}
				}

			}


			uint64_t decode()
			{
				// no more non zero values
				while ( !GammaDecoderBase<data_type>::isNonNull(p.second) && fileptr < filenames.size() )
					openNextFile();

				if ( !GammaDecoderBase<data_type>::isNonNull(p.second) )
				{
					return 0;
				}

				// zero value
				if ( GammaDecoderBase<data_type>::isNonNull(p.first) )
				{
					p.first -= 1;
					return 0;
				}
				// non zero value
				else
				{
					uint64_t const retval = SparseGammaGapDecoderNumberCast<data_type>::cast(p.second);

					// get information about next non zero value
					p.first = gdec->decode();
					p.second = gdec->decode();

					return retval;
				}
			}

			std::pair<uint64_t,uint64_t> nextPair()
			{
				p.first = gdec->decode();
				p.second = gdec->decode();

				// no more non zero values
				while ( !GammaDecoderBase<data_type>::isNonNull(p.second) && fileptr < filenames.size() )
					openNextFile();

				return std::pair<uint64_t,uint64_t>(
					SparseGammaGapDecoderNumberCast<data_type>::cast(p.first),
					SparseGammaGapDecoderNumberCast<data_type>::cast(p.second)
				);
			}

			uint64_t nextFirst()
			{
				nextPair();
				return SparseGammaGapDecoderNumberCast<data_type>::cast(p.first);
			}

			uint64_t nextSecond()
			{
				return SparseGammaGapDecoderNumberCast<data_type>::cast(p.second);
			}

			iterator begin()
			{
				return iterator(this);
			}

			static uint64_t getNextKey(libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder & index, uint64_t const ikey)
			{
				this_type dec(index,ikey);
				assert ( dec.hasNextKey() );
				return ikey + SparseGammaGapDecoderNumberCast<data_type>::cast(dec.p.first);
			}
			static bool hasNextKey(libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder & index, /* std::vector<std::string> const & filenames, */ uint64_t const ikey)
			{
				return this_type(index,ikey).hasNextKey();
			}

			private:
			static uint64_t getPrevKeyBlockStart(libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder & index, uint64_t const ikey)
			{
				assert ( index.hasPrevKey(ikey) );
				std::pair<uint64_t,uint64_t> const p = index.getBlockIndex(ikey-1);
				assert ( p.first < index.getFileNames().size() );
				libmaus2::gamma::SparseGammaGapFileIndexDecoder const & index1 = index.getSingleDecoder(p.first); // (filenames[p.first]);
				return index1.get(p.second).ikey;
			}

			public:
			// get highest non-zero key before ikey or -1 if there is no such key
			static int64_t getPrevKey(libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder & index, /* std::vector<std::string> const & filenames, */ uint64_t const ikey)
			{
				// libmaus2::gamma::SparseGammaGapFileIndexMultiDecoder index(filenames);

				if ( ! index.hasPrevKey(ikey) )
					return -1;

				uint64_t const prevblockstart = getPrevKeyBlockStart(index,ikey);

				this_type dec(index,prevblockstart);

				assert ( dec.p.first == 0 );

				uint64_t curkey = prevblockstart;

				while ( true )
				{
					std::pair<uint64_t,uint64_t> const p = dec.nextPair();

					if ( ! p.second )
						return curkey;

					uint64_t const nextkey = curkey + (1 + p.first);

					if ( nextkey >= ikey )
						return curkey;
					else
						curkey = nextkey;
				}
			}
		};

		typedef SparseGammaGapConcatDecoderTemplate<uint64_t> SparseGammaGapConcatDecoder;
		typedef SparseGammaGapConcatDecoderTemplate< libmaus2::math::UnsignedInteger<4> > SparseGammaGapConcatDecoder2;
	}
}
#endif
