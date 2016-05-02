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
#if ! defined(LIBMAUS2_SORTING_PARALLELEXTERNALRADIXSORT_HPP)
#define LIBMAUS2_SORTING_PARALLELEXTERNALRADIXSORT_HPP

#include <libmaus2/math/numbits.hpp>
#include <libmaus2/huffman/RLInitType.hpp>
#include <libmaus2/math/lowbits.hpp>
#include <libmaus2/huffman/IndexDecoderDataArray.hpp>
#include <libmaus2/util/PrefixSums.hpp>
#include <libmaus2/util/TempFileNameGenerator.hpp>

namespace libmaus2
{
	namespace sorting
	{
		struct ParallelExternalRadixSort
		{
			template<
				typename decoder_type,
				typename encoder_type,
				typename projector_type
			>
			static std::vector<std::string> parallelRadixSort(
				std::vector<std::string> Vfn,
				uint64_t const tnumthreads,
				uint64_t const maxfiles,
				bool deleteinput,
				libmaus2::util::TempFileNameGenerator & tmpgen,
				// block size for output
				uint64_t bs,
				// range restriction
				uint64_t ilow,
				uint64_t ihigh,
				uint64_t maxsym,
				bool maxsymvalid,
				uint64_t const keybs,
				uint64_t * rmaxsym = 0
			)
			{
				// we need at least 3 fils per thread (one input, two output)
				uint64_t const maxthreads = maxfiles / 3;
				// compute maximum number of threads we can actually use
				uint64_t const numthreads = std::min(tnumthreads,maxthreads);

				if ( ! numthreads )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "parallelRadixSort: maxfiles=" << maxfiles << " is too small (needs to be at least 3)" << std::endl;
					lme.finish();
					throw lme;
				}

				// maximum number of files per thread
				uint64_t const filesperthread = maxfiles / numthreads;
				// maximum number of output files per thread
				uint64_t const ofilesperthread = filesperthread - 1;

				uint64_t tfilebits = 1;
				uint64_t ofilecnt = 2;

				while ( ofilecnt * 2 <= ofilesperthread )
				{
					ofilecnt *= 2;
					tfilebits += 1;
				}
				assert ( ofilecnt <= ofilesperthread );

				// total symbol bits
				uint64_t totalsymbits = maxsymvalid ? libmaus2::math::numbits(maxsym) : libmaus2::math::numbits(std::numeric_limits<uint64_t>::max());

				unsigned int rshift = 0;
				while ( rshift < totalsymbits )
				{
					// rest bits to sort by
					uint64_t const restbits = totalsymbits - rshift;
					// minimum rounds left
					uint64_t const minrounds = (restbits + tfilebits - 1)/tfilebits;
					// number of file bits per round/this round
					uint64_t const filebits = (restbits + minrounds -1)/minrounds;
					// sanity checks
					assert ( (1ull << filebits) <= ofilesperthread );
					assert ( minrounds * filebits + rshift >= totalsymbits );

					uint64_t const outfilesperthread = (1ull<<filebits);
					uint64_t const totaloutfiles = numthreads * outfilesperthread;

					libmaus2::autoarray::AutoArray < typename encoder_type::unique_ptr_type > Aoutfiles(totaloutfiles);

					unsigned int const roundbits = filebits;
					uint64_t const rmask = ::libmaus2::math::lowbits(roundbits);

					uint64_t const isize = ihigh-ilow;
					uint64_t const packsize = (isize+numthreads-1)/numthreads;
					uint64_t const runthreads = (isize+packsize-1)/packsize;

					std::vector<std::string> Vout(runthreads * outfilesperthread);
					std::vector<std::string> Vkey(runthreads);

					for ( uint64_t i = 0; i < outfilesperthread; ++i )
						for ( uint64_t t = 0; t < runthreads; ++t )
						{
							// sym major
							uint64_t const fnid = i * runthreads + t;
							Vout[fnid] = tmpgen.getFileName(true);
							// thread major
							uint64_t const tid = t*outfilesperthread+i;
							typename encoder_type::unique_ptr_type Tenc(new encoder_type(Vout[fnid],bs));
							Aoutfiles[tid] = UNIQUE_PTR_MOVE(Tenc);
						}

					::libmaus2::huffman::IndexDecoderDataArray IDDA(Vfn,numthreads);
					uint64_t volatile cmaxsym = 0;
					libmaus2::parallel::PosixSpinLock cmaxsymlock;

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(runthreads)
					#endif
					for ( uint64_t t = 0; t < runthreads; ++t )
					{
						uint64_t const tlow = ilow + t * packsize;
						uint64_t const thigh = std::min( tlow+packsize, ihigh );
						uint64_t const trange = thigh-tlow;

						typename encoder_type::unique_ptr_type * enc = Aoutfiles.begin() + t * outfilesperthread;
						typename decoder_type::value_type V;

						decoder_type dec(IDDA,tlow);

						if ( maxsymvalid )
						{
							uint64_t todo = trange;

							while ( todo )
							{
								dec.decode(V);
								uint64_t const key = (projector_type::project(V) >> rshift) & rmask;
								enc[key]->encode(V);
								todo --;
							}
						}
						else
						{
							uint64_t lmaxsym = 0;

							uint64_t todo = trange;

							while ( todo )
							{
								dec.decode(V);
								uint64_t const key = (projector_type::project(V) >> rshift) & rmask;
								enc[key]->encode(V);
								todo --;
								if ( V > static_cast<int64_t>(lmaxsym) )
									lmaxsym = V;
							}

							cmaxsymlock.lock();
							if ( lmaxsym > cmaxsym )
								cmaxsym = lmaxsym;
							cmaxsymlock.unlock();

						}

						for ( uint64_t i = 0; i < outfilesperthread; ++i )
							(Aoutfiles.begin() + t * outfilesperthread)[i].reset();
					}

					if ( deleteinput )
						for ( uint64_t i = 0; i < Vfn.size(); ++i )
							libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
					deleteinput = true;

					Vfn = Vout;

					ilow = 0;
					ihigh = decoder_type::getLength(Vfn,numthreads);
					rshift += roundbits;

					if ( ! maxsymvalid )
					{
						// std::cerr << "[V] setting maxsym to " << cmaxsym << std::endl;
						maxsym = cmaxsym;
						maxsymvalid = true;
						totalsymbits = libmaus2::math::numbits(maxsym);
					}
				}

				assert ( maxsymvalid );

				if ( rmaxsym )
					*rmaxsym = maxsym;

				return Vfn;
			}
		};
	}
}
#endif
