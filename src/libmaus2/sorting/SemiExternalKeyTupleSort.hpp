/*
    libmaus2
    Copyright (C) 2015 German Tischler

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
#if ! defined(LIBMAUS2_SORTING_SEMIEXTERNALKEYTUPLESORT_HPP)
#define LIBMAUS2_SORTING_SEMIEXTERNALKEYTUPLESORT_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/math/ilog.hpp>
#include <libmaus2/math/numbits.hpp>
#include <libmaus2/math/lowbits.hpp>
#include <libmaus2/aio/ConcatInputStream.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <libmaus2/sorting/InPlaceParallelSort.hpp>

#include <iomanip>

namespace libmaus2
{
	namespace sorting
	{
		struct SemiExternalKeyTupleSort
		{
			// sorting for tuples with a key field
			// the keys are assumed to be uniformly distributed in [0,n)
			template<typename data_type, typename key_projector_type, typename output_data_type, typename output_projector_type>
			static void sort(std::vector<std::string> infn, std::string const & tmpbase, std::ostream & finalout, uint64_t const n, uint64_t tnumthreads, uint64_t const maxfiles, uint64_t const maxmem, bool removeinput = true)
			{
				// get number of input elements
				libmaus2::aio::ConcatInputStream::unique_ptr_type Psizecheckin(new libmaus2::aio::ConcatInputStream(infn));
				Psizecheckin->seekg(0,std::ios::end);
				std::streampos const numinbytes = Psizecheckin->tellg();
				assert ( numinbytes % sizeof(data_type) == 0 );
				uint64_t const numin = numinbytes / sizeof(data_type);
				Psizecheckin.reset();

				uint64_t maxblocksize = numin;
				uint64_t const maxinmem = (maxmem + sizeof(data_type)-1)/sizeof(data_type);
				if ( ! maxinmem )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "SemiExternalKeyTupleSort<>::sort(): maxinmem parameter is too small" << std::endl;
					lme.finish();
					throw lme;
				}

				uint64_t const maxradixthreads = maxfiles / 3; // one input, two output

				if ( (! maxradixthreads) && (numin > maxinmem) )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "SemiExternalKeyTupleSort<>::sort(): invalid parameters for sorting" << std::endl;
					lme.finish();
					throw lme;
				}

				tnumthreads = std::max(static_cast<uint64_t>(1),std::min(tnumthreads,maxradixthreads));

				std::cerr << "[V] SemiExternalKeyTupleSort::sort: sorting with " << tnumthreads << " threads" << std::endl;

				// tmp file id
				uint64_t tmpid = 0;

				// number of available open output files
				assert ( (maxinmem >= numin) || (maxfiles >= 3*tnumthreads) );
				uint64_t const numavoutputfiles = (maxfiles >= 3*tnumthreads) ? (maxfiles-tnumthreads) : 0;
				assert ( (maxinmem >= numin) || (numavoutputfiles >= 2*tnumthreads) );

				// maximum number of open output files per thread
				uint64_t filesperthread = numavoutputfiles / tnumthreads;

				// make sure this is at least 1
				if ( ! filesperthread )
					filesperthread = 1;

				// if it is not a power of two then make it the next lower one
				if ( libmaus2::math::nextTwoPow(filesperthread) != filesperthread )
					filesperthread = libmaus2::math::nextTwoPow(filesperthread)/2;

				assert ( filesperthread != 0 );

				// number of bits for filesperthread
				unsigned int const filebits = libmaus2::math::ilog(filesperthread);
				// mask
				uint64_t const filemask = libmaus2::math::lowbits(filebits);
				// number of bits for upper bound n
				unsigned int const numnbits = libmaus2::math::numbits(n);

				unsigned int radixruns = 0;
				unsigned int numradixbits = 0;
				while ( maxblocksize > maxinmem && numradixbits < numnbits )
				{
					maxblocksize >>= filebits;
					radixruns += 1;
					numradixbits += filebits;
				}

				uint64_t const packsize = (numin + tnumthreads-1)/tnumthreads;
				uint64_t const numthreads = (numin + packsize-1)/packsize;
				uint64_t const numfiles = numthreads * filesperthread;
				unsigned int const radixbits = std::min(numnbits,radixruns*filebits);
				unsigned int const nonradixbits = numnbits - radixbits;

				std::cerr << "[V] filesperthread=" << filesperthread << " filebits=" << filebits << " numnbits=" << numnbits << " radixruns=" << radixruns << " filemask=" << filemask << " radixbits=" << radixbits << " nonradixbits=" << nonradixbits << std::endl;

				for ( uint64_t r = 0; r < radixruns; ++r )
				{
					unsigned int const rightshift = ((radixruns-r)*filebits <= numnbits) ? (numnbits - (radixruns-r)*filebits) : 0;
					std::cerr << "[V] radix run " << r << " rightshift=" << rightshift << std::endl;

					std::vector<std::string> outfn(numfiles);
					// file 0 thread 0
					// file 0 thread 1
					// ...
					// file 1 thread 0
					// ...
					// id: fileid * numthreads + threadid
					for ( uint64_t i = 0; i < numfiles; ++i )
					{
						std::ostringstream tmpfnostr;
						tmpfnostr << tmpbase << "_" << std::setw(6) << std::setfill('0') << tmpid++;
						outfn.at(i) = tmpfnostr.str();
						libmaus2::util::TempFileRemovalContainer::addTempFile(outfn.at(i));
					}

					#if defined(_OPENMP)
					#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
					#endif
					for ( uint64_t t = 0; t < numthreads; ++t )
					{
						uint64_t const packlow = t*packsize;
						uint64_t const packhigh = std::min(packlow+packsize,numin);

						libmaus2::autoarray::AutoArray<libmaus2::aio::OutputStreamInstance::unique_ptr_type> Aout(filesperthread);
						libmaus2::autoarray::AutoArray<typename libmaus2::aio::SynchronousGenericOutput<data_type>::unique_ptr_type> Sout(filesperthread);
						for ( uint64_t i = 0; i < Aout.size(); ++i )
						{
							libmaus2::aio::OutputStreamInstance::unique_ptr_type Tptr(new libmaus2::aio::OutputStreamInstance(outfn.at(i*numthreads+t)));
							Aout.at(i) = UNIQUE_PTR_MOVE(Tptr);
							typename libmaus2::aio::SynchronousGenericOutput<data_type>::unique_ptr_type Sptr(new typename libmaus2::aio::SynchronousGenericOutput<data_type>(*(Aout.at(i)),8*1024));
							Sout.at(i) = UNIQUE_PTR_MOVE(Sptr);
						}

						libmaus2::aio::ConcatInputStream::unique_ptr_type Pin(new libmaus2::aio::ConcatInputStream(infn));
						Pin->seekg(packlow * sizeof(data_type));
						typename libmaus2::aio::SynchronousGenericInput<data_type>::unique_ptr_type Sin(new libmaus2::aio::SynchronousGenericInput<data_type>(*Pin,16*1024,packhigh-packlow));

						data_type D;
						while ( Sin->getNext(D) )
						{
							uint64_t const key = key_projector_type::project(D);
							uint64_t const file = (key >> rightshift)&filemask;
							Sout.at(file)->put(D);
						}

						for ( uint64_t i = 0; i < Aout.size(); ++i )
						{
							Sout.at(i)->flush();
							Sout.at(i).reset();
							Aout.at(i)->flush();
							Aout.at(i).reset();
						}
					}

					if ( removeinput )
						for ( uint64_t i = 0; i < infn.size(); ++i )
							libmaus2::aio::FileRemoval::removeFile(infn.at(i));

					infn = outfn;
					removeinput = true;
				}

				libmaus2::aio::ConcatInputStream conc(infn);

				uint64_t low = 0;
				uint64_t end = numin;
				libmaus2::autoarray::AutoArray<data_type> AS;
				typename libmaus2::aio::SynchronousGenericOutput<output_data_type>::unique_ptr_type Sout(new libmaus2::aio::SynchronousGenericOutput<output_data_type>(finalout,64*1024));
				while ( low < end )
				{
					conc.clear();
					conc.seekg(low*sizeof(data_type));

					// count number of elements in block
					typename libmaus2::aio::SynchronousGenericInput<data_type>::unique_ptr_type TSin(new libmaus2::aio::SynchronousGenericInput<data_type>(conc,8*1024));
					uint64_t snum = 1;
					data_type pv;
					#if ! defined(NDEBUG)
					bool const pvok =
					#endif
						TSin->getNext(pv);
					#if !defined(NDEBUG)
					assert ( pvok );
					#endif

					data_type tv;
					while ( TSin->getNext(tv) && (key_projector_type::project(tv)>>nonradixbits)==(key_projector_type::project(pv)>>nonradixbits) )
						++snum;

					TSin.reset();

					conc.clear();
					conc.seekg(low*sizeof(data_type));

					assert ( conc.tellg() == static_cast<std::streampos>(low*sizeof(data_type)) );
					if ( AS.size() < snum )
					{
						AS = libmaus2::autoarray::AutoArray<data_type>();
						AS = libmaus2::autoarray::AutoArray<data_type>(snum);
					}
					conc.read(reinterpret_cast<char *>(AS.begin()),snum*sizeof(data_type));
					assert ( conc.gcount() == static_cast<int64_t>(snum*sizeof(data_type)) );

					if ( nonradixbits )
					{
						libmaus2::sorting::InPlaceParallelSort::inplacesort2(AS.begin(),AS.begin()+snum,tnumthreads);
						// std::sort(AS.begin(),AS.begin()+snum);
					}

					if ( snum )
					{
						Sout->put(output_projector_type::project(AS.at(0)));
						uint64_t prev = key_projector_type::project(AS.at(0));

						// check for dups
						for ( uint64_t i = 1; i < snum; ++i )
							if ( key_projector_type::project(AS.at(i)) != prev )
							{
								Sout->put(output_projector_type::project(AS.at(i)));
								prev = key_projector_type::project(AS.at(i));
							}
					}

					low += snum;
				}

				Sout->flush();
				Sout.reset();

				if ( removeinput )
					for ( uint64_t i = 0; i < infn.size(); ++i )
						libmaus2::aio::FileRemoval::removeFile(infn.at(i));
			}
		};
	}
}
#endif
