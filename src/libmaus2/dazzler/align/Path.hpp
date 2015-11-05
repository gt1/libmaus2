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
#if ! defined(LIBMAUS_DAZZLER_ALIGN_PATH_HPP)
#define LIBMAUS_DAZZLER_ALIGN_PATH_HPP

#include <libmaus2/dazzler/db/InputBase.hpp>
#include <libmaus2/dazzler/db/OutputBase.hpp>
#include <utility>

namespace libmaus2
{
	namespace dazzler
	{
		namespace align
		{
			struct Path : public libmaus2::dazzler::db::InputBase, public libmaus2::dazzler::db::OutputBase
			{
				typedef std::pair<uint16_t,uint16_t> tracepoint;

				std::vector<tracepoint> path;
				int32_t tlen;
				int32_t diffs;
				int32_t abpos;
				int32_t bbpos;
				int32_t aepos;
				int32_t bepos;

				double getErrorRateA() const
				{
					return static_cast<double>(diffs) / (aepos-abpos);
				}

				double getErrorRateB() const
				{
					return static_cast<double>(diffs) / (bepos-bbpos);
				}

				bool operator==(Path const & O) const
				{
					return
						comparePathMeta(O) && path == O.path;
				}

				bool comparePathMeta(Path const & O) const
				{
					return
						tlen == O.tlen &&
						diffs == O.diffs &&
						abpos == O.abpos &&
						bbpos == O.bbpos &&
						aepos == O.aepos &&
						bepos == O.bepos;
				}

				bool comparePathMetaLower(Path const & O) const
				{
					return
						tlen == O.tlen &&
						diffs <= O.diffs &&
						abpos == O.abpos &&
						bbpos == O.bbpos &&
						aepos == O.aepos &&
						bepos == O.bepos;
				}

				uint64_t deserialise(std::istream & in)
				{
					uint64_t offset = 0;
					tlen = getLittleEndianInteger4(in,offset);
					diffs = getLittleEndianInteger4(in,offset);
					abpos = getLittleEndianInteger4(in,offset);
					bbpos = getLittleEndianInteger4(in,offset);
					aepos = getLittleEndianInteger4(in,offset);
					bepos = getLittleEndianInteger4(in,offset);
					return offset;
				}

				uint64_t serialise(std::ostream & out) const
				{
					uint64_t offset = 0;
					putLittleEndianInteger4(out,tlen,offset);
					putLittleEndianInteger4(out,diffs,offset);
					putLittleEndianInteger4(out,abpos,offset);
					putLittleEndianInteger4(out,bbpos,offset);
					putLittleEndianInteger4(out,aepos,offset);
					putLittleEndianInteger4(out,bepos,offset);
					return offset;
				}

				uint64_t serialiseWithPath(std::ostream & out, bool const small) const
				{
					uint64_t s = 0;
					s += serialise(out);
					s += serialisePath(out,small);
					return s;
				}

				uint64_t serialisePath(std::ostream & out, bool const small) const
				{
					uint64_t s = 0;
					if ( small )
					{
						for ( uint64_t i = 0; i < path.size(); ++i )
						{
							out.put(path[i].first);
							if ( ! out )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "Path::serialisePath: output error" << std::endl;
								lme.finish();
								throw lme;
							}
							s += 1;
							out.put(path[i].second);
							if ( ! out )
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "Path::serialisePath: output error" << std::endl;
								lme.finish();
								throw lme;
							}
							s += 1;
						}
						return s;
					}
					else
					{
						uint64_t offset = 0;

						for ( uint64_t i = 0; i < path.size(); ++i )
						{
							putLittleEndianInteger2(out,path[i].first,offset);
							putLittleEndianInteger2(out,path[i].second,offset);
						}

						return offset;
					}
				}

				Path()
				{

				}
				Path(std::istream & in)
				{
					deserialise(in);
				}
				Path(std::istream & in, uint64_t & s)
				{
					s += deserialise(in);
				}

				uint64_t getNumErrors() const
				{
					uint64_t s = 0;
					for ( uint64_t i = 0; i < path.size(); ++i )
						s += path[i].first;
					return s;
				}
			};

			std::ostream & operator<<(std::ostream & out, Path const & P);
		}
	}
}
#endif
