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
#include <libmaus2/aio/InputOutputStreamInstance.hpp>
#include <libmaus2/aio/InputOutputStreamFactoryContainer.hpp>

libmaus2::aio::InputOutputStreamInstance::InputOutputStreamInstance(std::string const & fn, std::ios_base::openmode mode)
: libmaus2::aio::InputOutputStreamPointerWrapper(libmaus2::aio::InputOutputStreamFactoryContainer::constructUnique(fn,mode)), std::iostream(InputOutputStreamPointerWrapper::getStreamReference().rdbuf()) {}
