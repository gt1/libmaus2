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
#if ! defined(LIBMAUS2_LCP_LCP_HPP)
#define LIBMAUS2_LCP_LCP_HPP

#include <libmaus2/autoarray/AutoArray.hpp>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <libmaus2/aio/SynchronousGenericInput.hpp>
#include <libmaus2/bitio/FastWriteBitWriter.hpp>
#include <libmaus2/bitio/putBit.hpp>
#include <libmaus2/fm/SampledSA.hpp>
#include <libmaus2/fm/SampledISA.hpp>
#include <libmaus2/lf/LF.hpp>
#include <libmaus2/timing/RealTimeClock.hpp>
#include <libmaus2/bitio/CompactQueue.hpp>
#include <libmaus2/util/NumberSerialisation.hpp>
#include <libmaus2/util/TempFileContainer.hpp>

namespace libmaus2
{
	namespace lcp
	{
		// compute plcp	array
		template<typename key_type, typename elem_type>
		::libmaus2::autoarray::AutoArray<elem_type> plcp(key_type const * t, size_t const n, elem_type const * const SA)
		{
			// compute Phi
			::libmaus2::autoarray::AutoArray<elem_type> APhi(n,false); elem_type * Phi = APhi.get();
			if ( n )
				Phi[SA[0]] = n;
			for ( size_t i = 1; i < n; ++i )
				Phi[SA[i]] = SA[i-1];
			  
			unsigned int l = 0;
			  
			::libmaus2::autoarray::AutoArray<elem_type> APLCP(n,false); elem_type * PLCP = APLCP.get();
			key_type const * const tn = t+n;
			for ( size_t i = 0; i < n; ++i )
			{
				unsigned int const j = Phi[i];
			    
				key_type const * ti = t+i+l;
				key_type const * tj = t+j+l;
			    
				if ( j < i ) while ( (ti != tn) && (*ti == *tj) ) ++l, ++ti, ++tj;
				else         while ( (tj != tn) && (*ti == *tj) ) ++l, ++ti, ++tj;
			    
				PLCP[i] = l;
				if ( l >= 1 )
					l -= 1;
				else
					l = 0;
			}
			  
			return APLCP;
		}

		/**
		 * compute lcp array using plcp array
		 **/
		template<typename key_type, typename elem_type>
		::libmaus2::autoarray::AutoArray<elem_type> computeLcp(
			key_type const * t, 
			size_t const n, 
			elem_type const * const SA
		)
		{
			::libmaus2::autoarray::AutoArray<elem_type> APLCP = plcp(t,n,SA); elem_type const * const PLCP = APLCP.get();
			::libmaus2::autoarray::AutoArray<elem_type> ALCP(n,false); elem_type * LCP = ALCP.get();
			  
			for ( size_t i = 0; i < n; ++i )
				LCP[i] = PLCP[SA[i]];
			  
			return ALCP;
		}

		/**
		 * compute LCP using linear time algorithm by Kasai et al
		 **/
		template<typename key_string, typename sa_elem_type, typename lcp_elem_type>
		void computeLCPKasai(key_string const & y, uint64_t const n, sa_elem_type const * const sa, lcp_elem_type * lcp)
		{
			::libmaus2::autoarray::AutoArray<sa_elem_type> isa(n);

			for ( uint64_t i = 0; i < n; ++i ) isa[sa[i]] = i;
		
			lcp_elem_type l = 0;
		
			for ( uint64_t j = 0; j < n; ++j )
			{
				if ( l > 0 ) l = l-1;
				sa_elem_type const i = isa[j];
			
				if ( i != 0 )
				{
					sa_elem_type jp = sa[i-1];
					
					if ( static_cast<sa_elem_type>(j) < jp )
						while ( (jp+l<static_cast<sa_elem_type>(n)) && (y[j+l]==y[jp+l]) )
							l++;
					else
						while ( (j+l<n) && (y[j+l]==y[jp+l]) )
							l++;
				}
				else
				{
					l = 0;
				}
				lcp[i] = l;
			}        
		}
	}
}
#endif
