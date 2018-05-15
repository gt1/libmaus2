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
#if ! defined(LIBMAUS2_SORTING_SORTINGBUFFEREDOUTPUTFILE_HPP)
#define LIBMAUS2_SORTING_SORTINGBUFFEREDOUTPUTFILE_HPP

#include <libmaus2/sorting/MergingReadBack.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/util/FiniteSizeHeap.hpp>

namespace libmaus2
{
	namespace sorting
	{
		template<typename _data_type, typename _order_type = std::less<_data_type> >
		struct SortingBufferedOutputFile
		{
			typedef _data_type data_type;
			typedef _order_type order_type;
			typedef SortingBufferedOutputFile<data_type,order_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::sorting::MergingReadBack<data_type,order_type> merger_type;
			typedef typename libmaus2::sorting::MergingReadBack<data_type,order_type>::unique_ptr_type merger_ptr_type;

			std::string const filename;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type PCOS;
			typename libmaus2::aio::SortingBufferedOutput<data_type,order_type>::unique_ptr_type SBO;

			SortingBufferedOutputFile(std::string const & rfilename, uint64_t const bufsize = 1024ull)
			: filename(rfilename), PCOS(new libmaus2::aio::OutputStreamInstance(filename)), SBO(new libmaus2::aio::SortingBufferedOutput<data_type,order_type>(*PCOS,bufsize))
			{
			}

			void put(data_type v)
			{
				SBO->put(v);
			}

			merger_ptr_type getMerger(uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull)
			{
				SBO->flush();
				std::vector<uint64_t> blocksizes = SBO->getBlockSizes();
				SBO.reset();
				PCOS->flush();
				PCOS.reset();

				blocksizes = libmaus2::sorting::MergingReadBack<data_type,order_type>::premerge(filename,blocksizes,maxfan,backblocksize);

				typename libmaus2::sorting::MergingReadBack<data_type,order_type>::unique_ptr_type ptr(
					new libmaus2::sorting::MergingReadBack<data_type,order_type>(filename,blocksizes,backblocksize)
				);

				return UNIQUE_PTR_MOVE(ptr);
			}
		};

		template<typename _data_type, typename _order_type = std::less<_data_type> >
		struct SerialisingSortingBufferedOutputFile
		{
			typedef _data_type data_type;
			typedef _order_type order_type;
			typedef SerialisingSortingBufferedOutputFile<data_type,order_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef libmaus2::sorting::SerialisingMergingReadBack<data_type,order_type> merger_type;
			typedef typename libmaus2::sorting::SerialisingMergingReadBack<data_type,order_type>::unique_ptr_type merger_ptr_type;

			typedef typename libmaus2::util::unique_ptr<order_type>::type order_ptr_type;
			order_ptr_type Porder;
			order_type & order;

			std::string const filename;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type PCOS;
			typename libmaus2::aio::SerialisingSortingBufferedOutput<data_type,order_type>::unique_ptr_type SBO;

			struct IndexCallback
			{
				virtual ~IndexCallback() {}
				virtual void operator()(data_type const & D, uint64_t const pos) = 0;
			};

			SerialisingSortingBufferedOutputFile(
				std::string const & rfilename, uint64_t const bufsize = 1024ull, uint64_t const sortthreads = 1
			)
			: Porder(new order_type), order(*Porder), filename(rfilename), PCOS(new libmaus2::aio::OutputStreamInstance(filename)),
			  SBO(new libmaus2::aio::SerialisingSortingBufferedOutput<data_type,order_type>(*PCOS,bufsize,order,sortthreads))
			{
			}

			SerialisingSortingBufferedOutputFile(
				std::string const & rfilename, order_type & rorder, uint64_t const bufsize = 1024ull, uint64_t const sortthreads = 1
			)
			: Porder(), order(rorder), filename(rfilename), PCOS(new libmaus2::aio::OutputStreamInstance(filename)),
			  SBO(new libmaus2::aio::SerialisingSortingBufferedOutput<data_type,order_type>(*PCOS,bufsize,order,sortthreads))
			{
			}

			void put(data_type v)
			{
				SBO->put(v);
			}

