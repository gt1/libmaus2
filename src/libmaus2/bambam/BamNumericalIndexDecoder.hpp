/*
    libmaus2
    Copyright (C) 2009-2016 German Tischler
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
#if ! defined(LIBMAUS2_BAMBAM_BAMNUMERICALINDEXDECODER_HPP)
#define LIBMAUS2_BAMBAM_BAMNUMERICALINDEXDECODER_HPP

#include <libmaus2/aio/InputStreamInstance.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/bambam/BamNumericalIndexBase.hpp>
#include <libmaus2/lz/BgzfInflate.hpp>
#include <libmaus2/bambam/BamAlignmentDecoder.hpp>
#include <libmaus2/lz/BgzfInflateFile.hpp>

namespace libmaus2
{
	namespace bambam
	{
		struct BamNumericalIndexDecoder : public libmaus2::bambam::BamNumericalIndexBase
		{
			typedef BamNumericalIndexDecoder this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			libmaus2::aio::InputStreamInstance::unique_ptr_type Pstream;
			std::istream & in;

			uint64_t const alcnt;
			uint64_t const mod;
			uint64_t const numblocks;

			public:
			BamNumericalIndexDecoder(std::string const & fn)
			: Pstream(new libmaus2::aio::InputStreamInstance(fn)), in(*Pstream),
			  alcnt(libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			  mod(libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
			  numblocks( (alcnt+mod-1)/mod )
			{
			
			}
			
			uint64_t size() const
			{
				return numblocks;
			}
			
			uint64_t getAlignmentCount() const
			{
				return alcnt;
			}
			
			uint64_t getBlockSize() const
			{
				return mod;
			}
			
			std::pair<uint64_t,uint64_t> operator[](uint64_t const i) const
			{
				in.clear();
				in.seekg(2*sizeof(uint64_t) + 2*sizeof(uint64_t)*i, std::ios::beg );
				uint64_t const fileoff = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				uint64_t const blockoff = libmaus2::util::NumberSerialisation::deserialiseNumber(in);
				return std::pair<uint64_t,uint64_t>(fileoff,blockoff);
			}
			
			libmaus2::lz::BgzfInflateFile::unique_ptr_type getStreamAt(std::string const & bamfn, uint64_t const id) const
			{
				libmaus2::bambam::BamAlignment algn;
				
				if ( id < getAlignmentCount() )
				{
					std::pair<uint64_t,uint64_t> const off = (*this)[id / mod];
					std::cerr << "off=" << off.first << "," << off.second << std::endl;
					libmaus2::lz::BgzfInflateFile::unique_ptr_type tptr(new libmaus2::lz::BgzfInflateFile(bamfn,off.first,off.second));
					uint64_t const m = id % mod;
					for ( uint64_t i = 0; i < m; ++i )
						libmaus2::bambam::BamAlignmentDecoder::readAlignmentGz(*tptr,algn);
					return UNIQUE_PTR_MOVE(tptr);
				}
				else
				{
					// file containing alignments
					if ( alcnt )
					{
						libmaus2::lz::BgzfInflateFile::unique_ptr_type tptr(getStreamAt(bamfn,alcnt-1));
						libmaus2::bambam::BamAlignmentDecoder::readAlignmentGz(*tptr,algn);
						return UNIQUE_PTR_MOVE(tptr);
					}
					// file contaning no alignments
					else
					{
						libmaus2::lz::BgzfInflateFile::unique_ptr_type tptr(new libmaus2::lz::BgzfInflateFile(bamfn,0,0));
						libmaus2::bambam::BamHeader header(*tptr);
						return UNIQUE_PTR_MOVE(tptr);
					}
				}
			}
		};
	}
}
#endif
