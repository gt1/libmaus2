/*
    libmaus
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

#include <libmaus/network/LogReceiver.hpp>

int main(/* int argc, char * argv[] */)
{
	{
		::libmaus::network::LogReceiver::unique_ptr_type LR(new ::libmaus::network::LogReceiver("logpre",4444,1024));

		::libmaus::network::StringRecDispatchCallback srdc;
		
		::libmaus::network::LogReceiverTestProcess::unique_ptr_type TP0 = UNIQUE_PTR_MOVE(LR->constructLogReceiverTestProcess(0 /* id */,&srdc));
		::libmaus::network::LogReceiverTestProcess::unique_ptr_type TP1 = UNIQUE_PTR_MOVE(LR->constructLogReceiverTestProcess(1 /* id */,&srdc));
		::libmaus::network::LogReceiverTestProcess::unique_ptr_type TP2 = UNIQUE_PTR_MOVE(LR->constructLogReceiverTestProcess(2 /* id */,&srdc));
		
		std::map<uint64_t, ::libmaus::network::SocketBase::shared_ptr_type > controlsocks;
		
		sleep(2);

		while ( LR->controlDescriptorPending() )
		{
			::libmaus::network::LogReceiver::ControlDescriptor P = LR->getControlDescriptor();
			controlsocks [ P.id ] = P.controlsock;
			std::cerr << "Got control socket for " << P.id << std::endl;
			
			switch ( P.id )
			{
				case 0: controlsocks.find(P.id)->second->writeString("Coke"); break;
				case 1: controlsocks.find(P.id)->second->writeString("Pepsi"); break;
				case 2: controlsocks.find(P.id)->second->writeString("Juice"); break;
			}
		}
		
		LR.reset();
			
		return 0;
	}
}