			merger_ptr_type getMerger(uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull)
			{
				SBO->flush();
				std::vector< typename libmaus2::aio::SerialisingSortingBufferedOutput<data_type,order_type>::BlockDescriptor > blocksizes = SBO->getBlockSizes();
				SBO.reset();
				PCOS->flush();
				PCOS.reset();

				blocksizes = libmaus2::sorting::SerialisingMergingReadBack<data_type,order_type>::premerge(filename,blocksizes,order,maxfan,backblocksize);

				typename libmaus2::sorting::SerialisingMergingReadBack<data_type,order_type>::unique_ptr_type ptr(
					new libmaus2::sorting::SerialisingMergingReadBack<data_type,order_type>(filename,blocksizes,order,backblocksize)
				);

				return UNIQUE_PTR_MOVE(ptr);
			}

			static void sort(
				std::string const & fn,
				uint64_t const blocksize = 1024ull,
				uint64_t const backblocksize = 1024ull,
				uint64_t const maxfan = 16ull,
				uint64_t const sortthreads = 1ull
			)
			{
				std::string const tmpfn = fn + ".tmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfn);
				reduce(std::vector<std::string>(1,fn),tmpfn,blocksize,backblocksize,maxfan,sortthreads);
				libmaus2::aio::OutputStreamFactoryContainer::rename(tmpfn,fn);
			}

			static void sort(
				std::string const & fn,
				IndexCallback & indexer,
				uint64_t const blocksize = 1024ull,
				uint64_t const backblocksize = 1024ull,
				uint64_t const maxfan = 16ull,
				uint64_t const sortthreads = 1ull
			)
			{
				std::string const tmpfn = fn + ".tmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfn);
				reduce(std::vector<std::string>(1,fn),indexer,tmpfn,blocksize,backblocksize,maxfan,sortthreads);
				libmaus2::aio::OutputStreamFactoryContainer::rename(tmpfn,fn);
			}

			static void reduce(
				std::vector<std::string> const & Vfn,
				std::string const & out,
				uint64_t const blocksize = 1024ull,
				uint64_t const backblocksize = 1024ull,
				uint64_t const maxfan = 16ull,
				uint64_t const sortthreads = 1ull
			)
			{
				std::string const tmp = out + ".tmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(tmp);
				unique_ptr_type U(new this_type(tmp,blocksize,sortthreads));
				data_type D;

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					libmaus2::aio::InputStreamInstance ISI(Vfn[i]);
					while ( ISI && ISI.peek() != std::istream::traits_type::eof() )
					{
						D.deserialise(ISI);
						U->put(D);
					}
				}

				merger_ptr_type Pmerger(U->getMerger(backblocksize,maxfan));
				libmaus2::aio::OutputStreamInstance OSI(out);
				while ( Pmerger->getNext(D) )
					D.serialise(OSI);
				OSI.flush();
				Pmerger.reset();
				U.reset();

				libmaus2::aio::FileRemoval::removeFile(tmp);
			}

			static void reduce(
				std::vector<std::string> const & Vfn,
				IndexCallback & indexer,
				std::string const & out,
				uint64_t const blocksize = 1024ull,
				uint64_t const backblocksize = 1024ull,
				uint64_t const maxfan = 16ull,
				uint64_t const sortthreads = 1ull
			)
			{
				std::string const tmp = out + ".tmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(tmp);
				unique_ptr_type U(new this_type(tmp,blocksize,sortthreads));
				data_type D;

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					libmaus2::aio::InputStreamInstance ISI(Vfn[i]);
					while ( ISI && ISI.peek() != std::istream::traits_type::eof() )
					{
						D.deserialise(ISI);
						U->put(D);
					}
				}

				merger_ptr_type Pmerger(U->getMerger(backblocksize,maxfan));
				libmaus2::aio::OutputStreamInstance OSI(out);
				while ( Pmerger->getNext(D) )
				{
					indexer(D,OSI.tellp());
					D.serialise(OSI);
				}
				OSI.flush();
				Pmerger.reset();
				U.reset();

				libmaus2::aio::FileRemoval::removeFile(tmp);
			}

			static void sortUnique(
				std::string const & fn,
				uint64_t const blocksize = 1024ull,
				uint64_t const backblocksize = 1024ull,
				uint64_t const maxfan = 16ull,
				uint64_t const sortthreads = 1ull
			)
			{
				std::string const tmpfn = fn + ".tmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfn);
				reduceUnique(std::vector<std::string>(1,fn),tmpfn,blocksize,backblocksize,maxfan,sortthreads);
				libmaus2::aio::OutputStreamFactoryContainer::rename(tmpfn,fn);
			}

