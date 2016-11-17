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
#if ! defined(LIBMAUS2_AIO_TEMPFILEARRAY_HPP)
#define LIBMAUS2_AIO_TEMPFILEARRAY_HPP

#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/util/TempFileRemovalContainer.hpp>
#include <libmaus2/sorting/SortingBufferedOutputFile.hpp>

namespace libmaus2
{
	namespace aio
	{
		struct TempFileArray
		{
			typedef TempFileArray this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::string const prefix;
			libmaus2::autoarray::AutoArray<std::string> Vfn;
			libmaus2::autoarray::AutoArray<libmaus2::aio::OutputStreamInstance::unique_ptr_type> AOS;

			TempFileArray(std::string const & rprefix, uint64_t const n)
			: prefix(rprefix), Vfn(n), AOS(n)
			{
				for ( uint64_t i = 0; i < n; ++i )
				{
					std::ostringstream ostr;
					ostr << prefix << libmaus2::util::ArgInfo::getDefaultTmpFileName(std::string()) << std::setw(6) << std::setfill('0') << i;
					Vfn[i] = ostr.str();
					libmaus2::util::TempFileRemovalContainer::addTempFile(Vfn[i]);
					libmaus2::aio::OutputStreamInstance::unique_ptr_type tptr(new libmaus2::aio::OutputStreamInstance(Vfn[i]));
					AOS[i] = UNIQUE_PTR_MOVE(tptr);
				}
			}

			~TempFileArray()
			{
				flush();
				close();
			}

			void close()
			{
				for ( uint64_t i = 0; i < size(); ++i )
					if ( AOS[i] )
						AOS[i].reset();
			}

			void flush()
			{
				for ( uint64_t i = 0; i < size(); ++i )
					if ( AOS[i] )
						(*this)[i].flush();
			}

			uint64_t size() const
			{
				return Vfn.size();
			}

			std::ostream & operator[](uint64_t const i)
			{
				return *(AOS[i]);
			}

			std::string const & getName(uint64_t const i) const
			{
				return Vfn[i];
			}

			template<typename data_type, typename order_type = std::less<data_type> >
			void merge(std::ostream & out, order_type & order = order_type())
			{
				flush();
				close();

				std::string const sorttemp = prefix + libmaus2::util::ArgInfo::getDefaultTmpFileName(std::string()) + ".sorttmp";
				libmaus2::util::TempFileRemovalContainer::addTempFile(sorttemp);

				typedef typename libmaus2::sorting::SerialisingSortingBufferedOutputFile<data_type,order_type> sorter_type;
				typename sorter_type::unique_ptr_type SSBOF(
					new sorter_type(sorttemp,order)
				);

				data_type D;

				for ( uint64_t i = 0; i < Vfn.size(); ++i )
				{
					libmaus2::aio::InputStreamInstance::unique_ptr_type PISI(new libmaus2::aio::InputStreamInstance(Vfn[i]));
					std::istream & ISI = *PISI;

					while ( ISI && ISI.peek() != std::istream::traits_type::eof() )
					{
						D.deserialise(ISI);
						SSBOF->put(D);
					}

					PISI.reset();
					libmaus2::aio::FileRemoval::removeFile(Vfn[i]);
				}

				typename sorter_type::merger_ptr_type Pmerger(SSBOF->getMerger());
				while ( Pmerger->getNext(D) )
					D.serialise(out);

				Pmerger.reset();
				SSBOF.reset();

				libmaus2::aio::FileRemoval::removeFile(sorttemp);

				out.flush();
			}

			template<typename data_type, typename order_type = std::less<data_type> >
			void merge(std::string const & fn, order_type & order = order_type())
			{
				libmaus2::aio::OutputStreamInstance OSI(fn);
				merge<data_type,order_type>(OSI,order);
			}
		};
	}
}
#endif
