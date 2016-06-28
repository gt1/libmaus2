/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_BAMBAM_READENDSBLOCKDECODERBASECOLLECTIONINFOBASE_HPP)
#define LIBMAUS2_BAMBAM_READENDSBLOCKDECODERBASECOLLECTIONINFOBASE_HPP

#include <string>
#include <vector>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/aio/OutputStreamFactoryContainer.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct ReadEndsBlockDecoderBaseCollectionInfoBase
		{
			typedef ReadEndsBlockDecoderBaseCollectionInfoBase this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string datafilename;
			std::string indexfilename;

			std::vector < uint64_t > blockelcnt;
			std::vector < uint64_t > indexoffset;

			void move()
			{
				std::string const mdatafilename =  datafilename + ".moved";
				std::string const mindexfilename =  indexfilename + ".moved";
				libmaus2::aio::OutputStreamFactoryContainer::rename(datafilename.c_str(),mdatafilename.c_str());
				libmaus2::aio::OutputStreamFactoryContainer::rename(indexfilename.c_str(),mindexfilename.c_str());
				datafilename = mdatafilename;
				indexfilename = mindexfilename;
			}

			void serialise(std::ostream & out) const
			{
				libmaus2::util::StringSerialisation::serialiseString(out,datafilename);
				libmaus2::util::StringSerialisation::serialiseString(out,indexfilename);
				libmaus2::util::NumberSerialisation::serialiseNumberVector(out,blockelcnt);
				libmaus2::util::NumberSerialisation::serialiseNumberVector(out,indexoffset);
			}

			void moveAndSerialise(std::ostream & out)
			{
				move();
				serialise(out);
			}

			void deserialise(std::istream & in)
			{
				datafilename = libmaus2::util::StringSerialisation::deserialiseString(in);
				indexfilename = libmaus2::util::StringSerialisation::deserialiseString(in);
				blockelcnt = libmaus2::util::NumberSerialisation::deserialiseNumberVector<uint64_t>(in);
				indexoffset = libmaus2::util::NumberSerialisation::deserialiseNumberVector<uint64_t>(in);
			}

			ReadEndsBlockDecoderBaseCollectionInfoBase(std::istream & in)
			{
				deserialise(in);
			}

			ReadEndsBlockDecoderBaseCollectionInfoBase()
			: datafilename(), indexfilename(), blockelcnt(), indexoffset()
			{

			}

			ReadEndsBlockDecoderBaseCollectionInfoBase(
				std::string const & rdatafilename,
				std::string const & rindexfilename,
				std::vector < uint64_t > const & rblockelcnt,
				std::vector < uint64_t > const & rindexoffset

			)
			: datafilename(rdatafilename), indexfilename(rindexfilename), blockelcnt(rblockelcnt), indexoffset(rindexoffset)
			{

			}

			ReadEndsBlockDecoderBaseCollectionInfoBase(
				ReadEndsBlockDecoderBaseCollectionInfoBase const & O
			) : datafilename(O.datafilename), indexfilename(O.indexfilename), blockelcnt(O.blockelcnt), indexoffset(O.indexoffset)
			{

			}
		};

		std::ostream & operator<<(std::ostream & out, ReadEndsBlockDecoderBaseCollectionInfoBase const & O)
		{
			out << "ReadEndsBlockDecoderBaseCollectionInfoBase(datafilename=" << O.datafilename << ",indexfilename=" << O.indexfilename << ",blockelcnt={";
			for ( uint64_t i = 0; i < O.blockelcnt.size(); ++i )
				out << O.blockelcnt[i] << ((i+1<O.blockelcnt.size()) ? ";" : "},indexoffset={");
			for ( uint64_t i = 0; i < O.indexoffset.size(); ++i )
				out << O.indexoffset[i] << ((i+1<O.indexoffset.size()) ? ";" : "})");
			return out;
		}
	}
}
#endif
