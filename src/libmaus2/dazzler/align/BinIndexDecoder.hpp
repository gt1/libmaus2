/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_BININDEXDECODER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_BININDEXDECODER_HPP

#include <libmaus2/dazzler/align/OverlapIndexer.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct BinIndexDecoder
			{
				typedef BinIndexDecoder this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::string const indexfn;
				std::vector< std::pair<uint64_t,uint64_t> > const T;

				struct PairFirstComparator
				{
					bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
					{
						return A.first < B.first;
					}
				};

				uint64_t decodeBinList(uint64_t const id, libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > & A) const
				{
					typedef std::vector< std::pair<uint64_t,uint64_t> >::const_iterator it;
					std::pair<it,it> const P = std::equal_range(T.begin(),T.end(),std::pair<uint64_t,uint64_t>(id,0),PairFirstComparator());

					if ( P.second == P.first )
						return 0;

					assert ( P.second-P.first == 1 );

					libmaus2::aio::InputStreamInstance ISI(indexfn);

					ISI.clear();
					ISI.seekg(P.first->second,std::ios::beg);

					uint64_t const rid = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
					assert ( rid == id );
					uint64_t const n = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);

					uint64_t o = 0;
					for ( uint64_t i = 0; i < n; ++i )
					{
						uint64_t const a = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
						uint64_t const b = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
						A.push(o,
							std::pair<uint64_t,uint64_t>(a,b)
						);
					}

					return n;
				}

				std::pair<uint64_t,uint64_t> getTableInfo(std::istream & istr) const
				{
					istr.clear();
					istr.seekg(
						- static_cast<int64_t>(sizeof(uint64_t)),
						std::ios::end
					);

					uint64_t const nrefseq = libmaus2::util::NumberSerialisation::deserialiseNumber(istr);

					uint64_t s = libmaus2::util::GetFileSize::getFileSize(istr);

					s -= sizeof(uint64_t);
					s -= nrefseq * 2 * sizeof(uint64_t);

					return std::pair<uint64_t,uint64_t>(s,nrefseq);
				}

				std::vector< std::pair<uint64_t,uint64_t> > getTable() const
				{
					libmaus2::aio::InputStreamInstance ISI(indexfn);
					std::pair<uint64_t,uint64_t> const P = getTableInfo(ISI);

					ISI.clear();
					ISI.seekg(P.first,std::ios::beg);

					std::vector< std::pair<uint64_t,uint64_t> > V;

					for ( uint64_t i = 0; i < P.second; ++i )
					{
						uint64_t const a = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
						uint64_t const b = libmaus2::util::NumberSerialisation::deserialiseNumber(ISI);
						V.push_back(std::pair<uint64_t,uint64_t>(a,b));
					}

					return V;
				}

				BinIndexDecoder(std::string const lasfn)
				: indexfn(libmaus2::dazzler::align::OverlapIndexer::getBinIndexName(lasfn)), T(getTable())
				{
				}
			};
		}
	}
}
#endif
