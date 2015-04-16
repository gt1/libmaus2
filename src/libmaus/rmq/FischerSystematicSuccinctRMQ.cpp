/**
    libmaus
    Copyright (C) 2010 Johannes Fischer
    Copyright (C) 2010-2013 German Tischler
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
**/

#include <libmaus/rmq/FischerSystematicSuccinctRMQ.hpp>

const libmaus::rmq::FischerSystematicSuccinctRMQBase::DTidx libmaus::rmq::FischerSystematicSuccinctRMQBase::Catalan[17][17] = {
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},
	{0,0,2,5,9,14,20,27,35,44,54,65,77,90,104,119,135},
	{0,0,0,5,14,28,48,75,110,154,208,273,350,440,544,663,798},
	{0,0,0,0,14,42,90,165,275,429,637,910,1260,1700,2244,2907,3705},
	{0,0,0,0,0,42,132,297,572,1001,1638,2548,3808,5508,7752,10659,14364},
	{0,0,0,0,0,0,132,429,1001,2002,3640,6188,9996,15504,23256,33915,48279},
	{0,0,0,0,0,0,0,429,1430,3432,7072,13260,23256,38760,62016,95931,144210},
	{0,0,0,0,0,0,0,0,1430,4862,11934,25194,48450,87210,149226,245157,389367},
	{0,0,0,0,0,0,0,0,0,4862,16796,41990,90440,177650,326876,572033,961400},
	{0,0,0,0,0,0,0,0,0,0,16796,58786,149226,326876,653752,1225785,2187185},
	{0,0,0,0,0,0,0,0,0,0,0,58786,208012,534888,1188640,2414425,4601610},
	{0,0,0,0,0,0,0,0,0,0,0,0,208012,742900,1931540,4345965,8947575},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,742900,2674440,7020405,15967980},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,2674440,9694845,25662825},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9694845,35357670},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,35357670}
};

const char libmaus::rmq::FischerSystematicSuccinctRMQBase::LSBTable256[256] = 
	{
		0,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0
	};

const char libmaus::rmq::FischerSystematicSuccinctRMQBase::LogTable256[256] = 
	{
		0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
		4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
	};

const libmaus::rmq::FischerSystematicSuccinctRMQBase::DTsucc 
	libmaus::rmq::FischerSystematicSuccinctRMQBase::HighestBitsSet[8] = {
		static_cast<libmaus::rmq::FischerSystematicSuccinctRMQBase::DTsucc>(~0), 
		static_cast<libmaus::rmq::FischerSystematicSuccinctRMQBase::DTsucc>(~1), 
		static_cast<libmaus::rmq::FischerSystematicSuccinctRMQBase::DTsucc>(~3), 
		static_cast<libmaus::rmq::FischerSystematicSuccinctRMQBase::DTsucc>(~7), 
		static_cast<libmaus::rmq::FischerSystematicSuccinctRMQBase::DTsucc>(~15), 
		static_cast<libmaus::rmq::FischerSystematicSuccinctRMQBase::DTsucc>(~31), 
		static_cast<libmaus::rmq::FischerSystematicSuccinctRMQBase::DTsucc>(~63), 
		static_cast<libmaus::rmq::FischerSystematicSuccinctRMQBase::DTsucc>(~127)
	};

