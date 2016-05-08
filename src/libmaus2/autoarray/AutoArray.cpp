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

#include <libmaus2/autoarray/AutoArray.hpp>

/**
 * constructor copying current values of AutoArray memory usage
 **/
libmaus2::autoarray::AutoArrayMemUsage::AutoArrayMemUsage()
: memusage(AutoArray_memusage), peakmemusage(AutoArray_peakmemusage), maxmem(AutoArray_maxmem)
{
}

/**
 * copy constructor
 * @param o object copied
 **/
libmaus2::autoarray::AutoArrayMemUsage::AutoArrayMemUsage(libmaus2::autoarray::AutoArrayMemUsage const & o)
: memusage(o.memusage), peakmemusage(o.peakmemusage), maxmem(o.maxmem)
{
}

/**
 * assignment operator
 * @param o object copied
 * @return *this
 **/
libmaus2::autoarray::AutoArrayMemUsage & libmaus2::autoarray::AutoArrayMemUsage::operator=(AutoArrayMemUsage const & o)
{
	memusage = o.memusage;
	peakmemusage = o.peakmemusage;
	maxmem = o.maxmem;
	return *this;
}

/**
 * return true iff *this == o
 * @param o object to be compared
 * @return true iff *this == o
 **/
bool libmaus2::autoarray::AutoArrayMemUsage::operator==(libmaus2::autoarray::AutoArrayMemUsage const & o) const
{
	return
		memusage == o.memusage
		&&
		peakmemusage == o.peakmemusage
		&&
		maxmem == o.maxmem;
}
/**
 * return true iff *this is different from o
 * @param o object to be compared
 * @return true iff *this != o
 **/
bool  libmaus2::autoarray::AutoArrayMemUsage::operator!=(libmaus2::autoarray::AutoArrayMemUsage const & o) const
{
	return ! ((*this)==o);
}

std::ostream & libmaus2::autoarray::operator<<(std::ostream & out, libmaus2::autoarray::AutoArrayMemUsage const & aamu)
{
	#if defined(_OPENMP)
	libmaus2::autoarray::AutoArray_lock.lock();
	#endif
	out << "AutoArrayMemUsage("
		"memusage=" << static_cast<double>(aamu.memusage)/(1024.0*1024.0) << "," <<
		"peakmemusage=" << static_cast<double>(aamu.peakmemusage)/(1024.0*1024.0) << "," <<
		"maxmem=" << static_cast<double>(aamu.maxmem)/(1024.0*1024.0) << ")";
	#if defined(_OPENMP)
	libmaus2::autoarray::AutoArray_lock.unlock();
	#endif
	return out;
}

#if defined(LIBMAUS2_AUTOARRAY_AUTOARRAYTRACE)
#include <libmaus2/util/PosixExecute.hpp>
#include <libmaus2/util/UnitNum.hpp>

#include <set>

