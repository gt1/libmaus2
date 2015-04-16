/*
    libmaus
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
#if ! defined(LIBMAUS_BAMBAM_PARALLEL_READENDSCONTAINERFREELISTINTERFACE_HPP)
#define LIBMAUS_BAMBAM_PARALLEL_READENDSCONTAINERFREELISTINTERFACE_HPP

#include <libmaus/bambam/ReadEndsContainer.hpp>

namespace libmaus
{
	namespace bambam
	{
		namespace parallel
		{
			struct ReadEndsContainerFreeListInterface
			{
				virtual ~ReadEndsContainerFreeListInterface() {}
				virtual libmaus::bambam::ReadEndsContainer::shared_ptr_type getFragContainer() = 0;
				virtual void returnFragContainer(libmaus::bambam::ReadEndsContainer::shared_ptr_type ptr) = 0;
				virtual libmaus::bambam::ReadEndsContainer::shared_ptr_type getPairContainer() = 0;
				virtual void returnPairContainer(libmaus::bambam::ReadEndsContainer::shared_ptr_type ptr) = 0;
			};
		}
	}
}
#endif
