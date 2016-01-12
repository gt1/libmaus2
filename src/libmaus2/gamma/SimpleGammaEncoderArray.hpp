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
#if ! defined(LIBMAUS2_GAMMA_SIMPLEGAMMAENCODERARRAY_HPP)
#define LIBMAUS2_GAMMA_SIMPLEGAMMAENCODERARRAY_HPP

#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/gamma/SimpleGammaEncoder.hpp>

namespace libmaus2
{
	namespace gamma
	{
		template<typename _data_type>
		struct SimpleGammaEncoderArray
		{
			typedef _data_type data_type;
			typedef SimpleGammaEncoderArray<data_type> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			typedef SimpleGammaEncoder<data_type> encoder_type;
			typedef typename encoder_type::unique_ptr_type encoder_ptr_type;

			std::vector<std::string> const V;
			libmaus2::autoarray::AutoArray<encoder_ptr_type> Aencoders;

			private:
			static std::vector<std::string> constructFileNames(std::string const & base, uint64_t const n, bool const registerTemp)
			{
				std::vector<std::string> V(n);
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					std::ostringstream ostr;
					ostr << base << "_" << std::setw(6) << std::setfill('0') << i;
					V[i] = ostr.str();
					if ( registerTemp )
						libmaus2::util::TempFileRemovalContainer::addTempFile(V[i]);
				}
				return V;
			}

			void init(uint64_t const bs)
			{
				for ( uint64_t i = 0; i < V.size(); ++i )
				{
					encoder_ptr_type Tptr(new encoder_type(V[i],bs));
					Aencoders[i] = UNIQUE_PTR_MOVE(Tptr);
				}
			}

			public:
			SimpleGammaEncoderArray(std::vector<std::string> const & rV, uint64_t const bs = 4*1024)
			: V(rV), Aencoders(V.size())
			{
				init(bs);
			}

			SimpleGammaEncoderArray(std::string const & base, uint64_t const n, bool const registerTemp = true, uint64_t const bs = 4*1024)
			: V(constructFileNames(base,n,registerTemp)), Aencoders(V.size())
			{
				init(bs);
			}

			~SimpleGammaEncoderArray()
			{
				flush();
			}

			std::vector<std::string> const & getFileNames() const
			{
				return V;
			}

			std::string getFileName(uint64_t const i) const
			{
				return V[i];
			}

			uint64_t size() const
			{
				return V.size();
			}

			encoder_type & operator[](uint64_t const i)
			{
				return *Aencoders[i];
			}

			void flush()
			{
				for ( uint64_t i = 0; i < size(); ++i )
					(*this)[i].flush();
			}
		};
	}
}
#endif
