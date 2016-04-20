/*
    libmaus2
    Copyright (C) 2009-2015 German Tischler
    Copyright (C) 2011-2015 Genome Research Limited

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
#if ! defined(LIBMAUS2_AIO_POSIXFDOUTPUTSTREAMFACTORY_HPP)
#define LIBMAUS2_AIO_POSIXFDOUTPUTSTREAMFACTORY_HPP

#include <libmaus2/aio/OutputStreamFactory.hpp>
#include <libmaus2/aio/PosixFdOutputStream.hpp>

#include <sys/stat.h>
#include <sys/types.h>

namespace libmaus2
{
	namespace aio
	{
		struct PosixFdOutputStreamFactory : public libmaus2::aio::OutputStreamFactory
		{
			typedef PosixFdOutputStreamFactory this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			virtual ~PosixFdOutputStreamFactory() {}
			virtual libmaus2::aio::OutputStream::unique_ptr_type constructUnique(std::string const & filename)
			{
				if ( filename == "-" )
				{
					libmaus2::util::shared_ptr<std::ostream>::type iptr(new PosixFdOutputStream(STDOUT_FILENO));
					libmaus2::aio::OutputStream::unique_ptr_type istr(new libmaus2::aio::OutputStream(iptr));
					return UNIQUE_PTR_MOVE(istr);
				}
				else
				{
					libmaus2::util::shared_ptr<std::ostream>::type iptr(new PosixFdOutputStream(filename));
					libmaus2::aio::OutputStream::unique_ptr_type istr(new libmaus2::aio::OutputStream(iptr));
					return UNIQUE_PTR_MOVE(istr);
				}
			}
			virtual libmaus2::aio::OutputStream::shared_ptr_type constructShared(std::string const & filename)
			{
				if ( filename == "-" )
				{
					libmaus2::util::shared_ptr<std::ostream>::type iptr(new PosixFdOutputStream(STDOUT_FILENO));
					libmaus2::aio::OutputStream::shared_ptr_type istr(new libmaus2::aio::OutputStream(iptr));
					return istr;
				}
				else
				{
					libmaus2::util::shared_ptr<std::ostream>::type iptr(new PosixFdOutputStream(filename));
					libmaus2::aio::OutputStream::shared_ptr_type istr(new libmaus2::aio::OutputStream(iptr));
					return istr;
				}
			}
			virtual void rename(std::string const & from, std::string const & to)
			{
				if ( ::rename(from.c_str(),to.c_str()) == -1 )
				{
					int const error = errno;
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "PosixFdOutputStreamFactory::rename(" << from << "," << to << "): " << strerror(error) << std::endl;
					lme.finish();
					throw lme;
				}
			}
			virtual void mkdir(std::string const & name, uint64_t const mode)
			{
				int r = -1;

				while ( r != 0 )
				{
					r = ::mkdir(name.c_str(),mode);

					if ( r != 0 )
					{
						int const error = errno;

						switch ( error )
						{
							case EEXIST:
							{
								struct stat sb;
								int rstat = -1;

								while ( (rstat = ::stat(name.c_str(),&sb)) < 0 )
								{
									int const error = errno;
									switch ( error )
									{
										case EAGAIN:
											break;
										default:
										{
											libmaus2::exception::LibMausException lme;
											lme.getStream() << "PosixFdOutputStreamFactory::mkdir(" << name << "," << std::oct << mode << std::dec << "): " << strerror(error) << std::endl;
											lme.finish();
											throw lme;
										}
									}
								}

								if ( S_ISDIR(sb.st_mode) )
									r = 0;
								else
								{
									libmaus2::exception::LibMausException lme;
									lme.getStream() << "PosixFdOutputStreamFactory::mkdir(" << name << "," << std::oct << mode << std::dec << "): exists but is not a directory" << std::endl;
									lme.finish();
									throw lme;
								}

								break;
							}
							case EAGAIN:
								break;
							default:
							{
								libmaus2::exception::LibMausException lme;
								lme.getStream() << "PosixFdOutputStreamFactory::mkdir(" << name << "," << std::oct << mode << std::dec << "): " << strerror(error) << std::endl;
								lme.finish();
								throw lme;
							}
						}
					}
				}
			}
		};
	}
}
#endif
