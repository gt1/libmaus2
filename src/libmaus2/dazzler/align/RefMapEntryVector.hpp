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
#if ! defined(LIBMAUS2_DAZZLER_ALIGN_REFMAPENTRYVECTOR_HPP)
#define LIBMAUS2_DAZZLER_ALIGN_REFMAPENTRYVECTOR_HPP

#include <libmaus2/dazzler/align/RefMapEntry.hpp>
#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/util/GetFileSize.hpp>
#include <libmaus2/fastx/FastAReader.hpp>
#include <libmaus2/dazzler/db/DatabaseFile.hpp>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct RefMapEntryVector : public std::vector < RefMapEntry >
			{
				RefMapEntryVector()
				{
				}

				RefMapEntryVector(std::string const & fn)
				{
					deserialise(fn);
				}

				RefMapEntryVector(libmaus2::dazzler::db::DatabaseFile const & DB)
				{
					std::vector<libmaus2::dazzler::db::Read> V;
					DB.getReadInterval(0, DB.indexbase.nreads, V);
					int64_t oid = -1;

					for ( uint64_t i = 0; i < V.size(); ++i )
					{
						if ( ! V[i].origin )
							++oid;

						assert ( oid >= 0 );

						// origin,fpulse,rlen
						std::vector < RefMapEntry >::push_back(
							RefMapEntry(static_cast<uint64_t>(oid), V[i].fpulse)
						);
					}
				}

				bool operator==(RefMapEntryVector const & R) const
				{
					if ( std::vector < RefMapEntry >::size() != R.size() )
						return false;

					for ( uint64_t i = 0; i < std::vector < RefMapEntry >::size(); ++i )
						if ( ! ((*this)[i] == R[i]) )
							return false;

					return true;
				}

				std::ostream & serialise(std::ostream & out) const
				{
					libmaus2::util::NumberSerialisation::serialiseNumber(out,std::vector < RefMapEntry >::size());
					for ( uint64_t i = 0; i < std::vector < RefMapEntry >::size(); ++i )
						std::vector < RefMapEntry >::at(i).serialise(out);
					return out;
				}

				void serialise(std::string const & fn) const
				{
					libmaus2::aio::OutputStreamInstance OSI(fn);
					serialise(OSI);
				}

				void deserialise(std::istream & in)
				{
					uint64_t const n = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
					std::vector < RefMapEntry >::resize(0);
					for ( uint64_t i = 0; i < n; ++i )
						std::vector < RefMapEntry >::push_back(RefMapEntry(in));
				}

				void deserialise(std::string const & fn)
				{
					libmaus2::aio::InputStreamInstance ISI(fn);
					deserialise(ISI);
				}

				static RefMapEntryVector computeRefSplitMap(std::string const reffn)
				{
					std::string const refmapfn = reffn + ".refmap";

					if (
						(! libmaus2::util::GetFileSize::fileExists(refmapfn))
						||
						(libmaus2::util::GetFileSize::isOlder(refmapfn,reffn))
					)
					{
						libmaus2::fastx::FastAReader FA(reffn);
						libmaus2::fastx::FastAReader::pattern_type pattern;
						RefMapEntryVector V;

						uint64_t outid = 0;
						for ( uint64_t inid = 0; FA.getNextPatternUnlocked(pattern); ++inid )
						{
							std::string const spat = pattern.spattern;
							uint64_t l = 0;

							while ( l < spat.size() )
							{
								uint64_t h = l;
								while (
									h < spat.size() &&
									libmaus2::fastx::mapChar(spat[h]) >= 4
								)
									++h;

								l = h;

								if ( l < spat.size() )
								{
									while (
										h < spat.size() &&
										libmaus2::fastx::mapChar(spat[h]) < 4
									)
										++h;

									assert ( outid == V.size() );

									RefMapEntry E(inid,l);
									V.push_back(E);

									assert ( h > l );

									l = h;
									outid += 1;
								}
							}
						}

						V.serialise(refmapfn);

						return V;
					}
					else
					{
						return RefMapEntryVector(refmapfn);
					}

				}

				template<typename db_type>
				static RefMapEntryVector computeRefSplitMap(std::string const reffn, db_type const & DB)
				{
					std::string const refmapfn = reffn + ".refmap";

					if (
						(! libmaus2::util::GetFileSize::fileExists(refmapfn))
						||
						(libmaus2::util::GetFileSize::isOlder(refmapfn,reffn))
					)
					{
						libmaus2::fastx::FastAReader FA(reffn);
						libmaus2::fastx::FastAReader::pattern_type pattern;
						RefMapEntryVector V;

						// id in untrimmed database
						uint64_t outid = 0;
						for ( uint64_t inid = 0; FA.getNextPatternUnlocked(pattern); ++inid )
						{
							std::string const spat = pattern.spattern;
							// std::cerr << "[V] processing " << pattern.sid << std::endl;
							uint64_t l = 0;

							while ( l < spat.size() )
							{
								uint64_t h = l;
								while (
									h < spat.size() &&
									libmaus2::fastx::mapChar(spat[h]) >= 4
								)
									++h;

								l = h;

								if ( l < spat.size() )
								{
									while (
										h < spat.size() &&
										libmaus2::fastx::mapChar(spat[h]) < 4
									)
										++h;

									if ( DB.isInTrimmed(outid) )
									{
										RefMapEntry E(inid,l);
										V.push_back(E);
									}

									assert ( h > l );

									l = h;
									outid += 1;
								}
							}
						}

						V.serialise(refmapfn);

						return V;
					}
					else
					{
						return RefMapEntryVector(refmapfn);
					}

				}

				template<typename db_type>
				static RefMapEntryVector computeRefSplitMapN(std::string const reffn, db_type const & DB)
				{
					std::string const refmapfn = reffn + ".refmap";

					if (
						(! libmaus2::util::GetFileSize::fileExists(refmapfn))
						||
						(libmaus2::util::GetFileSize::isOlder(refmapfn,reffn))
					)
					{
						libmaus2::fastx::FastAReader FA(reffn);
						libmaus2::fastx::FastAReader::pattern_type pattern;
						RefMapEntryVector V;

						// id in untrimmed database
						uint64_t outid = 0;
						for ( uint64_t inid = 0; FA.getNextPatternUnlocked(pattern); ++inid )
						{
							std::string const spat = pattern.spattern;
							// std::cerr << "[V] processing " << pattern.sid << std::endl;
							uint64_t l = 0;

							while ( l < spat.size() )
							{
								uint64_t h = l;
								while (
									h < spat.size() &&
									(
										spat[h] == 'n'
										||
										spat[h] == 'N'
									)
								)
									++h;

								l = h;

								if ( l < spat.size() )
								{
									while (
										h < spat.size() &&
										(
											spat[h] != 'n'
											&&
											spat[h] != 'N'
										)
									)
										++h;

									if ( DB.isInTrimmed(outid) )
									{
										RefMapEntry E(inid,l);
										V.push_back(E);
									}

									assert ( h > l );

									l = h;
									outid += 1;
								}
							}
						}

						V.serialise(refmapfn);

						return V;
					}
					else
					{
						return RefMapEntryVector(refmapfn);
					}

				}

				std::ostream & toString(std::ostream & out) const
				{
					out << "RefMapEntryVector(";
					for ( uint64_t i = 0; i < std::vector < RefMapEntry >::size(); ++i )
						std::vector < RefMapEntry >::operator[](i).toString(out);
					out << ")";
					return out;
				}

				std::string toString() const
				{
					std::ostringstream ostr;
					toString(ostr);
					return ostr.str();
				}

			};
		}
	}
}
#endif
