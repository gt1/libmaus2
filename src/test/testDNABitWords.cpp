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

#include <libmaus2/fastx/MultiWordDNABitBuffer.hpp>
#include <libmaus2/fastx/SingleWordDNABitBuffer.hpp>
#include <ctime>
#include <cstdlib>

#include <libmaus2/consensus/Consensus.hpp>

template<unsigned int bases_per_word>
void testMultiWordDNABitBuffer()
{
	for ( unsigned int k = 1; k <= 200; ++k )
	{
		::libmaus2::fastx::MultiWordDNABitBuffer<bases_per_word> mwdbb3(k);
		::libmaus2::fastx::MultiWordDNABitBuffer<bases_per_word> mwdbb4(k);

		std::cerr << "bases per word " << bases_per_word << " k=" << k << std::endl;

		srand(time(0));

		for ( unsigned int j = 0; j < 1000; ++j )
		{
			for ( unsigned int i = 0; i < 518; ++i )
			{
				mwdbb3.pushFront( rand() % 4 );
				mwdbb4.pushFront( rand() % 4 );
			}
			for ( unsigned int i = 0; i < mwdbb3.width; ++i )
			{
				uint64_t const v = rand() % 4;
				mwdbb3.pushBack(v);
				mwdbb4.pushBack(v);
			}
			for ( unsigned int i = 0; i < mwdbb3.singlewordbuffers; ++i )
			{
				bool ok = (mwdbb3.buffers[i] == mwdbb4.buffers[i]);
				if ( ! ok )
					std::cerr << "Fail." << std::endl;
				assert ( ok );
			}
			assert ( mwdbb3.toString() == mwdbb4.toString() );
		}
	}
}

template<unsigned int bases_per_word>
void testMultiWordDNABitBuffer32()
{
	for ( unsigned int k = 1; k <= 200; ++k )
	{
		::libmaus2::fastx::MultiWordDNABitBuffer<bases_per_word> mwdbb3(k);
		::libmaus2::fastx::MultiWordDNABitBuffer<32> mwdbb4(k);

		std::cerr << "bases per word " << bases_per_word << " k=" << k << std::endl;

		srand(time(0));

		for ( unsigned int j = 0; j < 1000; ++j )
		{
			for ( unsigned int i = 0; i < 518; ++i )
			{
				mwdbb3.pushFront( rand() % 4 );
				mwdbb4.pushFront( rand() % 4 );
			}
			for ( unsigned int i = 0; i < mwdbb3.width; ++i )
			{
				uint64_t const v = rand() % 4;
				mwdbb3.pushBack(v);
				mwdbb4.pushBack(v);
			}

			bool ok = mwdbb3.toString() == mwdbb4.toString();

			if ( ! ok )
			{
				std::cerr << "FAILURE: " << std::endl;
				std::cerr << mwdbb3.toString() << std::endl;
				std::cerr << mwdbb4.toString() << std::endl;
			}

			assert ( ok );
		}
	}
}

void shortWordDNABitBufferTest()
{
	::libmaus2::fastx::SingleWordDNABitBuffer swdbb(7);
	::libmaus2::fastx::MultiWordDNABitBuffer<32> mwdbb(7);

	for ( unsigned int i = 0; i < 2*swdbb.width; ++i )
	{
		swdbb.pushBackMasked(2);
		mwdbb.pushBack(2);
		assert ( swdbb.toString() == mwdbb.toString() );
		assert ( swdbb.buffer == mwdbb.buffers[0] );

		swdbb.pushBackMasked(1);
		mwdbb.pushBack(1);
		assert ( swdbb.toString() == mwdbb.toString() );
		assert ( swdbb.buffer == mwdbb.buffers[0] );
	}

	swdbb.reset();
	mwdbb.reset();
	for ( unsigned int i = 0; i < 2*swdbb.width; ++i )
	{
		swdbb.pushFront(2);
		mwdbb.pushFront(2);
		assert ( swdbb.toString() == mwdbb.toString() );
		assert ( swdbb.buffer == mwdbb.buffers[0] );

		swdbb.pushFront(1);
		mwdbb.pushFront(1);
		assert ( swdbb.toString() == mwdbb.toString() );
		assert ( swdbb.buffer == mwdbb.buffers[0] );
	}
	std::cerr << "----" << std::endl;

	swdbb.reset();
	for ( unsigned int i = 0; i < 2*swdbb.width; ++i )
	{
		swdbb.pushFront(3);
		swdbb.pushFront(2);
		swdbb.pushFront(1);
		swdbb.pushFront(0);
	}
}

int main()
{
	try
	{
		testMultiWordDNABitBuffer32<31>();

		testMultiWordDNABitBuffer<31>();
		testMultiWordDNABitBuffer<32>();
		shortWordDNABitBufferTest();
	}
	catch(std::exception const & ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}
