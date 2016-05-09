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
#if ! defined(LIBMAUS2_SORTING_PARALLELRUNLENGTHRADIXUNSORT_HPP)
#define LIBMAUS2_SORTING_PARALLELRUNLENGTHRADIXUNSORT_HPP

#include <libmaus2/util/TempFileNameGenerator.hpp>
#include <libmaus2/aio/FileRemoval.hpp>

namespace libmaus2
{
	namespace sorting
	{
		struct ParallelRunLengthRadixUnsort
		{
			typedef ParallelRunLengthRadixUnsort this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			struct UnsortLevel
			{
				// key sequence
				std::vector < std::string > keyseqfn;
				// key bits
				unsigned int bits;
				// key histogram
				std::vector < uint64_t > Ghist;
				// key scan offsets
				std::vector < uint64_t > Ohist;
				// threading intervals
				std::vector < std::pair<uint64_t,uint64_t> > Vthreadint;

				void removeKeyFiles()
				{
					for ( uint64_t i = 0; i < keyseqfn.size(); ++i )
						libmaus2::aio::FileRemoval::removeFile(keyseqfn[i]);
				}
			};

			void removeKeyFiles()
			{
				for ( uint64_t i = 0; i < levels.size(); ++i )
					levels[i].removeKeyFiles();
			}

			~ParallelRunLengthRadixUnsort()
			{
				removeKeyFiles();
			}

			std::vector<UnsortLevel> levels;
			uint64_t unsortthreads;
			uint64_t totalsyms;

			template<typename decoder_type, typename encoder_type, typename key_decoder_type>
			std::vector<std::string> unsort(std::vector<std::string> Vin, bool deleteinput, libmaus2::util::TempFileNameGenerator & tmpgen, uint64_t const encbs)
			{
				// iterate over levels
				for ( uint64_t zz = 0; zz < levels.size(); ++zz )
				{
					// back to front
					uint64_t const z = levels.size()-zz-1;
					// get level
					UnsortLevel const & L = levels[z];
					// number of input files per thread
					uint64_t const numinputfiles = (1ull << L.bits);

					// output files for this level
					std::vector < std::string> Vout(L.Vthreadint.size());
					// create output files
					for ( uint64_t t = 0; t < L.Vthreadint.size(); ++t )
						Vout[t] = tmpgen.getFileName(true) + ".unsort";

					// unsort
					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(unsortthreads)
					#endif
					for ( uint64_t t = 0; t < L.Vthreadint.size(); ++t )
					{
						// decoder pointer type
						typedef typename decoder_type::unique_ptr_type decoder_ptr_type;
						// decoder array
						libmaus2::autoarray::AutoArray < decoder_ptr_type > Adec(numinputfiles);
						// thread key interval
						std::pair<uint64_t,uint64_t> const & threadint = L.Vthreadint[t];

						// open input files
						for ( uint64_t i = 0; i < numinputfiles; ++i )
						{
							decoder_ptr_type Tdec(new decoder_type(Vin,L.Ohist[t*numinputfiles+i],1/* numthread*/));
							Adec[i] = UNIQUE_PTR_MOVE(Tdec);
						}

						// key decoder
						key_decoder_type Kdec(L.keyseqfn,threadint.first,1 /* numthread */);
						// output encoder
						encoder_type enc(Vout[t],std::numeric_limits<uint64_t>::max(),encbs);

						// number of elements still to do
						assert ( threadint.second >= threadint.first );
						uint64_t todo = threadint.second - threadint.first;
						typename key_decoder_type::run_type K;
						typename decoder_type::run_type R;

						while ( todo )
						{
							// decode key run
							bool const okK = Kdec.decodeRun(K);
							assert ( okK );
							// how much can we still use?
							uint64_t av = std::min(todo,K.rlen);
							// update todo
							todo -= av;

							// while we still have some of the key left
							while ( av )
							{
								// decode run from data file
								bool const okA = Adec[K.sym]->decodeRun(R);
								assert ( okA );

								// how much can we use of this run?
								uint64_t lav = std::min(av,R.rlen);

								// encode
								enc.encodeRun(typename decoder_type::run_type(R.sym,lav));
								// update av
								av -= lav;

								// we have used lav
								R.rlen -= lav;

								// put back rest
								if ( R.rlen )
								{
									Adec[K.sym]->putBack(R);
									assert ( !av );
								}
							}
						}
					}

					if ( deleteinput )
						for ( uint64_t i = 0; i < Vin.size(); ++i )
							libmaus2::aio::FileRemoval::removeFile(Vin[i]);

					deleteinput = true;
					Vin = Vout;
				}

				return Vin;
			}
		};
	}
}
#endif
