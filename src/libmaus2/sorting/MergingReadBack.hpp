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
#if !defined(LIBMAUS2_SORTING_MERGINGREADBACK_HPP)
#define LIBMAUS2_SORTING_MERGINGREADBACK_HPP

#include <libmaus2/LibMausConfig.hpp>
#if defined(LIBMAUS2_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <libmaus2/aio/BufferedOutput.hpp>
#include <libmaus2/aio/InputStreamFactoryContainer.hpp>
#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>
#include <libmaus2/aio/SynchronousGenericOutput.hpp>
#include <queue>

namespace libmaus2
{
	namespace sorting
	{
		template<typename _data_type, typename _order_type = std::less<_data_type> >
		struct MergingReadBack
		{
			typedef _data_type data_type;
			typedef _order_type order_type;
			typedef MergingReadBack<data_type,order_type> this_type;
			typedef typename libmaus2::util::unique_ptr<order_type>::type order_ptr_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;

			private:
			struct SubBlock
			{
				data_type * pa;
				data_type * pc;
				data_type * pe;

				SubBlock() : pa(0), pc(0), pe(0) {}
				SubBlock(data_type * const rpa, data_type * const rpc, data_type * const rpe) : pa(rpa), pc(rpc), pe(rpe) {}
			};

			struct HeapOrderAdapter
			{
				order_type * order;

				HeapOrderAdapter(order_type * rorder = 0) : order(rorder) {}

				bool operator()(std::pair<uint64_t, data_type> const & A, std::pair<uint64_t, data_type> const & B)
				{
					if ( A.second != B.second )
						return (*order)(B.second,A.second);
					else
						return A.first > B.first;
				}
			};

			libmaus2::aio::InputStream::unique_ptr_type PCIS;

			order_ptr_type Porder;
			order_type & order;
			HeapOrderAdapter HOA;
			std::priority_queue< std::pair<uint64_t,data_type>, std::vector< std::pair<uint64_t,data_type> >, HeapOrderAdapter > Q;

			std::vector<uint64_t> blocksizes;
			uint64_t const backblocksize;
			libmaus2::autoarray::AutoArray<uint64_t> blockoffsets;

			libmaus2::autoarray::AutoArray<data_type> blocks;
			libmaus2::autoarray::AutoArray<SubBlock> subblocks;

			bool fillBlock(uint64_t const b)
			{
				uint64_t const toread = std::min(blocksizes[b],backblocksize);

				if ( toread )
				{
					PCIS->clear();
					PCIS->seekg(blockoffsets[b] * sizeof(data_type));
					PCIS->read(reinterpret_cast<char *>(subblocks[b].pa),toread * sizeof(data_type));

					if ( PCIS->gcount() != static_cast<ssize_t>(toread * sizeof(data_type)) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "MergingReadBack::fillBlock: input failed to read " << toread << " elements from block " << b << std::endl;
						lme.finish();
						throw lme;
					}

					blockoffsets[b] += toread;
					blocksizes[b] -= toread;

					subblocks[b].pc = subblocks[b].pa;
					subblocks[b].pe = subblocks[b].pa + toread;
				}

				return toread;
			}

			bool getNext(uint64_t const b, data_type & v)
			{
				if ( subblocks[b].pc != subblocks[b].pe )
				{
					v = *(subblocks[b].pc++);
					return true;
				}
				else
				{
					bool const ok = fillBlock(b);

					if ( ! ok )
						return false;
					else
					{
						assert ( subblocks[b].pc != subblocks[b].pe );
						v = *(subblocks[b].pc++);
						return true;
					}
				}
			}

			public:
			MergingReadBack(std::string const & filename, std::vector<uint64_t> const & rblocksizes, uint64_t const rbackblocksize = 1024, uint64_t const blockshift = 0)
			:
				// input streams
				PCIS(libmaus2::aio::InputStreamFactoryContainer::constructUnique(filename)),
				// order pointer
				Porder(new order_type),
				// order
				order(*Porder),
				// heap order adapter
				HOA(&order),
				// block sizes
				blocksizes(rblocksizes),
				// read back block size
				backblocksize(rbackblocksize),
				// block offsets
				blockoffsets(blocksizes.size(),false),
				// block data
				blocks(backblocksize*blocksizes.size(),false),
				// sub block meta information (pointers)
				subblocks(blocksizes.size())
			{
				std::copy(blocksizes.begin(),blocksizes.end(),blockoffsets.begin());
				blockoffsets.prefixSums();
				for ( uint64_t i = 0; i < blockoffsets.size(); ++i )
					blockoffsets[i] += blockshift;

				for ( uint64_t i = 0; i < blocksizes.size(); ++i )
				{
					subblocks[i] = SubBlock(
						blocks.begin() + i * backblocksize,
						blocks.begin() + i * backblocksize,
						blocks.begin() + i * backblocksize
					);

					data_type v;
					bool const ok = getNext(i,v);
					if ( ok )
						Q.push(std::pair<uint64_t,data_type>(i,v));
				}
			}

			static std::vector<uint64_t> mergeStep(std::string const & filename, std::vector<uint64_t> const & blocksizes, uint64_t const maxfan = 16, uint64_t const rbackblocksize = 1024)
			{
				uint64_t const tnumpacks = (blocksizes.size() + maxfan - 1)/maxfan;
				uint64_t const packsize = (blocksizes.size() + tnumpacks -1)/tnumpacks;
				uint64_t const numpacks = (blocksizes.size() + packsize - 1)/packsize;
				uint64_t blockshift = 0;
				std::vector<uint64_t> oblocksizes(numpacks);
				std::string const ofilename = filename + "_merge";
				libmaus2::aio::OutputStreamInstance::unique_ptr_type OSI(new libmaus2::aio::OutputStreamInstance(ofilename));

				for ( uint64_t p = 0; p < numpacks; ++p )
				{
					uint64_t const p_low = p*packsize;
					uint64_t const p_high = std::min(p_low+packsize,static_cast<uint64_t>(blocksizes.size()));
					std::vector<uint64_t> const subblocksizes(blocksizes.begin()+p_low,blocksizes.begin()+p_high);
					MergingReadBack<data_type,order_type> MRB(filename,subblocksizes,rbackblocksize,blockshift);
					libmaus2::aio::SynchronousGenericOutput<data_type> Sout(*OSI,8*1024);
					data_type v;
					while ( MRB.getNext(v) )
						Sout.put(v);
					Sout.flush();
					uint64_t const blocksum = std::accumulate(subblocksizes.begin(),subblocksizes.end(),0ull);
					blockshift += blocksum;
					oblocksizes[p] = blocksum;
				}

				OSI->flush();
				OSI.reset();

				libmaus2::aio::OutputStreamFactoryContainer::rename(ofilename,filename);

				return oblocksizes;
			}

			static std::vector<uint64_t> premerge(std::string const & filename, std::vector<uint64_t> blocksizes, uint64_t const maxfan = 16, uint64_t const rbackblocksize = 1024)
			{
				while ( blocksizes.size() > maxfan )
					blocksizes = mergeStep(filename,blocksizes,maxfan,rbackblocksize);
				return blocksizes;
			}

			bool getNext(data_type & v)
			{
				if ( Q.size() )
				{
					v = Q.top().second;
					uint64_t const b = Q.top().first;
					Q.pop();

					data_type vn;

					bool const ok = getNext(b,vn);
					if ( ok )
						Q.push(std::pair<uint64_t,data_type>(b,vn));

					return true;
				}
				else
				{
					return false;
				}
			}
		};
	}
}
#endif
