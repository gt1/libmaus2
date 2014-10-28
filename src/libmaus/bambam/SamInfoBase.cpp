/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#include <libmaus/bambam/SamInfoBase.hpp>

/*
 * the tables below were produced by the following c program
 */
 
/*
#include <string.h>
#include <stdio.h>

static char qnameValid[256];
static char rnameFirstValid[256];
static char rnameOtherValid[256];
static char seqValid[256];
static char qualValid[256];

void initSamCharTables()
{
	int i = 0;
	memset(&qnameValid[0],0,sizeof(qnameValid));
	memset(&rnameFirstValid[0],0,sizeof(rnameFirstValid));
	memset(&rnameOtherValid[0],0,sizeof(rnameOtherValid));
	memset(&seqValid[0],0,sizeof(seqValid));
	memset(&qualValid[0],0,sizeof(qualValid));
	
	for ( i = '!'; i <= '?'; ++i )
		qnameValid[i] = 1;
	for ( i = 'A'; i <= '~'; ++i )
		qnameValid[i] = 1;
	for ( i = '!'; i <= '('; ++i )
		rnameFirstValid[i] = 1;
	rnameFirstValid[')'] = 1;
	for ( i = '+'; i <= '<'; ++i )
		rnameFirstValid[i] = 1;
	for ( i = '>'; i <= '~'; ++i )
		rnameFirstValid[i] = 1;
	for ( i = '!'; i <= '~'; ++i )
		rnameOtherValid[i] = 1;
	for ( i = 'A'; i <= 'Z'; ++i )
		seqValid[i] = 1;
	for ( i = 'a'; i <= 'z'; ++i )
		seqValid[i] = 1;
	seqValid['='] = 1;
	seqValid['.'] = 1;
	for ( i = '!'; i <= '~'; ++i )
		qualValid[i] = 1;
}

void printTable(char const * name, char const * T, size_t l, FILE * out)
{
	size_t i;
	fprintf(out,"char const %s[%d] = {", name, (int)l);
	for ( i = 0; i < l; ++i )
	{
		if ( i % 40 == 0 )
			fprintf(out,"\n\t");
		fprintf(out,"%d",T[i]);
		if ( i + 1 < l )
			fprintf(out,",");
	}
	fprintf(out,"\n};\n");
}

int main()
{
	initSamCharTables();
	fprintf(stdout,"#if ! defined(LIBMAUS_BAMBAM_SAMINFO_HPP)\n");
	fprintf(stdout,"#define LIBMAUS_BAMBAM_SAMINFO_HPP\n\n");
	char const * s = 
		"namespace libmaus\n"
		"{\n"
		"\tnamespace bambam\n"
		"\t{\n"
		"\t\tstruct SamInfoBase\n"
		"\t\t{\n"
		"\t\t\tstatic char const qnameValid[256];\n"
		"\t\t\tstatic char const rnameFirstValid[256];\n"
		"\t\t\tstatic char const rnameOtherValid[256];\n"
		"\t\t\tstatic char const seqValid[256];\n"
		"\t\t\tstatic char const qualValid[256];\n"
		"\t\t};\n"
		"\t}\n"
		"}\n"
		;
	fprintf(stdout,"%s",s);
	fprintf(stdout,"#endif\n");
	printTable("libmaus::bambam::SamInfoBase::qnameValid",&qnameValid[0],sizeof(qnameValid)/sizeof(qnameValid[0]),stdout);
	printTable("libmaus::bambam::SamInfoBase::rnameFirstValid",&rnameFirstValid[0],sizeof(rnameFirstValid)/sizeof(rnameFirstValid[0]),stdout);
	printTable("libmaus::bambam::SamInfoBase::rnameOtherValid",&rnameOtherValid[0],sizeof(rnameOtherValid)/sizeof(rnameOtherValid[0]),stdout);
	printTable("libmaus::bambam::SamInfoBase::seqValid",&seqValid[0],sizeof(seqValid)/sizeof(seqValid[0]),stdout);
	printTable("libmaus::bambam::SamInfoBase::qualValid",&qualValid[0],sizeof(qualValid)/sizeof(qualValid[0]),stdout);
	return 0;
}
*/

char const libmaus::bambam::SamInfoBase::qnameValid[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
char const libmaus::bambam::SamInfoBase::rnameFirstValid[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,
	1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
char const libmaus::bambam::SamInfoBase::rnameOtherValid[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
char const libmaus::bambam::SamInfoBase::seqValid[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
char const libmaus::bambam::SamInfoBase::qualValid[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

libmaus::util::DigitTable const libmaus::bambam::SamInfoBase::DT;
libmaus::util::AlphaDigitTable const libmaus::bambam::SamInfoBase::ADT;
libmaus::util::AlphaTable const libmaus::bambam::SamInfoBase::AT;
libmaus::bambam::SamPrintableTable const libmaus::bambam::SamInfoBase::SPT;
libmaus::bambam::SamZPrintableTable const libmaus::bambam::SamInfoBase::SZPT;
libmaus::math::DecimalNumberParser const libmaus::bambam::SamInfoBase::DNP;