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
#if ! defined(LIBMAUS2_GAMMA_GAMMAPDINDEXDECODER_HPP)
#define LIBMAUS2_GAMMA_GAMMAPDINDEXDECODER_HPP

#include <libmaus2/gamma/GammaPDIndexDecoderBase.hpp>
#include <libmaus2/util/iterator.hpp>
#include <libmaus2/util/PrefixSums.hpp>

namespace libmaus2
{
	namespace gamma
	{
		struct GammaPDIndexDecoder : public GammaPDIndexDecoderBase
		{
			struct FileBlockOffset
			{
				uint64_t file;
				uint64_t block;
				uint64_t blockoffset;
				uint64_t offset;

				FileBlockOffset(
					uint64_t const rfile = 0,
					uint64_t const rblock = 0,
					uint64_t const rblockoffset = 0,
					uint64_t const roffset = 0
				) : file(rfile), block(rblock), blockoffset(rblockoffset), offset(roffset) {}
			};

			std::vector<std::string> Vfn;
			std::vector<uint64_t> valuesPerFile;
			std::vector<uint64_t> blocksPerFile;
			std::vector<uint64_t> indexEntriesPerFile;
			std::vector<uint64_t> indexOffset;

			struct IndexAccessor
			{
				typedef libmaus2::util::ConstIterator< IndexAccessor,std::pair<uint64_t,uint64_t> > const_iterator;

				mutable libmaus2::aio::InputStreamInstance ISI;
				uint64_t const indexoffset;
				uint64_t const indexentries;

				IndexAccessor(std::string const & fn, uint64_t const rindexoffset, uint64_t const rindexentries)
				: ISI(fn), indexoffset(rindexoffset), indexentries(rindexentries)
				{

				}

				std::pair<uint64_t,uint64_t> get(uint64_t const i) const
				{
					ISI.clear();
					ISI.seekg(indexoffset + i*2*sizeof(uint64_t));
					std::pair<uint64_t,uint64_t> P;
					P.first = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					P.second = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					return P;
				}

				std::pair<uint64_t,uint64_t> operator[](uint64_t const i) const
				{
					return get(i);
				}

				const_iterator begin() const
				{
					return const_iterator(this,0);
				}

				const_iterator end() const
				{
					return const_iterator(this,indexentries);
				}
			};

			struct PairSecondComp
			{
				bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
				{
					return A.second < B.second;
				}
			};

			GammaPDIndexDecoder(std::vector<std::string> const & rVfn)
			: Vfn(rVfn), valuesPerFile(0), blocksPerFile(0), indexEntriesPerFile(0)
			{
				uint64_t o = 0;
				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					libmaus2::aio::InputStreamInstance ISI(Vfn[i]);
					uint64_t const vpf = getNumValues(ISI);

					if ( vpf )
					{
						valuesPerFile.push_back(vpf);
						blocksPerFile.push_back(getNumBlocks(ISI));
						indexEntriesPerFile.push_back(blocksPerFile.back()+1);
						indexOffset.push_back(getIndexOffset(ISI));
						Vfn[o++] = Vfn[i];
					}
				}
				// for prefix sum
				valuesPerFile.push_back(0);
				Vfn.resize(o);

				libmaus2::util::PrefixSums::prefixSums(valuesPerFile.begin(),valuesPerFile.end());
			}

			uint64_t size() const
			{
				return valuesPerFile.back();
			}

			FileBlockOffset lookup(uint64_t offset)
			{
				if ( offset >= valuesPerFile.back() )
					return FileBlockOffset(Vfn.size(),0,0);

				std::vector<uint64_t>::const_iterator it = ::std::lower_bound(valuesPerFile.begin(),valuesPerFile.end(),offset);
				assert ( it != valuesPerFile.end() );

				if ( *it != offset )
				{
					assert ( it != valuesPerFile.begin() );
					it -= 1;
				}

				assert ( offset >= *it );

				uint64_t const fileid = it - valuesPerFile.begin();
				offset -= *it;

				assert ( offset < valuesPerFile[fileid+1]-valuesPerFile[fileid] );

				IndexAccessor IA(Vfn[fileid],indexOffset[fileid],indexEntriesPerFile[fileid]);
				IndexAccessor::const_iterator fit = std::lower_bound(IA.begin(),IA.end(),std::pair<uint64_t,uint64_t>(0,offset),PairSecondComp());
				assert ( fit != IA.end() );


				if ( fit[0].second != offset )
					fit -= 1;

				assert ( offset >= fit[0].second );

				uint64_t const blockid = fit - IA.begin();
				uint64_t const blockoffset = fit[0].first;

				offset -= fit[0].second;

				// std::cerr << "fileid=" << fileid << " blockid=" << blockid << " blockoffset=" << blockoffset << " offset=" << offset << std::endl;

				return FileBlockOffset(fileid,blockid,blockoffset,offset);
			}
		};
	}
}
#endif