			static void sortUnique(
				std::string const & fn,
				IndexCallback & indexer,
				uint64_t const blocksize = 1024ull,
				uint64_t const backblocksize = 1024ull,
				uint64_t const maxfan = 16ull,
				uint64_t const sortthreads = 1ull
			)
			{
				std::string const tmpfn = fn + ".tmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(tmpfn);
				reduceUnique(std::vector<std::string>(1,fn),indexer,tmpfn,blocksize,backblocksize,maxfan,sortthreads);
				libmaus2::aio::OutputStreamFactoryContainer::rename(tmpfn,fn);
			}

			static void reduceUnique(
				std::vector<std::string> const & Vfn,
				std::string const & out,
				uint64_t const blocksize = 1024ull,
				uint64_t const backblocksize = 1024ull,
				uint64_t const maxfan = 16ull,
				uint64_t const sortthreads = 1ull
			)
			{
				std::string const tmp = out + ".tmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(tmp);
				unique_ptr_type U(new this_type(tmp,blocksize,sortthreads));
				data_type D;

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					libmaus2::aio::InputStreamInstance ISI(Vfn[i]);
					while ( ISI && ISI.peek() != std::istream::traits_type::eof() )
					{
						D.deserialise(ISI);
						U->put(D);
					}
				}

				merger_ptr_type Pmerger(U->getMerger(backblocksize,maxfan));
				libmaus2::aio::OutputStreamInstance OSI(out);

				data_type Dprev;
				order_type order;
				if ( Pmerger->getNext(Dprev) )
				{
					while ( Pmerger->getNext(D) )
					{
						if ( order(Dprev,D) )
						{
							Dprev.serialise(OSI);
							Dprev = D;
						}
					}

					Dprev.serialise(OSI);
				}
				OSI.flush();
				Pmerger.reset();
				U.reset();

