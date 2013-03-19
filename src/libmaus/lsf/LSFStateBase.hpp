/**
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
**/
#if !defined(LIBMAUS_LSF_LSFSTATEBASE_HPP)
#define LIBMAUS_LSF_LSFSTATEBASE_HPP

#include <ostream>

namespace libmaus
{
	namespace lsf
	{
		struct LSFStateBase
		{
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
		
		};
		
		inline std::ostream & operator<<(std::ostream & out, LSFStateBase::state state)
		{
			switch ( state )
			{
				case LSFStateBase::state_unknown: out << "state_unknown"; break;
				case LSFStateBase::state_pending: out << "state_pending"; break;
				case LSFStateBase::state_psusp: out << "state_psusp"; break;
				case LSFStateBase::state_ssusp: out << "state_ssusp"; break;
				case LSFStateBase::state_ususp: out << "state_ususp"; break;
				case LSFStateBase::state_run: out << "state_run"; break;
				case LSFStateBase::state_exit: out << "state_exit"; break;
				case LSFStateBase::state_done: out << "state_done"; break;
				case LSFStateBase::state_pdone: out << "state_pdone"; break;
				case LSFStateBase::state_perr: out << "state_perr"; break;
				case LSFStateBase::state_wait: out << "state_wait"; break;
			}
			
			return out;
		}
	}
}
#endif
