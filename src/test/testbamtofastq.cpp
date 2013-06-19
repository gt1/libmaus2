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
#include <libmaus/bambam/CircularHashCollatingBamDecoder.hpp>	
#include <libmaus/util/TempFileRemovalContainer.hpp>

using namespace libmaus::bambam;
using namespace libmaus::util;
using namespace std;

int main()
{
	// typedef BamParallelCircularHashCollatingBamDecoder collator_type;
	typedef BamCircularHashCollatingBamDecoder collator_type;
	typedef collator_type::alignment_ptr_type alignment_ptr_type;

	/* remove temporary file at program exit */
	string const tmpfilename = "tmpfile";
	TempFileRemovalContainer::addTempFile(tmpfilename);

	// libmaus::aio::CheckedOutputStream copystr("copy.bam");

	/* set up collator object */	
	collator_type C(cin,tmpfilename,0,true,20,32*1024*1024);
	// collator_type C(cin,8,tmpfilename);
	pair<alignment_ptr_type,alignment_ptr_type> P;

	/* read alignments */	
	while ( C.tryPair(P) )
		/* if we have a pair, then print both ends as FastQ */
		if ( P.first && P.second )
		{
			cout << P.first->formatFastq();
			cout << P.second->formatFastq();
		}
}
