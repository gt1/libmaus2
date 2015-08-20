/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS2_WAVELET_RLTOHWTTERMREQUEST_HPP)
#define LIBMAUS2_WAVELET_RLTOHWTTERMREQUEST_HPP

#include <libmaus2/wavelet/RlToHwtBase.hpp>

namespace libmaus2
{
	namespace wavelet
	{
		struct RlToHwtTermRequest
		{
			typedef RlToHwtTermRequest this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			
			std::vector<std::string> bwt;
			std::string hwt;
			std::string tmpprefix;
			std::string huftreefilename;
			uint64_t bwtterm;
			uint64_t p0r;
			bool utf8;
			
			RlToHwtTermRequest() {}
			RlToHwtTermRequest(
				std::vector<std::string> const & rbwt,
				std::string const & rhwt,
				std::string const & rtmpprefix,
				std::string const & rhuftreefilename,
				uint64_t const rbwtterm,
				uint64_t const rp0r,
				bool const rutf8
			) : bwt(rbwt), hwt(rhwt), tmpprefix(rtmpprefix), huftreefilename(rhuftreefilename), bwtterm(rbwtterm), p0r(rp0r), utf8(rutf8) {}
			
			RlToHwtTermRequest(std::istream & in)
			:
				bwt(libmaus2::util::StringSerialisation::deserialiseStringVector(in)),
				hwt(libmaus2::util::StringSerialisation::deserialiseString(in)),
				tmpprefix(libmaus2::util::StringSerialisation::deserialiseString(in)),
				huftreefilename(libmaus2::util::StringSerialisation::deserialiseString(in)),
				bwtterm(libmaus2::util::NumberSerialisation::deserialiseSignedNumber(in)),
				p0r(libmaus2::util::NumberSerialisation::deserialiseNumber(in)),
				utf8(libmaus2::util::NumberSerialisation::deserialiseNumber(in))
			{
			
			}
			
			static unique_ptr_type load(std::string const & filename)
			{
				libmaus2::aio::InputStreamInstance CIS(filename);
				unique_ptr_type ptr(new this_type(CIS));
				return UNIQUE_PTR_MOVE(ptr);
			}
			
			std::ostream & serialise(std::ostream & out) const
			{
				libmaus2::util::StringSerialisation::serialiseStringVector(out,bwt);
				libmaus2::util::StringSerialisation::serialiseString(out,hwt);
				libmaus2::util::StringSerialisation::serialiseString(out,tmpprefix);
				libmaus2::util::StringSerialisation::serialiseString(out,huftreefilename);
				libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,bwtterm);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,p0r);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,utf8);
				return out;
			}

			static std::ostream & serialise(
				std::ostream & out,
				std::vector<std::string> const & bwt,
				std::string const & hwt,
				std::string const & tmpprefix,
				std::string const & huftreefilename,
				uint64_t const bwtterm,
				uint64_t const p0r,
				bool const utf8
			)
			{
				libmaus2::util::StringSerialisation::serialiseStringVector(out,bwt);
				libmaus2::util::StringSerialisation::serialiseString(out,hwt);
				libmaus2::util::StringSerialisation::serialiseString(out,tmpprefix);
				libmaus2::util::StringSerialisation::serialiseString(out,huftreefilename);
				libmaus2::util::NumberSerialisation::serialiseSignedNumber(out,bwtterm);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,p0r);
				libmaus2::util::NumberSerialisation::serialiseNumber(out,utf8);
				return out;
			}

			template<typename rl_decoder>
			libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type dispatch()
			{
				if ( utf8 )
				{
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tptr(
						libmaus2::wavelet::RlToHwtBase<true,rl_decoder>::rlToHwtTerm(bwt,hwt,tmpprefix,huftreefilename,bwtterm,p0r)
					);
					return UNIQUE_PTR_MOVE(tptr);
				}
				else
				{
					libmaus2::wavelet::ImpCompactHuffmanWaveletTree::unique_ptr_type tptr(
						libmaus2::wavelet::RlToHwtBase<false,rl_decoder>::rlToHwtTerm(bwt,hwt,tmpprefix,huftreefilename,bwtterm,p0r)
					);
					return UNIQUE_PTR_MOVE(tptr);
				}
			}
		};
	}
}
#endif
