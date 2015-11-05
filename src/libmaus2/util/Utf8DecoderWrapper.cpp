/*
    libmaus2
    Copyright (C) 2009-2013 German Tischler
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

#include <libmaus2/util/Utf8DecoderWrapper.hpp>

libmaus2::util::Utf8DecoderWrapper::Utf8DecoderWrapper(std::string const & filename, uint64_t const buffersize)
: Utf8DecoderBuffer(filename,buffersize), ::std::wistream(this)
{

}

wchar_t libmaus2::util::Utf8DecoderWrapper::getSymbolAtPosition(std::string const & filename, uint64_t const offset)
{
	this_type I(filename);
	I.seekg(offset);
	return I.get();
}

uint64_t libmaus2::util::Utf8DecoderWrapper::getFileSize(std::string const & filename)
{
	this_type I(filename);
	I.seekg(0,std::ios::end);
	return I.tellg();
}
