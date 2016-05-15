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
#include <libmaus2/util/TempFileRemovalContainer.hpp>

bool ::libmaus2::util::SignalHandlerContainer::setupComplete = false;

std::set < std::string > libmaus2::util::TempFileRemovalContainer::tmpfilenames;
std::vector < std::string > libmaus2::util::TempFileRemovalContainer::tmpdirectories;
std::vector < std::string > libmaus2::util::TempFileRemovalContainer::tmpsemaphores;
bool ::libmaus2::util::TempFileRemovalContainer::setupComplete = false;
libmaus2::util::TempFileRemovalContainer::sighandler_t libmaus2::util::TempFileRemovalContainer::siginthandler = 0;
libmaus2::util::TempFileRemovalContainer::sighandler_t libmaus2::util::TempFileRemovalContainer::sigtermhandler = 0;
libmaus2::util::TempFileRemovalContainer::sighandler_t libmaus2::util::TempFileRemovalContainer::sighuphandler = 0;
libmaus2::util::TempFileRemovalContainer::sighandler_t libmaus2::util::TempFileRemovalContainer::sigpipehandler = 0;
::libmaus2::parallel::OMPLock libmaus2::util::TempFileRemovalContainer::lock;
