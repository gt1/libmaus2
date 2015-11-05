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
#if ! defined(LIBMAUS2_DAZZLER_DB_HITSINDEXBASE_HPP)
#define LIBMAUS2_DAZZLER_DB_HITSINDEXBASE_HPP

#include <libmaus2/dazzler/db/InputBase.hpp>
#include <libmaus2/dazzler/db/OutputBase.hpp>
#include <istream>

namespace libmaus2
{
	namespace dazzler
	{
		namespace db
		{
			struct IndexBase : public InputBase, public OutputBase
			{
				// untrimmed number of reads
				int32_t ureads;
				// trimmed number of reads
				int32_t treads;
				// cut off
				int32_t cutoff;
				// multiple reads from each well?
				bool all;
				// frequency of bases A,C,G and T
				float freq[4];
				// maximum read length
				int32_t maxlen;
				// total number of bases in database
				int64_t totlen;
				// number of reads
				int32_t nreads;
				// is database trimmed?
				bool trimmed;
				// part this record refers to (0 for all of database, >0 for partial)
				int part;
				// first untrimmed record index
				int32_t ufirst;
				// first trimmed record index
				int32_t tfirst;
				// path to db
				std::string path;
				// are records loaded into memory
				bool loaded;


				IndexBase()
				{

				}

				IndexBase(std::istream & in)
				{
					deserialise(in);
				}

				uint64_t serialise(std::ostream & out) const
				{
					uint64_t offset = 0;

					putLittleEndianInteger4(out,ureads,offset);
					putLittleEndianInteger4(out,treads,offset);
					putLittleEndianInteger4(out,cutoff,offset);
					putLittleEndianInteger4(out,all,offset);

					for ( size_t i = 0; i < sizeof(freq)/sizeof(freq[0]); ++i )
						putFloat(out,freq[i],offset);

					putLittleEndianInteger4(out,maxlen,offset);
					putLittleEndianInteger8(out,totlen,offset);
					putLittleEndianInteger4(out,nreads,offset);
					putLittleEndianInteger4(out,trimmed,offset);

					putLittleEndianInteger4(out,0,offset); // part
					putLittleEndianInteger4(out,0,offset); // ufirst
					putLittleEndianInteger4(out,0,offset); // tfirst

					putLittleEndianInteger8(out,0,offset); // path
					putLittleEndianInteger4(out,0,offset); // loaded
					putLittleEndianInteger8(out,0,offset); // bases
					putLittleEndianInteger8(out,0,offset); // reads
					putLittleEndianInteger8(out,0,offset); // tracks

					return offset;
				}

				void deserialise(std::istream & in)
				{
					uint64_t offset = 0;

					ureads  = getLittleEndianInteger4(in,offset); // number of reads in untrimmed database
					treads  = getLittleEndianInteger4(in,offset); // number of reads in trimmed database
					cutoff  = getLittleEndianInteger4(in,offset); // length cut off
					all     = getLittleEndianInteger4(in,offset); // keep all reads

					// symbol frequences
					for ( size_t i = 0; i < sizeof(freq)/sizeof(freq[0]); ++i )
						freq[i] = getFloat(in,offset);

					maxlen  = getLittleEndianInteger4(in,offset); // maximum length of a reads
					totlen  = getLittleEndianInteger8(in,offset); // total length
					/* nreads  = */ getLittleEndianInteger4(in,offset); // number of reads
					trimmed = getLittleEndianInteger4(in,offset);

					/* part = */   getLittleEndianInteger4(in,offset);
					/* ufirst = */ getLittleEndianInteger4(in,offset);
					/* tfirst = */ getLittleEndianInteger4(in,offset);

					/* path =   */ getLittleEndianInteger8(in,offset); // 8 byte pointer
					/* loaded = */ getLittleEndianInteger4(in,offset);
					/* bases =  */ getLittleEndianInteger8(in,offset); // 8 byte pointer

					/* reads =  */ getLittleEndianInteger8(in,offset); // 8 byte pointer
					/* tracks = */ getLittleEndianInteger8(in,offset); // 8 byte pointer

					nreads = ureads; /* number of untrimmed reads */
				}
			};

			std::ostream & operator<<(std::ostream & out, IndexBase const & H);
		}
	}
}
#endif
