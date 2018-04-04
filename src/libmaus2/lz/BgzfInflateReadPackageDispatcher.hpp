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
#if !defined(LIBMAUS2_LZ_BGZFINFLATEREADPACKAGEDISPATCHER_HPP)
#define LIBMAUS2_LZ_BGZFINFLATEREADPACKAGEDISPATCHER_HPP

#include <libmaus2/parallel/SimpleThreadWorkPackageDispatcher.hpp>
#include <libmaus2/lz/BgzfInflateBaseTypeInfo.hpp>
#include <libmaus2/lz/BgzfInflateBaseAllocator.hpp>
#include <libmaus2/aio/InputStreamObjectTypeInfo.hpp>
#include <libmaus2/aio/InputStreamObjectAllocator.hpp>
#include <libmaus2/util/AutoArrayCharBaseTypeInfo.hpp>
#include <libmaus2/util/AutoArrayCharBaseAllocator.hpp>
#include <libmaus2/parallel/LockedFreeList.hpp>
#include <libmaus2/parallel/LockedGrowingFreeList.hpp>

namespace libmaus2
{
	namespace lz
	{
		struct BgzfInflateReadPackageDispatcher : public libmaus2::parallel::SimpleThreadWorkPackageDispatcher
		{
			typedef BgzfInflateReadPackageDispatcher this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			struct BgzfInflateReadPackageResult
			{
				libmaus2::lz::BgzfInflateBase::shared_ptr_type block;
				libmaus2::lz::BgzfInflateBase::BaseBlockInfo BBI;
				uint64_t streamid;
				uint64_t blockid;
				libmaus2::autoarray::AutoArray<char>::shared_ptr_type ablock;
				uint64_t ubytes;
				bool eof;

				BgzfInflateReadPackageResult() {}
				BgzfInflateReadPackageResult(
					libmaus2::lz::BgzfInflateBase::shared_ptr_type rblock,
					libmaus2::lz::BgzfInflateBase::BaseBlockInfo const rBBI,
					uint64_t const rstreamid,
					uint64_t const rblockid,
					libmaus2::autoarray::AutoArray<char>::shared_ptr_type rablock,
					bool const reof
				) : block(rblock), BBI(rBBI), streamid(rstreamid), blockid(rblockid), ablock(rablock), ubytes(0), eof(reof)
				{

				}

				bool operator<(BgzfInflateReadPackageResult const & O) const
				{
					return blockid < O.blockid;
				}
			};


			struct BgzfInflateReadPackage : public libmaus2::parallel::SimpleThreadWorkPackage
			{
				typedef BgzfInflateReadPackage this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				BgzfInflateReadPackage() : SimpleThreadWorkPackage() {}
				BgzfInflateReadPackage(
					uint64_t const rpriority,
					uint64_t const rdispatcherid,
					uint64_t const rpackageid = 0,
					uint64_t const rsubid = 0
				)
				: SimpleThreadWorkPackage(rpriority,rdispatcherid,rpackageid,rsubid) {}

				char const * getPackageName() const
				{
					return "BgzfInflateReadPackage";
				}
			};


			struct BgzfInflateReadPackageFinished
			{
				virtual ~BgzfInflateReadPackageFinished() {}
				virtual void bgzfInflateReadPackageFinished(BgzfInflateReadPackageResult ptr) = 0;
			};

			struct BgzfInflateReadPackageReturn
			{
				virtual ~BgzfInflateReadPackageReturn() {}
				virtual void bgzfInflateReadPackageReturn(BgzfInflateReadPackage * p) = 0;
			};

			libmaus2::parallel::LockedFreeList<
				libmaus2::aio::InputStreamObject::shared_ptr_type,
				libmaus2::aio::InputStreamObjectAllocator,
				libmaus2::aio::InputStreamObjectTypeInfo
			> * ifreelist;
			libmaus2::parallel::LockedFreeList<
				libmaus2::lz::BgzfInflateBase::shared_ptr_type,
				libmaus2::lz::BgzfInflateBaseAllocator,
				libmaus2::lz::BgzfInflateBaseTypeInfo
			> * zfreelist;

