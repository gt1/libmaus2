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
#if ! defined(LIBMAUS_LSF_LSFTASKQUEUE_HPP)
#define LIBMAUS_LSF_LSFTASKQUEUE_HPP

#include <libmaus/types/types.hpp>
#include <iostream>
#include <string>

namespace libmaus
{
	namespace lsf
	{
		struct LsfTaskQueueElement
		{
			uint64_t id;
			std::string packettype;
			std::string payload;
			
			LsfTaskQueueElement() {}
			LsfTaskQueueElement(
				uint64_t const rid,
				std::string rpackettype,
				std::string rpayload
			) : id(rid), packettype(rpackettype), payload(rpayload) {}
		};
		
		inline std::ostream & operator<<(std::ostream & out, LsfTaskQueueElement const & LTQE)
		{
			out << "LsfTaskQueueElement("
				<< LTQE.id << "," << LTQE.packettype << ",[" << LTQE.payload.size() << "])"
				<< ")";
			return out;
		}
	
		struct LsfTaskQueue
		{
			bool verbose;
			
			LsfTaskQueue(bool const rverbose = false)
			: verbose(rverbose)
			{
			}
		
			virtual ~LsfTaskQueue() {}

			virtual bool empty() = 0;
			virtual uint64_t next() = 0;
			virtual LsfTaskQueueElement nextPacket()
			{
				uint64_t const nextid = next();
				return LsfTaskQueueElement(nextid,
					packettype(nextid),
					payload(nextid));
			}
			virtual std::string payload(uint64_t const i) = 0;
			virtual std::string packettype(uint64_t const i) = 0;
			virtual std::string termpackettype() = 0;
			virtual void putback(uint64_t const) = 0;
			virtual void finished(uint64_t const, std::string) = 0;
			virtual bool isRealPackage(uint64_t)
			{
				return true;
			}
			virtual uint64_t numUnfinished() = 0;
		};
	}
}
#endif