				libmaus2::aio::FileRemoval::removeFile(tmp);
			}

			static void reduceUnique(
				std::vector<std::string> const & Vfn,
				IndexCallback & indexer,
				std::string const & out,
				uint64_t const blocksize = 1024ull,
				uint64_t const backblocksize = 1024ull,
				uint64_t const maxfan = 16ull,
				uint64_t const sortthreads = 1ull
			)
			{
				std::string const tmp = out + ".tmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(tmp);
				unique_ptr_type U(new this_type(tmp,blocksize,sortthreads));
				data_type D;

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					libmaus2::aio::InputStreamInstance ISI(Vfn[i]);
					while ( ISI && ISI.peek() != std::istream::traits_type::eof() )
					{
						D.deserialise(ISI);
						U->put(D);
					}
				}

				merger_ptr_type Pmerger(U->getMerger(backblocksize,maxfan));
				libmaus2::aio::OutputStreamInstance OSI(out);

				data_type Dprev;
				order_type order;
				if ( Pmerger->getNext(Dprev) )
				{
					while ( Pmerger->getNext(D) )
					{
						if ( order(Dprev,D) )
						{
							indexer(Dprev,OSI.tellp());
							Dprev.serialise(OSI);
							Dprev = D;
						}
					}

					indexer(Dprev,OSI.tellp());
					Dprev.serialise(OSI);
				}
				OSI.flush();
				Pmerger.reset();
				U.reset();

				libmaus2::aio::FileRemoval::removeFile(tmp);
			}
		};

		template<typename _data_type, typename _order_type = std::less<_data_type> >
		struct SerialisingSortingBufferedOutputFileArray
		{
			typedef _data_type data_type;
			typedef _order_type order_type;
			typedef SerialisingSortingBufferedOutputFileArray<data_type,order_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::vector < std::string > Vfn;

			typedef typename libmaus2::util::unique_ptr<order_type>::type order_ptr_type;
			order_ptr_type Porder;
			order_type & order;

			std::string const filename;

			typedef SerialisingSortingBufferedOutputFile<data_type,order_type> sorter_type;
			typedef typename sorter_type::unique_ptr_type sorter_ptr_type;
			libmaus2::autoarray::AutoArray < sorter_ptr_type > ASO;

			static std::vector < std::string > computeVfn(std::string const & s, uint64_t const n)
			{
				std::vector < std::string > Vfn(n);
				for ( uint64_t i = 0; i < n; ++i )
				{
					std::ostringstream ostr;
					ostr << s << "_" << std::setw(6) << std::setfill('0') << i;
					Vfn[i] = ostr.str();
				}
				return Vfn;
			}

			void setupFileArray(uint64_t const bufsize)
			{
				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					sorter_ptr_type tptr(
						new sorter_type(Vfn[i],order,bufsize)
					);
					ASO[i] = UNIQUE_PTR_MOVE(tptr);
				}
			}

			SerialisingSortingBufferedOutputFileArray(std::string const & rfilename, uint64_t const n, uint64_t const bufsize = 1024ull)
			: Vfn(computeVfn(rfilename,n)), Porder(new order_type), order(*Porder), filename(rfilename), ASO(n)
			{
				setupFileArray(bufsize);
			}

			SerialisingSortingBufferedOutputFileArray(std::string const & rfilename, uint64_t const n, order_type & rorder, uint64_t const bufsize = 1024ull)
			: Vfn(computeVfn(rfilename,n)), Porder(), order(rorder), filename(rfilename), ASO(n)
			{
				setupFileArray(bufsize);
			}

			sorter_type & operator[](uint64_t const i)
			{
				assert ( i < ASO.size() );
				assert ( ASO[i] );
				return *(ASO[i]);
			}

			struct IndexCallback
			{
				virtual ~IndexCallback() {}
				virtual void operator()(data_type const & D, uint64_t const pos) = 0;
			};

			struct Merger
			{
				typedef Merger this_type;
				typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
				typedef _data_type data_type;

				struct MergeObject
				{
					data_type D;
					uint64_t i;

					MergeObject()
					{}

					MergeObject(
						data_type const & rD,
						uint64_t const ri
					) : D(rD), i(ri) {}
				};

				struct MergeObjectComparator
				{
					order_type & order;

					MergeObjectComparator(order_type & rorder)
					: order(rorder) {}

					bool operator()(MergeObject const & A, MergeObject const & B) const
					{
						if ( order(A.D,B.D) )
							return true;
						else if ( order(B.D,A.D) )
							return false;
						else
							return A.i < B.i;
					}
				};

				std::vector<std::string> const Vfn;
				libmaus2::autoarray::AutoArray < typename sorter_type::merger_ptr_type > AME;
				MergeObjectComparator comp;
				libmaus2::util::FiniteSizeHeap < MergeObject, MergeObjectComparator > FSE;

				Merger(
					libmaus2::autoarray::AutoArray < sorter_ptr_type > & ASO,
					std::vector<std::string> const & rVfn,
					order_type & order,
					uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull
				)
				: Vfn(rVfn), AME(Vfn.size()), comp(order), FSE(Vfn.size(),comp)
				{
					for ( uint64_t i = 0; i < Vfn.size(); ++i )
					{
						typename sorter_type::merger_ptr_type tptr(ASO[i]->getMerger(backblocksize,maxfan));
						AME[i] = UNIQUE_PTR_MOVE(tptr);

						data_type D;
						if ( AME[i]->getNext(D) )
						{
							FSE.push(MergeObject(D,i));
						}
						else
						{
							AME[i].reset();
							libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
						}
					}
				}

				Merger(
					uint64_t const numthreads,
					libmaus2::autoarray::AutoArray < sorter_ptr_type > & ASO,
					std::vector<std::string> const & rVfn,
					order_type & order,
					uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull
				)
				: Vfn(rVfn), AME(Vfn.size()), comp(order), FSE(Vfn.size(),comp)
				{
					libmaus2::parallel::PosixSpinLock lock;

					#if defined(_OPENMP)
					#pragma omp parallel for schedule(dynamic,1) num_threads(numthreads)
					#endif
					for ( uint64_t i = 0; i < Vfn.size(); ++i )
					{
						typename sorter_type::merger_ptr_type tptr(ASO[i]->getMerger(backblocksize,maxfan));
						AME[i] = UNIQUE_PTR_MOVE(tptr);

						data_type D;
						if ( AME[i]->getNext(D) )
						{
							libmaus2::parallel::ScopePosixSpinLock slock(lock);
							FSE.push(MergeObject(D,i));
						}
						else
						{
							AME[i].reset();
							libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
						}
					}
				}

				bool getNext(data_type & D)
				{
					if ( !FSE.empty() )
					{
						MergeObject M = FSE.pop();

						D = M.D;

						if ( AME[M.i]->getNext(M.D) )
						{
							FSE.push(M);
						}
						else
						{
							AME[M.i].reset();
							libmaus2::aio::FileRemoval::removeFile(Vfn[M.i]);
						}

						return true;
					}
					else
					{
						return false;
					}
				}
			};

			typedef Merger merger_type;
			typedef typename merger_type::unique_ptr_type merger_ptr_type;

			merger_ptr_type getMerger(uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull)
			{
				merger_ptr_type mpt(
					new merger_type(ASO,Vfn,order,backblocksize,maxfan)
				);

				return UNIQUE_PTR_MOVE(mpt);
			}

			merger_ptr_type getMergerParallel(uint64_t const numthreads, uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull)
			{
				merger_ptr_type mpt(
					new merger_type(numthreads,ASO,Vfn,order,backblocksize,maxfan)
				);

				return UNIQUE_PTR_MOVE(mpt);
			}

			std::string reduce(uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull)
			{
				struct MergeObject
				{
					data_type D;
					uint64_t i;

					MergeObject()
					{}

					MergeObject(
						data_type const & rD,
						uint64_t const ri
					) : D(rD), i(ri) {}
				};

				struct MergeObjectComparator
				{
					order_type & order;

					MergeObjectComparator(order_type & rorder)
					: order(rorder) {}

					bool operator()(MergeObject const & A, MergeObject const & B) const
					{
						if ( order(A.D,B.D) )
							return true;
						else if ( order(B.D,A.D) )
							return false;
						else
							return A.i < B.i;
					}
				};

				libmaus2::autoarray::AutoArray < typename sorter_type::merger_ptr_type > AME(Vfn.size());
				MergeObjectComparator comp(order);
				libmaus2::util::FiniteSizeHeap < MergeObject, MergeObjectComparator > FSE(Vfn.size(),comp);

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					typename sorter_type::merger_ptr_type tptr(ASO[i]->getMerger(backblocksize,maxfan));
					AME[i] = UNIQUE_PTR_MOVE(tptr);

					data_type D;
					if ( AME[i]->getNext(D) )
					{
						FSE.push(MergeObject(D,i));
					}
					else
					{
						AME[i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
					}
				}

				libmaus2::aio::OutputStreamInstance OSI(filename);
				while ( !FSE.empty() )
				{
					MergeObject M = FSE.pop();

					M.D.serialise(OSI);

					if ( AME[M.i]->getNext(M.D) )
					{
						FSE.push(M);
					}
					else
					{
						AME[M.i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[M.i]);
					}
				}
				OSI.flush();

				return filename;

			}

			std::string reduceUnique(uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull)
			{
				struct MergeObject
				{
					data_type D;
					uint64_t i;

					MergeObject()
					{}

					MergeObject(
						data_type const & rD,
						uint64_t const ri
					) : D(rD), i(ri) {}
				};

				struct MergeObjectComparator
				{
					order_type & order;

					MergeObjectComparator(order_type & rorder)
					: order(rorder) {}

					bool operator()(MergeObject const & A, MergeObject const & B) const
					{
						if ( order(A.D,B.D) )
							return true;
						else if ( order(B.D,A.D) )
							return false;
						else
							return A.i < B.i;
					}
				};

				libmaus2::autoarray::AutoArray < typename sorter_type::merger_ptr_type > AME(Vfn.size());
				MergeObjectComparator comp(order);
				libmaus2::util::FiniteSizeHeap < MergeObject, MergeObjectComparator > FSE(Vfn.size(),comp);

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					typename sorter_type::merger_ptr_type tptr(ASO[i]->getMerger(backblocksize,maxfan));
					AME[i] = UNIQUE_PTR_MOVE(tptr);

					data_type D;
					if ( AME[i]->getNext(D) )
					{
						FSE.push(MergeObject(D,i));
					}
					else
					{
						AME[i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
					}
				}

				libmaus2::aio::OutputStreamInstance OSI(filename);
				bool prevvalid = false;
				data_type prevD;

				while ( !FSE.empty() )
				{
					MergeObject M = FSE.pop();

					if ( (! prevvalid) || order(prevD,M.D) )
						M.D.serialise(OSI);

					prevD = M.D;
					prevvalid = true;

					if ( AME[M.i]->getNext(M.D) )
					{
						FSE.push(M);
					}
					else
					{
						AME[M.i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[M.i]);
					}
				}
				OSI.flush();

				return filename;

			}

			std::string reduceParallel(uint64_t const numthreads, uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull)
			{
				struct MergeObject
				{
					data_type D;
					uint64_t i;

					MergeObject()
					{}

					MergeObject(
						data_type const & rD,
						uint64_t const ri
					) : D(rD), i(ri) {}
				};

				struct MergeObjectComparator
				{
					order_type & order;

					MergeObjectComparator(order_type & rorder)
					: order(rorder) {}

					bool operator()(MergeObject const & A, MergeObject const & B) const
					{
						if ( order(A.D,B.D) )
							return true;
						else if ( order(B.D,A.D) )
							return false;
						else
							return A.i < B.i;
					}
				};

				libmaus2::autoarray::AutoArray < typename sorter_type::merger_ptr_type > AME(Vfn.size());
				MergeObjectComparator comp(order);
				libmaus2::util::FiniteSizeHeap < MergeObject, MergeObjectComparator > FSE(Vfn.size(),comp);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
				#endif
				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					typename sorter_type::merger_ptr_type tptr(ASO[i]->getMerger(backblocksize,maxfan));
					AME[i] = UNIQUE_PTR_MOVE(tptr);
				}

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					data_type D;
					if ( AME[i]->getNext(D) )
					{
						FSE.push(MergeObject(D,i));
					}
					else
					{
						AME[i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
					}
				}

				libmaus2::aio::OutputStreamInstance OSI(filename);
				while ( !FSE.empty() )
				{
					MergeObject M = FSE.pop();

					M.D.serialise(OSI);

					if ( AME[M.i]->getNext(M.D) )
					{
						FSE.push(M);
					}
					else
					{
						AME[M.i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[M.i]);
					}
				}
				OSI.flush();

				return filename;

			}

			std::string reduceUniqueParallel(uint64_t const numthreads, uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull)
			{
				struct MergeObject
				{
					data_type D;
					uint64_t i;

					MergeObject()
					{}

					MergeObject(
						data_type const & rD,
						uint64_t const ri
					) : D(rD), i(ri) {}
				};

				struct MergeObjectComparator
				{
					order_type & order;

					MergeObjectComparator(order_type & rorder)
					: order(rorder) {}

					bool operator()(MergeObject const & A, MergeObject const & B) const
					{
						if ( order(A.D,B.D) )
							return true;
						else if ( order(B.D,A.D) )
							return false;
						else
							return A.i < B.i;
					}
				};

				libmaus2::autoarray::AutoArray < typename sorter_type::merger_ptr_type > AME(Vfn.size());
				MergeObjectComparator comp(order);
				libmaus2::util::FiniteSizeHeap < MergeObject, MergeObjectComparator > FSE(Vfn.size(),comp);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
				#endif
				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					typename sorter_type::merger_ptr_type tptr(ASO[i]->getMerger(backblocksize,maxfan));
					AME[i] = UNIQUE_PTR_MOVE(tptr);
				}

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					data_type D;
					if ( AME[i]->getNext(D) )
					{
						FSE.push(MergeObject(D,i));
					}
					else
					{
						AME[i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
					}
				}

				bool prevvalid = false;
				data_type prevD;

				libmaus2::aio::OutputStreamInstance OSI(filename);
				while ( !FSE.empty() )
				{
					MergeObject M = FSE.pop();

					if ( (! prevvalid) || order(prevD,M.D) )
						M.D.serialise(OSI);

					prevD = M.D;
					prevvalid = true;

					if ( AME[M.i]->getNext(M.D) )
					{
						FSE.push(M);
					}
					else
					{
						AME[M.i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[M.i]);
					}
				}
				OSI.flush();

				return filename;

			}

			std::string reduce(IndexCallback & indexer, uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull)
			{
				libmaus2::autoarray::AutoArray < typename sorter_type::merger_ptr_type > AME(Vfn.size());

				struct MergeObject
				{
					data_type D;
					uint64_t i;

					MergeObject() {}

					MergeObject(data_type const rD, uint64_t const ri)
					: D(rD), i(ri) {}

					MergeObject(MergeObject const & O)
					: D(O.D), i(O.i) {}

					MergeObject & operator=(MergeObject const & O)
					{
						D = O.D;
						i = O.i;
						return *this;
					}
				};

				struct MergeObjectComparator
				{
					order_type & order;

					MergeObjectComparator(order_type & rorder)
					: order(rorder) {}

					bool operator()(MergeObject const & A, MergeObject const & B) const
					{
						if ( order(A.D,B.D) )
							return true;
						else if ( order(B.D,A.D) )
							return false;
						else
							return A.i < B.i;
					}
				};

				MergeObjectComparator comp(order);
				libmaus2::util::FiniteSizeHeap < MergeObject, MergeObjectComparator > FSE(Vfn.size(),comp);

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					typename sorter_type::merger_ptr_type tptr(ASO[i]->getMerger(backblocksize,maxfan));
					AME[i] = UNIQUE_PTR_MOVE(tptr);
				}

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					data_type D;
					if ( AME[i]->getNext(D) )
					{
						assert ( ! FSE.full() );
						FSE.push(MergeObject(D,i));
					}
					else
					{
						AME[i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
					}
				}

				libmaus2::aio::OutputStreamInstance OSI(filename);
				while ( !FSE.empty() )
				{
					MergeObject M = FSE.pop();

					uint64_t const p = OSI.tellp();
					indexer(M.D,p);

					M.D.serialise(OSI);

					if ( AME[M.i]->getNext(M.D) )
					{
						assert ( ! FSE.full() );
						FSE.push(M);
					}
					else
					{
						AME[M.i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[M.i]);
					}
				}
				OSI.flush();

				return filename;

			}

			std::string reduceParallel(uint64_t const numthreads, IndexCallback & indexer, uint64_t const backblocksize = 1024ull, uint64_t const maxfan = 16ull)
			{
				libmaus2::autoarray::AutoArray < typename sorter_type::merger_ptr_type > AME(Vfn.size());

				struct MergeObject
				{
					data_type D;
					uint64_t i;

					MergeObject() {}

					MergeObject(data_type const rD, uint64_t const ri)
					: D(rD), i(ri) {}

					MergeObject(MergeObject const & O)
					: D(O.D), i(O.i) {}

					MergeObject & operator=(MergeObject const & O)
					{
						D = O.D;
						i = O.i;
						return *this;
					}
				};

				struct MergeObjectComparator
				{
					order_type & order;

					MergeObjectComparator(order_type & rorder)
					: order(rorder) {}

					bool operator()(MergeObject const & A, MergeObject const & B) const
					{
						if ( order(A.D,B.D) )
							return true;
						else if ( order(B.D,A.D) )
							return false;
						else
							return A.i < B.i;
					}
				};

				MergeObjectComparator comp(order);
				libmaus2::util::FiniteSizeHeap < MergeObject, MergeObjectComparator > FSE(Vfn.size(),comp);

				#if defined(_OPENMP)
				#pragma omp parallel for num_threads(numthreads) schedule(dynamic,1)
				#endif
				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					typename sorter_type::merger_ptr_type tptr(ASO[i]->getMerger(backblocksize,maxfan));
					AME[i] = UNIQUE_PTR_MOVE(tptr);
				}

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					data_type D;
					if ( AME[i]->getNext(D) )
					{
						assert ( ! FSE.full() );
						FSE.push(MergeObject(D,i));
					}
					else
					{
						AME[i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
					}
				}

				libmaus2::aio::OutputStreamInstance OSI(filename);
				while ( !FSE.empty() )
				{
					MergeObject M = FSE.pop();

					uint64_t const p = OSI.tellp();
					indexer(M.D,p);

					M.D.serialise(OSI);

					if ( AME[M.i]->getNext(M.D) )
					{
						assert ( ! FSE.full() );
						FSE.push(M);
					}
					else
					{
						AME[M.i].reset();
						libmaus2::aio::FileRemoval::removeFile(Vfn[M.i]);
					}
				}
				OSI.flush();

				return filename;

			}
		};
	}
}
#endif
