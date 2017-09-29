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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTFILESYMMETRYCHECK_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_ALIGNMENTFILESYMMETRYCHECK_HPP

#include <libmaus2/dazzler/db/DatabaseFile.hpp>
#include <libmaus2/dazzler/align/OverlapIndexer.hpp>
#include <libmaus2/aio/SerialisedPeeker.hpp>
#include <libmaus2/sorting/SortingBufferedOutputFile.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct AlignmentFileSymmetryCheck
			{
				static int checkSymmetry(std::string const & dbfn, std::vector<std::string> const & Vin, std::string const & tmpfilebase, std::ostream * vstream = 0)
				{
					std::string const symforfn = tmpfilebase + "ovl.for.sym";
					std::string const syminvfn = tmpfilebase + "ovl.inv.sym";

					libmaus2::util::TempFileRemovalContainer::addTempFile(symforfn);
					libmaus2::util::TempFileRemovalContainer::addTempFile(syminvfn);

					libmaus2::aio::OutputStreamInstance::unique_ptr_type Psymfor(
						new libmaus2::aio::OutputStreamInstance(symforfn)
					);
					libmaus2::aio::OutputStreamInstance::unique_ptr_type Psyminv(
						new libmaus2::aio::OutputStreamInstance(syminvfn)
					);

					libmaus2::dazzler::db::DatabaseFile DB(dbfn);
					DB.computeTrimVector();

					std::vector<uint64_t> RL;
					DB.getAllReadLengths(RL);

					for ( uint64_t i = 0; i < Vin.size(); ++i )
					{
						std::string const lasfn = Vin[i];
						libmaus2::dazzler::align::AlignmentFileRegion::unique_ptr_type afile(libmaus2::dazzler::align::OverlapIndexer::openAlignmentFileWithoutIndex(lasfn));

						libmaus2::dazzler::align::Overlap OVL;
						uint64_t c = 0;
						while ( afile->getNextOverlap(OVL) )
						{
							OVL.getHeader().getInfo().swappedStraight(RL.begin()).serialise(*Psyminv);
							OVL.getHeader().getInfo().serialise(*Psymfor);

							if ( (++c % (1024*1024)) == 0 )
							{
								if ( vstream )
									*vstream << "[V] " << c << std::endl;
							}
						}
					}

					Psyminv->flush();
					Psyminv.reset();
					Psymfor->flush();
					Psymfor.reset();

					libmaus2::sorting::SerialisingSortingBufferedOutputFile<libmaus2::dazzler::align::OverlapInfo>::sort(symforfn,16*1024*1024);
					libmaus2::sorting::SerialisingSortingBufferedOutputFile<libmaus2::dazzler::align::OverlapInfo>::sort(syminvfn,16*1024*1024);

					libmaus2::aio::SerialisedPeeker<libmaus2::dazzler::align::OverlapInfo> peekerfor(symforfn);
					libmaus2::aio::SerialisedPeeker<libmaus2::dazzler::align::OverlapInfo> peekerinv(syminvfn);
					bool ok = true;

					libmaus2::dazzler::align::OverlapInfo infofor;
					libmaus2::dazzler::align::OverlapInfo infoinv;
					while ( peekerinv.peekNext(infoinv) && peekerfor.peekNext(infofor) )
					{
						if ( infoinv < infofor )
						{
							if ( vstream )
								*vstream << "[E1] missing " << infoinv.swapped() << std::endl;
							peekerinv.getNext(infoinv);
							ok = false;
						}
						else if ( infofor < infoinv )
						{
							if ( vstream )
								*vstream << "[E2] missing " << infofor.swapped() << std::endl;
							peekerfor.getNext(infofor);
							ok = false;
						}
						else
						{
							peekerinv.getNext(infoinv);
							peekerfor.getNext(infofor);
						}
					}

					if ( ok )
						return EXIT_SUCCESS;
					else
						return EXIT_FAILURE;
				}
			};
		}
	}
}
#endif
