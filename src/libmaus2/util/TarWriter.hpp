/*
    libmaus2
    Copyright (C) 2017 German Tischler

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
#if ! defined(LIBMAUS2_UTIL_TARWRITER_HPP)
#define LIBMAUS2_UTIL_TARWRITER_HPP

#include <libmaus2/types/types.hpp>

#if defined(LIBMAUS2_HAVE_LIBARCHIVE)
#include <archive.h>
#include <archive_entry.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#endif

#include <cerrno>
#include <libmaus2/exception/LibMausException.hpp>
#include <libmaus2/aio/OutputStreamInstance.hpp>
#include <libmaus2/aio/StreamLock.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>

namespace libmaus2
{
	namespace util
	{
		struct TarWriter
		{
			typedef TarWriter this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			uid_t const uid;
			gid_t const gid;

			#if defined(LIBMAUS2_HAVE_LIBARCHIVE)
			struct archive * arch;
			std::string tarfilename;
			std::string error;
			libmaus2::aio::OutputStreamInstance::unique_ptr_type pOSI;
			std::ostream * OSI;
			bool closed;
			std::string username;
			std::string groupname;

			static int open_callback(struct archive * arch, void *client_data)
			{
				TarWriter * T = reinterpret_cast<TarWriter *>(client_data);

				try
				{

					libmaus2::aio::OutputStreamInstance::unique_ptr_type tOSI(
						new libmaus2::aio::OutputStreamInstance(T->tarfilename)
					);

					T->pOSI = UNIQUE_PTR_MOVE(tOSI);
					T->OSI = T->pOSI.get();

					return ARCHIVE_OK;
				}
				catch(std::exception const & ex)
				{
					try
					{
						T->error = ex.what();
						archive_set_error(arch,EIO,"%s",T->error.c_str());
					}
					catch(...)
					{

					}
					return ARCHIVE_FATAL;
				}
			}

			static int close_callback(struct archive * arch, void *client_data)
			{
				TarWriter * T = reinterpret_cast<TarWriter *>(client_data);

				try
				{
					T->OSI->flush();
					T->pOSI.reset();

					return ARCHIVE_OK;
				}
				catch(std::exception const & ex)
				{
					try
					{
						T->error = ex.what();
						archive_set_error(arch,EIO,"%s",T->error.c_str());
					}
					catch(...)
					{

					}
					return ARCHIVE_FATAL;
				}
			}

			static __LA_SSIZE_T write_callback(struct archive * arch, void *client_data, const void *buffer, size_t length)
			{
				TarWriter * T = reinterpret_cast<TarWriter *>(client_data);

				try
				{
					T->OSI->write(reinterpret_cast<char const *>(buffer),length);

					return length;
				}
				catch(std::exception const & ex)
				{
					try
					{
						T->error = ex.what();
						archive_set_error(arch,EIO,"%s",T->error.c_str());
					}
					catch(...)
					{

					}
					return -1;
				}
			}

			TarWriter(std::string const & rfilename)
			: uid(getuid()), gid(getgid()), tarfilename(rfilename), closed(false), username(getUserName()), groupname(getGroupName())
			{
				arch = archive_write_new();

				if ( ! arch )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "[E] TarWriter: failed call archive_write_new" << std::endl;
					lme.finish();
					throw lme;
				}

				// archive_write_add_filter_gzip(arch);
				archive_write_set_format_pax_restricted(arch);

				archive_write_open(arch,reinterpret_cast<void *>(this),open_callback,write_callback,close_callback);
			}

			void close()
			{
				if ( ! closed )
				{
					int const r = archive_write_close(arch);

					closed = true;

					if ( r != ARCHIVE_OK )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "[E] TarWriter: failed archive_write_close: " << archive_error_string(arch) << std::endl;
						lme.finish();
						throw lme;
					}
				}
			}

			std::string getUserName()
			{
				uint64_t n = 1024;

				while ( true )
				{
					struct passwd pwd;
					struct passwd * rpwd = 0;

					libmaus2::autoarray::AutoArray<char> C(n);

					int const r = getpwuid_r(uid,&pwd,C.begin(),C.size(),&rpwd);

					if ( r != 0 )
					{
						int const error = errno;

						switch ( error )
						{
							case EINTR:
							case EAGAIN:
								break;
							case ERANGE:
								n *= 2;
								break;
							default:
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "[E] TarWriter: unable to get user name: " << strerror(error) << std::endl;
								lme.finish();
								throw lme;
							}
						}
					}
					else
					{
						if ( rpwd )
						{
							return rpwd->pw_name;
						}
						else
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "[E] TarWriter: unable to get user name" << std::endl;
							lme.finish();
							throw lme;
						}
					}
				}
			}

			std::string getGroupName()
			{
				uint64_t n = 1024;

				while ( true )
				{
					struct group pwd;
					struct group * rpwd = 0;

					libmaus2::autoarray::AutoArray<char> C(n);

					int const r = getgrgid_r(gid,&pwd,C.begin(),C.size(),&rpwd);

					if ( r != 0 )
					{
						int const error = errno;

						switch ( error )
						{
							case EINTR:
							case EAGAIN:
								break;
							case ERANGE:
								n *= 2;
								break;
							default:
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "[E] TarWriter: unable to get group name: " << strerror(error) << std::endl;
								lme.finish();
								throw lme;
							}
						}
					}
					else
					{
						if ( rpwd )
						{
							return rpwd->gr_name;
						}
						else
						{
							libmaus2::exception::LibMausException lme;
							lme.getStream() << "[E] TarWriter: unable to get group name" << std::endl;
							lme.finish();
							throw lme;
						}
					}
				}
			}

			void addFile(std::string const & filename, std::string const & data, ::std::size_t const writeblocksize = 64*1024)
			{
				struct archive_entry * entry = archive_entry_new();

				if ( ! entry )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "[E] TarWriter: archive_entry_new failed" << std::endl;
					lme.finish();
					throw lme;
				}

				time_t const tim = ::time(NULL);

				archive_entry_set_pathname(entry, filename.c_str());
				archive_entry_set_size(entry, data.size());
				archive_entry_set_filetype(entry, AE_IFREG);
				archive_entry_set_perm(entry, 0644);

				archive_entry_set_mtime(entry,tim,0);
				archive_entry_set_atime(entry,tim,0);
				archive_entry_set_ctime(entry,tim,0);
				archive_entry_set_uid(entry,uid);
				archive_entry_set_gid(entry,gid);
				archive_entry_set_uname(entry,username.c_str());
				archive_entry_set_gname(entry,groupname.c_str());

				if ( archive_write_header(arch, entry) != ARCHIVE_OK )
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "[E] TarWriter: archive_write_header failed: " << archive_error_string(arch) << std::endl;
					lme.finish();
					throw lme;
				}

				char const * pc = data.c_str();
				char const * pe = pc + data.size();

				while ( pc != pe )
				{
					::std::size_t r = pe - pc;
					::std::size_t towrite = std::min(r,writeblocksize);

					if ( archive_write_data(arch, pc, towrite) != static_cast< ::ssize_t>(towrite) )
					{
						libmaus2::exception::LibMausException lme;
						lme.getStream() << "[E] TarWriter: archive_write_data failed: " << archive_error_string(arch) << std::endl;
						lme.finish();
						throw lme;
					}

					pc += towrite;
				}

				archive_entry_free(entry);
			}

			~TarWriter()
			{
				if ( ! closed )
				{
					try
					{
						close();
					}
					catch(std::exception const & ex)
					{
						libmaus2::parallel::ScopePosixSpinLock slock(libmaus2::aio::StreamLock::cerrlock);
						std::cerr << ex.what() << std::endl;
					}
				}

				archive_write_free(arch);
			}
			#else
			TarWriter(std::string const &)
			: uid(getuid()), gid(getgid())
			{
				libmaus2::exception::LibMausException lme;
				lme.getStream() << "[E] libmaus2 is not compiled with support for libarchive" << std::endl;
				lme.finish();
				throw lme;
			}
			void close() {}
			void addFile(std::string const &, std::string const &, ::std::size_t const = 64*1024) {}
			#endif
		};
	}
}
#endif