void libmaus2::autoarray::autoArrayPrintTraces(std::ostream & out)
{
	libmaus2::parallel::ScopePosixSpinLock slock(tracelock);
	std::sort(tracevector.begin(),tracevector.end());
	std::reverse(tracevector.begin(),tracevector.end());

	std::vector< std::vector<std::string> > traces;

	for ( uint64_t i = 0; i < tracevector.size(); ++i )
	{
		char ** strings = backtrace_symbols((void **)(&(tracevector[i].P[0])),tracevector[i].tracelength);

		std::vector<std::string> V;
		for ( size_t j = 0; j < tracevector[i].tracelength; ++j )
			V.push_back(std::string(strings[j]));

		traces.push_back(V);

		free(strings);
	}

	std::map<std::string,std::set<std::string> > exes;
	for ( uint64_t i = 0; i < traces.size(); ++i )
		for ( uint64_t j = 0; j < traces[i].size(); ++j )
		{
			std::string exe = traces[i][j];

			if ( exe.find('(') != std::string::npos )
			{
				exe = exe.substr(0,exe.find('('));

				std::string adr = traces[i][j];

				if ( adr.find("[0x") != std::string::npos )
				{
					adr = adr.substr(adr.find("[0x")+1);

					if ( adr.size() && adr[adr.size()-1] == ']' )
					{
						adr = adr.substr(0,adr.size()-1);
						exes[exe].insert(adr);
					}
				}
			}
		}

	std::map< std::string, std::map<std::string,std::string> > linenumbers;
	for ( std::map<std::string,std::set<std::string> >::const_iterator ita = exes.begin(); ita != exes.end(); ++ita )
	{
		std::string const exe = ita->first;
		std::set<std::string> const & adrs = ita->second;

		std::ostringstream comlinestr;
		comlinestr << "/usr/bin/addr2line" << " -e " << exe;

		for ( std::set<std::string>::const_iterator sita = adrs.begin(); sita != adrs.end(); ++sita )
			comlinestr << " " << *sita;
		std::string const comline = comlinestr.str();

		std::string addrout,addrerr;
		::libmaus2::util::PosixExecute::execute(comline,addrout,addrerr,true /* do not throw exceptions */);

		std::vector<std::string> outputlines;
		std::istringstream addristr(addrout);
		while ( addristr )
		{
			std::string line;
			std::getline(addristr,line);
			if ( line.size() )
				outputlines.push_back(line);
		}

		if ( outputlines.size() == adrs.size() )
		{
			uint64_t j = 0;
			for ( std::set<std::string>::const_iterator sita = adrs.begin(); sita != adrs.end(); ++sita )
				linenumbers[exe][*sita /* adr */] = outputlines[j++];
		}
	}

	for ( uint64_t i = 0; i < tracevector.size(); ++i )
	{
		out << std::string(80,'-') << std::endl;
		char ** strings = backtrace_symbols((void **)(&(tracevector[i].P[0])),tracevector[i].tracelength);

		for ( size_t j = 0; j < tracevector[i].tracelength; ++j )
		{
			std::string exe = strings[j];

			if ( exe.find('(') != std::string::npos )
				exe = exe.substr(0,exe.find('('));

			std::string adr = strings[j];

			if ( adr.find("[0x") != std::string::npos )
			{
				adr = adr.substr(adr.find("[0x")+1);

				if ( adr.size() && adr[adr.size()-1] == ']' )
					adr = adr.substr(0,adr.size()-1);
			}
			else
			{
				adr = std::string();
			}

			std::string mang = strings[j];
			if ( mang.find("(") != std::string::npos )
			{
				mang = mang.substr(mang.find("(")+1);
				if ( mang.find(")") != std::string::npos )
				{
					mang = mang.substr(0,mang.find(")"));

					if ( mang.find("+0x") != std::string::npos )
					{
						mang = mang.substr(0,mang.find("+0x"));
					}
					else
					{
						mang = std::string();
					}

					mang = libmaus2::util::Demangle::demangleName(mang);
				}
				else
				{
					mang = std::string();
				}
			}
			else
			{
				mang = std::string();
			}

			std::string addrout;
			addrout =
				(linenumbers.find(exe) != linenumbers.end() &&
				linenumbers.find(exe)->second.find(adr) != linenumbers.find(exe)->second.end())
				?
				linenumbers.find(exe)->second.find(adr)->second : std::string();

			if ( mang.size() )
			{
				out << mang << "\t" << addrout << std::endl;
			}
			else
			{
				out << strings[j] << "\t" << addrout;
				if ( mang.size() )
					out << "\t" << mang;
				out << std::endl;
			}
		}

		out << "type=" << tracevector[i].type << std::endl;
		out << "remcnt=" << libmaus2::util::UnitNum::unitNum(tracevector[i].allocbytes - tracevector[i].freebytes) << std::endl;
		out << "alloccnt=" << tracevector[i].alloccnt << std::endl;
		out << "allocbytes=" << tracevector[i].allocbytes << std::endl;
		out << "freecnt=" << tracevector[i].freecnt << std::endl;
		out << "freebytes=" << tracevector[i].freebytes << std::endl;

		free(strings);
	}
}
#endif
