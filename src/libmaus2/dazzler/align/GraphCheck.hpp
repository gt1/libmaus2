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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_GRAPHCHECK_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_GRAPHCHECK_HPP

#include <libmaus2/dazzler/align/GraphDecoder.hpp>
#include <libmaus2/dazzler/align/OverlapIndexer.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct GraphCheck
			{
				static bool checkEncodedGraph(std::vector<std::string> const & Vin, std::string const & graphfn, std::ostream & errOSI)
				{
					// open graph file descriptor
					libmaus2::aio::InputStreamInstance graphISI(graphfn);

					// initialize graph decoder
					libmaus2::dazzler::align::GraphDecoder GD(graphISI);

					bool ok = true;

					errOSI << "[V] Checking encoded graph..." << std::flush;
					for ( uint64_t i = 0; ok && i < Vin.size(); ++i )
					{
						libmaus2::dazzler::align::AlignmentFileRegion::unique_ptr_type Plas(libmaus2::dazzler::align::OverlapIndexer::openAlignmentFileWithoutIndex(Vin[i]));
						libmaus2::dazzler::align::Overlap OVL;
						libmaus2::dazzler::align::GraphDecoderContext::shared_ptr_type scontext_a = GD.getContext();
						libmaus2::dazzler::align::GraphDecoderContext & context_a = *scontext_a;

						while ( ok && Plas->peekNextOverlap(OVL) )
						{
							int64_t const aread = OVL.aread;
							uint64_t z = 0;
							GD.decode(graphISI,aread,context_a);

							for ( ; ok && Plas->peekNextOverlap(OVL) && (OVL.aread == aread); ++z )
							{
								if ( z < context_a.size() )
								{
									ok = ok && ( OVL.getHeader() == context_a[z] );
									Plas->getNextOverlap(OVL);
								}
								else
								{
									errOSI << "[E] alignment " << OVL.getHeader() << " missing from encoded graph" << std::endl;
									ok = false;
								}
							}

							while ( z < context_a.size() )
							{
								errOSI << "[E] alignment " << context_a[z++] << " is in graph but not in input file" << std::endl;
								ok = false;
							}
						}

						GD.returnContext(scontext_a);
					}
					errOSI << "done, result " << (ok?"ok":"FAILED") << std::endl;

					return ok;
				}
			};
		}
	}
}
#endif
