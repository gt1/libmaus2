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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_LASRANGEDECODER_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_LASRANGEDECODER_HPP

#include <libmaus2/dazzler/align/BinIndexDecoder.hpp>
#include <libmaus2/dazzler/align/AlignmentFile.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct LasRangeDecoder
			{
				typedef LasRangeDecoder this_type;
				typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
				typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

				std::string const fn;
				libmaus2::dazzler::align::BinIndexDecoder::unique_ptr_type const pBID;
				libmaus2::dazzler::align::BinIndexDecoder const & BID;

				uint64_t abpos;
				uint64_t aepos;
				libmaus2::aio::InputStreamInstance::unique_ptr_type PISI;
				libmaus2::dazzler::align::AlignmentFile::unique_ptr_type PAF;

				libmaus2::dazzler::align::Overlap peekslot;
				bool peekslotused;

				LasRangeDecoder(std::string const & rfn)
				: fn(rfn), pBID(new libmaus2::dazzler::align::BinIndexDecoder(fn)), BID(*pBID), abpos(0), aepos(0), PISI(), PAF(), peekslot(), peekslotused(false) {}

				LasRangeDecoder(std::string const & rfn, uint64_t const rl, uint64_t const rabpos, uint64_t const raepos)
				: fn(rfn), pBID(new libmaus2::dazzler::align::BinIndexDecoder(fn)), BID(*pBID), abpos(0), aepos(0), PISI(), PAF(), peekslot(), peekslotused(false) { setup(rl,rabpos,raepos); }

				LasRangeDecoder(std::string const & rfn, libmaus2::dazzler::align::BinIndexDecoder const & rBID)
				: fn(rfn), pBID(), BID(rBID), abpos(0), aepos(0), PISI(), PAF(), peekslot(), peekslotused(false) {}

				void setup(uint64_t const rl , uint64_t const rabpos, uint64_t const raepos)
				{
					peekslotused = false;
					abpos = rabpos;
					aepos = raepos;

					libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > A;
					libmaus2::autoarray::AutoArray< std::pair<uint64_t,uint64_t> > B;

					libmaus2::aio::InputStreamInstance::unique_ptr_type TISI(
						new libmaus2::aio::InputStreamInstance(fn)
					);
					PISI = UNIQUE_PTR_MOVE(TISI);

					std::istream & ISI = *PISI;

					libmaus2::dazzler::align::AlignmentFile::unique_ptr_type TAF(
						new libmaus2::dazzler::align::AlignmentFile(ISI)
					);
					PAF = UNIQUE_PTR_MOVE(TAF);

					uint64_t const o = libmaus2::dazzler::align::Path::getBinList(A,rl,abpos,aepos);
					uint64_t const n = BID.decodeBinList(0,B);

					ISI.clear();
					uint64_t minp = libmaus2::util::GetFileSize::getFileSize(ISI);

					struct PairFirstComparator
					{
						bool operator()(std::pair<uint64_t,uint64_t> const & A, std::pair<uint64_t,uint64_t> const & B) const
						{
							return A.first < B.first;
						}
					};

					uint64_t mul = 1;
					uint64_t vrl = rl;
					uint64_t off = 0;
					for ( uint64_t i = 0; i < o; ++i )
					{
						uint64_t const binlow = A[i].first;
						uint64_t const binhigh = A[i].second;


						std::pair<uint64_t,uint64_t> const * P = ::std::lower_bound(
							B.begin(),B.begin()+n,std::pair<uint64_t,uint64_t>(binlow,0),PairFirstComparator()
						);

						uint64_t q = 0;
						for ( ; P < B.begin() + n && P->first < binhigh; ++P, ++q )
							minp = std::min(minp,P->second);

						//std::cerr << "[binlow,binhigh)=[" << (binlow-off)*mul << "," << (binhigh-off)*mul << ") num " << q << std::endl;

						mul *= 2;
						off += vrl;
						vrl = (vrl+1)/2;
					}

					ISI.clear();
					ISI.seekg(minp);
				}

				bool getNext(libmaus2::dazzler::align::Overlap & OVL)
				{
					if ( peekslotused )
					{
						OVL = peekslot;
						peekslotused = false;
						return true;
					}
					else
					{
						while (
							PISI->peek() != std::istream::traits_type::eof()
							&&
							PAF->getNextOverlap(*PISI,OVL)
							&&
							OVL.path.abpos < static_cast<int64_t>(aepos)
						)
							if ( OVL.path.aepos >= static_cast<int64_t>(abpos) )
								return true;

						return false;
					}
				}

				bool peekNext(libmaus2::dazzler::align::Overlap & OVL)
				{
					if ( ! peekslotused )
						peekslotused = getNext(peekslot);

					OVL = peekslot;
					return peekslotused;
				}
			};
		}
	}
}
#endif
