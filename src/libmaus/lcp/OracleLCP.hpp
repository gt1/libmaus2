/**
    libmaus
    Copyright (C) 2008 Simon Puglisi
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

/*
 * This file contains a slight modification of Simon Puglisi's code for oracle based
 * LCP computation as presented in
 * Simon J. Puglisi & Andrew Turpin, Space-time tradeoffs for longest-common-prefix array computation,
 * ISAAC 2008
 * Original code by Simon Puglisi, added bugs by German Tischler
 */
 
#if ! defined(DC_LCP_ORACLE_HPP)
#define DC_LCP_ORACLE_HPP

#include <cstdio>
#include <cstdlib>
#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/rmq/FischerSystematicSuccinctRMQ.hpp>
#include <stdexcept>
#include <libmaus/exception/LibMausException.hpp>

namespace libmaus
{
   namespace lcp
   {
      struct OracleLCPBase
      {
         protected:
         static const uint32_t * _covers[];
         static const uint32_t max_precomputed_cover;
         static const uint32_t _cover_sizes[];
      };
      
      template<typename value_type>
      struct DefaultComparator
      {
         static bool equal(value_type a, value_type b) { return a == b; }
         static bool unequal(value_type a, value_type b) { return a != b; }
      };

      template<typename value_type>
      struct ComparatorZeroUnequal
      {
         static bool equal(value_type a, value_type b) { return a == b && (a != 0); }
         static bool unequal(value_type a, value_type b) { return !equal(a,b); }
      };
   
      template<
         typename iterator_type,
         template<class> class rmq_type_template,
         typename comparator_type = DefaultComparator < typename std::iterator_traits<iterator_type>::value_type >
      >
      struct OracleLCP : public OracleLCPBase
      {
         private:
         typedef uint32_t ell_type;
         typedef rmq_type_template<ell_type const *> rmq_type;
         typedef typename rmq_type::unique_ptr_type rmq_ptr_type;

         iterator_type const _x;
         ell_type * const _sa;
         uint32_t const _n;
         uint32_t const _logv;
         uint32_t const _v;
         uint32_t const _mask;
         uint32_t const * const _cover;
         uint32_t const _cover_size;
         uint32_t _m;
         
         /* size <= 256 for _logv <= 8 */
         ::libmaus::autoarray::AutoArray<int32_t> _rev_cover;
         /* _delta <= 256 for _logv <= 8 */ 
         ::libmaus::autoarray::AutoArray<uint32_t> _delta;
         /* _n/256 * 20 for _logv=8 equals 0.3125 bytes per symbol */
         ::libmaus::autoarray::AutoArray<uint32_t> _ess;
         /* _n/256 * 20 for _logv=8 equals 0.3125 bytes per symbol */
         ::libmaus::autoarray::AutoArray<uint32_t> _rev_ess;

         /* _n/256 * 20 for _logv=8 equals 0.3125 bytes per symbol */
         ::libmaus::autoarray::AutoArray< ell_type > _ell;
         /* _n/32 * 2 + o(n) = .0625 bytes per symbol */
         rmq_ptr_type _rmq;

         uint32_t delta(uint32_t i, uint32_t j)
         {
            return ((_delta[(j-i)%_v]-i)%_v);
         }

         OracleLCP(iterator_type const a_x, ell_type * a_sa, uint32_t a_n, uint32_t a_logv)
         : _x(a_x), _sa(a_sa), _n(a_n), _logv(a_logv), _v (1 << _logv), _mask(_v - 1),
           _cover (_covers[_logv]), _cover_size ( _cover_sizes[_logv] ),
            _m ( (_n/_v) * _cover_size ),
            _rev_cover(_v), _delta(_v)
         {
            if(_logv > max_precomputed_cover)
            {
               ::libmaus::exception::LibMausException se;
               se.getStream() << "Specified DC ("<< _logv << ") greater than max ("<<max_precomputed_cover<<")";
               se.finish();
               throw se;
            }
            
            //fprintf(stderr,"_m = %d\n",_m);
            //fprintf(stderr,"_v = %d\n",_v);
          
            //compute _rev_cover
            uint32_t j = 0;
            for(uint32_t i = 0; i < _v; i++)
            {
               _rev_cover[i] = -1;
               if(_cover[j] == i)
               {
                  _rev_cover[i] = j;
                  if(j < (_n & _mask))
                  {
                     _m++;
                  }
                  j++;
               }
            }
            
            #if 0
            uint64_t ___m = 0;
            for ( uint64_t i = 0; i < _n; ++i )
            {
               uint32_t const si = _sa[i];
               if ( _rev_cover[ si & _mask ] != -1 )
                  ++___m;
            }
            
            std::cerr << "_m=" << _m << " ___m=" << ___m << std::endl;
            #endif

            //compute _delta
            for (int32_t i = _cover_size-1; i >= 0; i--) {
               for (j = 0; j < _cover_size; j++) {
                  _delta[(_cover[j]-_cover[i])%_v] = _cover[i];
               }
            }

            //compute arrays _ess, _rev_ess
            _ess = ::libmaus::autoarray::AutoArray<uint32_t>(_m);
            _rev_ess = ::libmaus::autoarray::AutoArray<uint32_t>(_m);
            j = 0;
            for(uint32_t i = 0; i < _n; i++)
            {
               uint32_t si = _sa[i];
               if(_rev_cover[si&_mask] != -1)
               {
                  //this is a sample suffix
                  // assert ( j < _m );
                  _ess[j] = si; // crash
                  _rev_ess[(_cover_size*(si>>_logv)) + _rev_cover[si&_mask]] = j;
                  j++;
               }
            }
            //fprintf(stderr,"j = %d\n",j);

            //compute _ell using _ess, _rev_ess, _rev_cover
            _ell = ::libmaus::autoarray::AutoArray<ell_type>(_m,true);
            _ell[0] = 0;
            int32_t len = 0;
            uint32_t computed = 0;
            ::libmaus::autoarray::AutoArray<int> lengths(_cover_size);

            for(uint32_t i = 0; i < _cover_size; i++){
               lengths[i] = 0;
            }
            uint32_t compares_saved = 0;
            for(uint32_t i = 0; i < _m; i++){
               //if(i % 1000000 == 0){
               //   fprintf(stderr,"Processed %d entires\n",i);
               //}
               uint32_t const ihat = _rev_ess[i];
               len = lengths[i%_cover_size];
               if(len < 0) len = 0;
               compares_saved += len;
               if(ihat > 0){
                  uint32_t j = _ess[ihat-1];
                  while(_ess[ihat]+len < _n && j+len < _n){
                     if( comparator_type::unequal(_x[_ess[ihat]+len],_x[j+len]) ){
                        break;
                     }
                     len++;
                  }
               }
               _ell[ihat] = len;
               uint32_t const a = _rev_cover[(_ess[ihat])&_mask];
               lengths[a] = len - _v;
               //#define CHECK_SAMPLE_LCP
               #if defined(CHECK_SAMPLE_LCP)
               int l = 0;
               while(
                 (_ess[ihat]+l < _n)
                 &&
                 (_ess[ihat-1]+l < _n)
                 &&
                 ( comparator_type::equal(_x[_ess[ihat]+l],_x[_ess[ihat-1]+l]) )
               )
               {
                  l++;
               }
               if(l != len){
                  fprintf(stderr,"mismatch at i=%d, (%d,%d)\n",i,l,len);
                  exit(1);
               }else{
               }
               #endif
               computed++;
            }
            //fprintf(stderr,"computed %d lcp values\n",computed);
            //fprintf(stderr,"compares_saved = %d\n",compares_saved);

            //we no longer need _ess, free it
            _ess.release();
            lengths.release();

            //preprocess _ell for RMQs
            _rmq = UNIQUE_PTR_MOVE(rmq_ptr_type ( new rmq_type(_ell.get(),_m) ));
         }

