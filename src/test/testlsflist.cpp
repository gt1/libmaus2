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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(HAVE_LSF)
#include <lsf/lsbatch.h>
#endif

#include <unistd.h>
#include <sys/types.h>

#include <libmaus2/LibMausConfig.hpp>

#if defined(HAVE_LSF)
int main(int argc, char * argv[])
{
	if (lsb_init(argv[0]) < 0)
	{
		char buf[] = "lsb_init()";
		lsb_perror(buf);
		return EXIT_FAILURE;
	}

	char all[] = "all";
	double const cpuexcessallowed = 0.25;
	time_t minrun = 120;

	fprintf(stdout,"[V] excessallowed %lf minrun %ld\n", cpuexcessallowed, (long)minrun);

	int lsfr = lsb_openjobinfo(0,0/*jobname*/,&all[0]/*user*/,0/*queue*/,0/*host*/,ALL_JOB);

	if ( lsfr < 0 )
	{
		fprintf(stderr, "lsb_openjobinfo failed.\n");
		return EXIT_FAILURE;
	}

	fprintf(stdout,"%12s\t%6s\t%10s\t%7s\t%s\n","jobid","user","used cpu","req cpu","flags");

	while ( lsfr )
	{
		struct jobInfoEnt const * jie = lsb_readjobinfo(&lsfr);

		if ( ! jie )
		{
			lsb_closejobinfo();
			fprintf(stderr,"lsb_readjobinfo failed.\n");
			return EXIT_FAILURE;
		}

		int64_t const jobId = jie->jobId;
		int64_t const state = jie->status;

		int const isrun = (state & JOB_STAT_RUN) != 0;
		int const isfin = (state & (JOB_STAT_DONE|JOB_STAT_EXIT)) != 0;

		// jobs's runtime
		int64_t const rtime = (isfin ? jie->endTime :  time(0)) - jie->startTime;

		if ( (isrun | isfin) && (rtime >= minrun) )
		{
			int64_t const ctime = jie->runRusage.utime + jie->runRusage.stime;
			double const ccpu = (double)(ctime)/(double)(rtime);
			int64_t const reqcpu = jie->submit.numProcessors;

			if ( ccpu > reqcpu + cpuexcessallowed )
			{
				fprintf(stdout,"%12ld\t%6s\t%10lf\t%7ld\t",(long)jobId,jie->user,ccpu,(long)reqcpu);

				if ( state & JOB_STAT_RUN )
					fprintf(stdout,"RUN;");
				if ( state & JOB_STAT_DONE )
					fprintf(stdout,"DONE;");
				if ( state & JOB_STAT_EXIT )
					fprintf(stdout,"EXIT;");

				fprintf(stdout,"\n");
			}
		}
	}

	lsb_closejobinfo();
}
#else
int main()
{
	fprintf(stderr,"LSF support is not present.\n");
}
#endif
