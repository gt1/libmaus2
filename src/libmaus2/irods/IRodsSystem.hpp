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

/*
 * Creation of this code used the samtools_irods source as a reference
 */
#if ! defined(LIBMAUS2_IRODS_IRODSSYSTEM_HPP)
#define LIBMAUS2_IRODS_IRODSSYSTEM_HPP

#include <libmaus2/autoarray/AutoArray.hpp>
#include <libmaus2/irods/IRodsFileBase.hpp>
#include <libmaus2/parallel/PosixMutex.hpp>

namespace libmaus2
{
	namespace irods
	{
		struct IRodsSystem : public IRodsCommProvider
		{
			typedef IRodsSystem this_type;
			typedef libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			static IRodsSystem::shared_ptr_type defaultIrodsSystem;
			static libmaus2::parallel::PosixMutex defaultIrodsSystemLock;

			static IRodsSystem::shared_ptr_type getDefaultIRodsSystem();
			static IRodsCommProvider::shared_ptr_type getDefaultIRodsSystemCommProvider()
			{
				IRodsCommProvider::shared_ptr_type tptr(getDefaultIRodsSystem());
				return tptr;
			}

			#if defined(LIBMAUS2_HAVE_IRODS)
			rodsEnv irodsEnvironment;
			rcComm_t * comm;
			sighandler_t prevpipesighandler;
			std::map<std::string, rcComm_t *> comms;
			#endif

			static IRodsFileBase::unique_ptr_type openFile(IRodsSystem::shared_ptr_type commProvider, std::string const & filename);
			static IRodsFileBase::unique_ptr_type openFile(std::string const & filename)
			{
				IRodsSystem::shared_ptr_type irodsSystem = getDefaultIRodsSystem();
				IRodsFileBase::unique_ptr_type tptr(openFile(irodsSystem,filename));
				return UNIQUE_PTR_MOVE(tptr);
			}

			IRodsSystem();
			~IRodsSystem();

    	    	    	void disconnect(void);

			#if defined(LIBMAUS2_HAVE_IRODS)
			virtual rcComm_t * getComm()
			{
				return comm;
			}
			#endif

			#if defined(LIBMAUS2_HAVE_IRODS)
			private:
			    std::string setComm(std::string const & filename);
    	    	    	    std::string parseIRodsURI(std::string const & uri, std::string & host, std::string & zone,
			    	    	    	      std::string & user, int & port);
			#endif
		};
	}
}
#endif
