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
#include <libmaus2/aio/ArrayFile.hpp>
#include <libmaus2/util/ArgParser.hpp>

void testArrayInput(std::string const & s)
{
	libmaus2::aio::ArrayInputStream<std::string::const_iterator> AIS(s.begin(),s.end());

	for ( uint64_t i = 0; i < s.size(); ++i )
		assert ( AIS.peek() != std::istream::traits_type::eof() && AIS.get() == s[i] );

	for ( uint64_t i = 0; i < s.size(); ++i )
	{
		AIS.clear();
		AIS.seekg(i);
		for ( uint64_t j = i; j < s.size(); ++j )
			assert ( AIS.peek() != std::istream::traits_type::eof() && AIS.get() == s[j] );
	}

	libmaus2::aio::ArrayFileContainer<std::string::const_iterator> container;
	container.add("file",s.begin(),s.end());
	libmaus2::aio::ArrayInputStreamFactory<std::string::const_iterator>::shared_ptr_type factory(
		new libmaus2::aio::ArrayInputStreamFactory<std::string::const_iterator>(container));

	libmaus2::aio::InputStream::unique_ptr_type Pistr(factory->constructUnique("file"));
	for ( uint64_t i = 0; i < s.size(); ++i )
		assert ( Pistr->peek() != std::istream::traits_type::eof() && Pistr->get() == s[i] );
	libmaus2::aio::InputStream::shared_ptr_type Sistr(factory->constructShared("file"));
	for ( uint64_t i = 0; i < s.size(); ++i )
		assert ( Sistr->peek() != std::istream::traits_type::eof() && Sistr->get() == s[i] );

	libmaus2::aio::InputStreamFactoryContainer::addHandler("array", factory);
	libmaus2::aio::InputStreamInstance ISI("array:file");
	for ( uint64_t i = 0; i < s.size(); ++i )
		assert ( ISI.peek() != std::istream::traits_type::eof() && ISI.get() == s[i] );
	libmaus2::aio::InputStreamFactoryContainer::removeHandler("array");
}

int main()
{
	try
	{
		std::string const s = "hello world, hello moon";
		testArrayInput(s);
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
		return EXIT_FAILURE;
	}
}