         void dc_lcp_construct(){
            ell_type *_lcp = _sa;
            uint32_t longcount = 0;
            uint32_t shortcount = 0;
            uint32_t maxlcp = 0;
            
            // uint32_t s0 = _sa[0];
            
            for(uint32_t i = 1; i < _n; i++){
               // uint32_t const s1 = _sa[i];
            
               //if(i % 10000000 == 0){
               //   fprintf(stderr,"Processed %d entires\n",i);
               //}
               #if 1
               uint32_t const s0 = _sa[i-1];
               uint32_t const s1 = _sa[i];
               #endif
               
               //check if lcp(SA[i-1],SA[i]) < v
               uint32_t j = 0;
               while(
                 (s0+j < _n)
                 &&
                 (s1+j < _n)
                 &&
                 ( comparator_type::equal(_x[s0+j],_x[s1+j]))
                 && 
                 (j < _v)
               )
               {
                  j++;
               }
               if(j < _v)
               {
                  shortcount++;
                  _lcp[i-1] = j;
               }
               else
               {
                  longcount++;
                  uint32_t const ds0s1 = delta(s0,s1);
                  uint32_t const a0 = s0 + ds0s1;
                  uint32_t const a1 = s1 + ds0s1;
                  //fprintf(stderr,"s0 = %d; s1 = %d; ds0s1 = %d; a0 = %d; a1 = %d\n",s0,s1,ds0s1,a0,a1);
                  uint32_t const r0 = _rev_ess[(_cover_size*(a0>>_logv)) + _rev_cover[a0&_mask]];
                  uint32_t const r1 = _rev_ess[(_cover_size*(a1>>_logv)) + _rev_cover[a1&_mask]];
                  uint32_t const rmin_auto = _ell[( (*_rmq)(r0+1,r1))];
                  _lcp[i-1] = ds0s1 + rmin_auto;
                  if(_lcp[i-1] > maxlcp)
                  {
                     maxlcp = _lcp[i-1];
                  }
                  //#define CHECK_LCP
                  #if defined(CHECK_LCP)
                  int j = 0;
                  while( comparator_type::equal(_x[s0+j],_x[s1+j])){
                     j++;
                  }
                  if(j != _lcp[i-1]){
                     fprintf(stderr,"mismatch at position %d (%d %d); r0 = %d, r1 = %d\n",i,j,_lcp[i-1],r0,r1);
                     int j = 0;
                     while( comparator_type::equal(_x[a0+j], _x[a1+j])){
                        j++;
                     }
                     fprintf(stderr,"ds0s1 = %d; a0 = %d; a1 = %d; j = %d\n",ds0s1,a0,a1,j);
                  }
                  #endif
               }
               
               // s0 = s1;
            }
            //fprintf(stderr,"maxlcp = %d\n",maxlcp);
            //fprintf(stderr,"longcount = %d; shortcount = %d\n",longcount,shortcount);
         }

         public:
         static void dc_lcp_compute(iterator_type const x, uint32_t *SA, uint32_t n, int logv)
         {
            OracleLCP oracle(x,SA,n,logv);
            oracle.dc_lcp_construct();
            
            if ( n )
            {
               for ( uint32_t i = n-1; i != 0; --i )
                  SA[i] = SA[i-1];
               SA[0] = 0;
            }
         }
      };
   }
}
#endif

