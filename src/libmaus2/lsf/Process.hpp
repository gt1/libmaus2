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
#if ! defined(LIBMAUS2_LSF_PROCESS_HPP)
#define LIBMAUS2_LSF_PROCESS_HPP

#if defined(HAVE_LSF)

#include <lsf/lsbatch.h>

namespace libmaus2
{
	namespace lsf
	{
		struct Process
		{
			int64_t lsfid;

			enum state
			{
				state_unknown,
				state_pending,
				state_psusp,
				state_ssusp,
				state_ususp,
				state_run,
				state_exit,
				state_done,
				state_pdone,
				state_perr,
				state_wait
			};

			state getState() const
			{
				int64_t const status = getIntState();

				if ( status & JOB_STAT_PEND )
					return state_pending;
				else if ( status & JOB_STAT_PSUSP )
					return state_psusp;
				else if ( status & JOB_STAT_SSUSP )
					return state_ssusp;
				else if ( status & JOB_STAT_USUSP )
					return state_ususp;
				else if ( status & JOB_STAT_RUN )
					return state_run;
				else if ( status & JOB_STAT_EXIT )
					return state_exit;
				else if ( status & JOB_STAT_DONE )
					return state_done;
				else if ( status & JOB_STAT_PDONE )
					return state_pdone;
				else if ( status & JOB_STAT_PERR )
					return state_perr;
				else if ( status & JOB_STAT_WAIT )
					return state_wait;
				else
					return state_unknown;
			}

			int64_t getIntState() const
			{
				int lsfr = lsb_openjobinfo(lsfid,0/*jobname*/,0/*user*/,0/*queue*/,0/*host*/,ALL_JOB);

				if ( lsfr < 0 )
				{
					return 0;
				}
				else if ( lsfr == 0 )
				{
					lsb_closejobinfo();
					return 0;
				}
				else
				{
					jobInfoEnt const * jie = lsb_readjobinfo(&lsfr);

					if ( ! jie )
					{
						lsb_closejobinfo();
						return 0;
					}

					int64_t const status = jie->status;
					lsb_closejobinfo();

					return status;
				}
			}
		};
	}
}
#endif

#endif
