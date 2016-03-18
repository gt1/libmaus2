/*
    libmaus2
    Copyright (C) 2009-2016 German Tischler
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
#if ! defined(LIBMAUS2_AIO_ARRAYFILECONTAINER_HPP)
#define LIBMAUS2_AIO_ARRAYFILECONTAINER_HPP

#include <libmaus2/aio/ArrayInputStream.hpp>
#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/parallel/PosixMutex.hpp>
#include <map>

namespace libmaus2
{
	namespace aio
	{
		template<typename _iterator>
		struct ArrayFileContainer
		{
			typedef _iterator iterator;
			typedef ArrayFileContainer<iterator> this_type;
			typedef typename libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			libmaus2::parallel::PosixMutex lock;
			std::map < std::string, std::pair<iterator,iterator> > M;

			void add(std::string const & name, iterator ita, iterator ite)
			{
				libmaus2::parallel::ScopePosixMutex slock(lock);
				M [ name ] = std::pair<iterator,iterator>(ita,ite);
			}

			void list(std::vector<std::string> & V)
			{
				libmaus2::parallel::ScopePosixMutex slock(lock);
				V.resize(M.size());
				uint64_t o = 0;
				for ( typename std::map < std::string, std::pair<iterator,iterator> >::const_iterator ita = M.begin();
					ita != M.end(); ++ita )
					V[o++] = ita->first;
			}

			uint64_t list(std::vector< std::pair<std::string,uint64_t> > & V)
			{
				libmaus2::parallel::ScopePosixMutex slock(lock);
				uint64_t acc = 0;
				V.resize(M.size());
				uint64_t o = 0;
				for ( typename std::map < std::string, std::pair<iterator,iterator> >::const_iterator ita = M.begin();
					ita != M.end(); ++ita )
				{
					uint64_t const s = ita->second.second - ita->second.first;
					acc += s;
					V[o++] = std::pair<std::string,uint64_t>(ita->first,s);
				}
				return acc;
			}

			void rename(std::string const & from, std::string const & to)
			{
				typename std::map < std::string, std::pair<iterator,iterator> >::iterator it = M.find(from);

				if ( it != M.end() )
				{
					std::pair<iterator,iterator> file = it->second;
					M.erase(it);
					M[to] = file;
				}
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::aio::ArrayFileContainer::rename(" << from << "," << to << "): file " << from << " does not exist" << std::endl;
					lme.finish();
					throw lme;
				}
			}

			bool hasEntry(std::string const & name)
			{
				libmaus2::parallel::ScopePosixMutex slock(lock);
				return M.find(name) != M.end();
			}

			typename libmaus2::aio::ArrayInputStream<iterator>::shared_ptr_type getEntryIfExists(std::string const & name)
			{
				libmaus2::parallel::ScopePosixMutex slock(lock);

				typename std::map < std::string, std::pair<iterator,iterator> >::iterator ita = M.find(name);

				if ( ita != M.end() )
				{
					typename libmaus2::aio::ArrayInputStream<iterator>::shared_ptr_type ptr(new libmaus2::aio::ArrayInputStream<iterator>(ita->second.first,ita->second.second));
					return ptr;
				}
				else
				{
					libmaus2::exception::LibMausException lme;
					lme.getStream() << "libmaus2::aio::ArrayFileContainer::getEntryIfExists(): file " << name << " does not exist." << std::endl;
					lme.finish();
					throw lme;
				}
			}

			typename libmaus2::aio::ArrayInputStream<iterator>::shared_ptr_type getEntry(std::string const & name)
			{
				return getEntryIfExists(name);
			}

			void eraseEntry(std::string const & name)
			{
				libmaus2::parallel::ScopePosixMutex slock(lock);

				typename std::map < std::string, std::pair<iterator,iterator> >::iterator ita = M.find(name);

				if ( ita != M.end() )
					M.erase(ita);
			}
		};
	}
}
#endif
