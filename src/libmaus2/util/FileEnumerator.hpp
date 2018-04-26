/*
    libmaus2
    Copyright (C) 2018 German Tischler-Höhle

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
#if ! defined(LIBMAUS2_UTIL_FILEENUMERATOR_HPP)
#define LIBMAUS2_UTIL_FILEENUMERATOR_HPP

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <libmaus2/exception/LibMausException.hpp>
#include <cstring>
#include <cerrno>

namespace libmaus2
{
	namespace util
	{
		struct FileEnumerator
		{
			std::string const fn;
			DIR * dir;

			FileEnumerator(std::string const & rfn) : fn(rfn), dir(0)
			{
				while ( ! dir )
				{
					dir = opendir(fn.c_str());

					if ( !dir )
					{
						int const error = errno;

						switch ( error )
						{
							case EINTR:
							case EAGAIN:
								break;
							default:
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "[E] opendir(" << fn << "): " << strerror(error) << std::endl;
								lme.finish();
								throw lme;
							}
						}
					}
				}
			}
			~FileEnumerator()
			{
				if ( dir )
				{
					closedir(dir);
					dir = 0;
				}
			}

			bool getNextFile(std::string & name, std::string const prefix, std::string const suffix)
			{
				struct dirent * p = 0;

				while ( (p=readdir(dir)) )
				{
					std::string const q = fn + "/" + &(p->d_name[0]);

					struct stat statbuf;

					int r = -1;
					bool done = false;

					while ( !done )
					{
						r = stat(q.c_str(), &statbuf);

						if ( r < 0 )
						{
							int const error = errno;

							switch ( error )
							{
								case EINTR:
								case EAGAIN:
									break;
								default:
								{
									std::cerr << "[W] stat(" << q << "): " << strerror(error) << std::endl;
									done = true;
									break;
								}
							}
						}
						else
						{
							done = true;
						}
					}

					if ( r == 0 )
					{
						if ( (statbuf.st_mode & S_IFMT) == S_IFREG )
						{
							name = std::string(&(p->d_name[0]));

							if (
								name.size() >= prefix.size()
								&&
								name.substr(0,prefix.size()) == prefix
							)
							{
								name = name.substr(prefix.size());

								if (
									name.size() >= suffix.size()
									&&
									name.substr(name.size()-suffix.size()) == suffix
								)
								{
									name = name.substr(0,name.size()-suffix.size());

									return true;
								}
							}
						}
					}
				}

				return false;
			}
		};
	}
}
#endif