			libmaus2::parallel::LockedGrowingFreeList<
				libmaus2::autoarray::AutoArray<char>::shared_ptr_type,
				libmaus2::util::AutoArrayCharBaseAllocator,
				libmaus2::util::AutoArrayCharBaseTypeInfo
			> * afreelist;

			BgzfInflateReadPackageFinished * finished;
			BgzfInflateReadPackageReturn * packreturn;

			BgzfInflateReadPackageDispatcher(
				libmaus2::parallel::LockedFreeList<
					libmaus2::aio::InputStreamObject::shared_ptr_type,
					libmaus2::aio::InputStreamObjectAllocator,
					libmaus2::aio::InputStreamObjectTypeInfo
				> * rifreelist,
				libmaus2::parallel::LockedFreeList<
					libmaus2::lz::BgzfInflateBase::shared_ptr_type,
					libmaus2::lz::BgzfInflateBaseAllocator,
					libmaus2::lz::BgzfInflateBaseTypeInfo
				> * rzfreelist,
				libmaus2::parallel::LockedGrowingFreeList<
					libmaus2::autoarray::AutoArray<char>::shared_ptr_type,
					libmaus2::util::AutoArrayCharBaseAllocator,
					libmaus2::util::AutoArrayCharBaseTypeInfo
				> * rafreelist,
				BgzfInflateReadPackageFinished * rfinished,
				BgzfInflateReadPackageReturn * rpackreturn
			) : libmaus2::parallel::SimpleThreadWorkPackageDispatcher(), ifreelist(rifreelist), zfreelist(rzfreelist), afreelist(rafreelist), finished(rfinished), packreturn(rpackreturn) {}

			bool getResources(libmaus2::aio::InputStreamObject::shared_ptr_type & iptr, libmaus2::lz::BgzfInflateBase::shared_ptr_type & block)
			{
				iptr = ifreelist->getIf();

				if ( ! iptr )
				{
					return false;
				}

				block = zfreelist->getIf();

				if ( ! block )
				{
					ifreelist->put(iptr);
					return false;
				}

				return true;
			}

			void freeResources(libmaus2::aio::InputStreamObject::shared_ptr_type & iptr, libmaus2::lz::BgzfInflateBase::shared_ptr_type & block)
			{
				zfreelist->put(block);
				ifreelist->put(iptr);
			}

			void dispatch(libmaus2::parallel::SimpleThreadWorkPackage * P, libmaus2::parallel::SimpleThreadPoolInterfaceEnqueTermInterface & /* tpi */)
			{
				BgzfInflateReadPackage * BIRP = dynamic_cast<BgzfInflateReadPackage *>(P);
				assert ( BIRP );

				libmaus2::aio::InputStreamObject::shared_ptr_type iptr;
				libmaus2::lz::BgzfInflateBase::shared_ptr_type block;

				while ( getResources(iptr,block) )
				{
					if ( iptr->istr->peek() == std::istream::traits_type::eof() )
					{
						freeResources(iptr,block);

						if ( !iptr->eof )
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "[E] unexpected EOF in libmaus2:lz::BgzfInflateReadPackageDispatcher::dispatch" << std::endl;
							lme.finish();
							throw lme;
						}
						else
						{
							break;
						}
					}
					else
					{
						libmaus2::autoarray::AutoArray<char>::shared_ptr_type ablock = afreelist->get();
						libmaus2::lz::BgzfInflateBase::BaseBlockInfo const BBI = block->readBlock(*(iptr->istr));

						bool reporteof = (iptr->istr->peek() == std::istream::traits_type::eof());
						if ( reporteof )
							iptr->eof = true;

						BgzfInflateReadPackageResult result(block,BBI,iptr->streamid,iptr->blockid++,ablock,reporteof);
						ifreelist->put(iptr);
						finished->bgzfInflateReadPackageFinished(result);
					}
				}

				packreturn->bgzfInflateReadPackageReturn(BIRP);
			}
		};
	}
}
#endif
