/*
    libmaus
    Copyright (C) 2009-2014 German Tischler
    Copyright (C) 2011-2014 Genome Research Limited

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
#if ! defined(LIBMAUS_MATH_NUMBERPARSER_HPP)
#define LIBMAUS_MATH_NUMBERPARSER_HPP

#include <libmaus/autoarray/AutoArray.hpp>
#include <libmaus/util/DigitTable.hpp>

namespace libmaus
{
	namespace math
	{
		struct DecimalNumberParser
		{
			libmaus::util::DigitTable DT;
			libmaus::autoarray::AutoArray<uint8_t> PM;
			libmaus::autoarray::AutoArray<uint8_t> M;
			
			std::vector<unsigned int> uposlentable;
			std::vector<unsigned int> iposlentable;
			std::vector<unsigned int> ineglentable;
			
			std::vector<uint64_t> udiv10;
			std::vector<uint64_t> umod10;
			std::vector<int64_t> idiv10;
			std::vector<int64_t> imod10;
			std::vector<int64_t> indiv10;
			std::vector<int64_t> inmod10;
			
			template<typename N, typename C> 
			static unsigned int getUnsignedNumLength()
			{
				std::ostringstream ostr;
				ostr << static_cast<C>(std::numeric_limits<N>::max());
				return ostr.str().size();
			}
			template<typename N, typename C> 
			static unsigned int getSignedMinLength()
			{
				std::ostringstream ostr;
				ostr << static_cast<C>(std::numeric_limits<N>::min());
				assert ( ostr.str().size() );
				assert ( ostr.str()[0] == '-' );
				return ostr.str().size()-1;
			}
			
			template<typename N, typename C>
			void insertU(std::vector<unsigned int> & V)
			{
				while ( ! (sizeof(N) < V.size()) )
					V.push_back(0);
				V [ sizeof(N) ] = getUnsignedNumLength<N,C>();
			}

			template<typename N, typename C>
			void insertN(std::vector<unsigned int> & V)
			{
				while ( ! (sizeof(N) < V.size()) )
					V.push_back(0);
				V [ sizeof(N) ] = getSignedMinLength<N,C>();
			}

			template<typename N>
			void insertDiv10(size_t const i, N const v, std::vector<N> & V)
			{
				while ( ! (i<V.size() ) )
					V.push_back(0);
				V[i] = v;
			}
			
			DecimalNumberParser()
			: PM(256,false), M(256,false)
			{
				std::fill(PM.begin(),PM.end(),0);
				PM['+'] = 1;
				PM['-'] = 1;
				std::fill(M.begin(),M.end(),0);
				M['-'] = 1;
				
				insertU<uint8_t,uint64_t> (uposlentable);
				insertU<uint16_t,uint64_t>(uposlentable);
				insertU<uint32_t,uint64_t>(uposlentable);
				insertU<uint64_t,uint64_t>(uposlentable);

				insertU< int8_t,uint64_t> (iposlentable);
				insertU< int16_t,uint64_t>(iposlentable);
				insertU< int32_t,uint64_t>(iposlentable);
				insertU< int64_t,uint64_t>(iposlentable);
				
				insertN< int8_t,int64_t> (ineglentable);
				insertN<int16_t,int64_t> (ineglentable);
				insertN<int32_t,int64_t> (ineglentable);
				insertN<int64_t,int64_t> (ineglentable);
						
				insertDiv10(sizeof(uint8_t),static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()/10),udiv10);
				insertDiv10(sizeof(uint16_t),static_cast<uint64_t>(std::numeric_limits<uint16_t>::max()/10),udiv10);
				insertDiv10(sizeof(uint32_t),static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()/10),udiv10);
				insertDiv10(sizeof(uint64_t),static_cast<uint64_t>(std::numeric_limits<uint64_t>::max()/10),udiv10);

				insertDiv10(sizeof(uint8_t),static_cast<uint64_t>(std::numeric_limits<uint8_t>::max()%10),umod10);
				insertDiv10(sizeof(uint16_t),static_cast<uint64_t>(std::numeric_limits<uint16_t>::max()%10),umod10);
				insertDiv10(sizeof(uint32_t),static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()%10),umod10);
				insertDiv10(sizeof(uint64_t),static_cast<uint64_t>(std::numeric_limits<uint64_t>::max()%10),umod10);

				insertDiv10(sizeof(int8_t), static_cast<int64_t>(std::numeric_limits<int8_t>::max()/10),idiv10);
				insertDiv10(sizeof(int16_t),static_cast<int64_t>(std::numeric_limits<int16_t>::max()/10),idiv10);
				insertDiv10(sizeof(int32_t),static_cast<int64_t>(std::numeric_limits<int32_t>::max()/10),idiv10);
				insertDiv10(sizeof(int64_t),static_cast<int64_t>(std::numeric_limits<int64_t>::max()/10),idiv10);

				insertDiv10(sizeof(int8_t), static_cast<int64_t>(std::numeric_limits<int8_t>::max()%10),imod10);
				insertDiv10(sizeof(int16_t),static_cast<int64_t>(std::numeric_limits<int16_t>::max()%10),imod10);
				insertDiv10(sizeof(int32_t),static_cast<int64_t>(std::numeric_limits<int32_t>::max()%10),imod10);
				insertDiv10(sizeof(int64_t),static_cast<int64_t>(std::numeric_limits<int64_t>::max()%10),imod10);

				insertDiv10(sizeof(int8_t), static_cast<int64_t>(std::numeric_limits<int8_t>::min()/10),indiv10);
				insertDiv10(sizeof(int16_t),static_cast<int64_t>(std::numeric_limits<int16_t>::min()/10),indiv10);
				insertDiv10(sizeof(int32_t),static_cast<int64_t>(std::numeric_limits<int32_t>::min()/10),indiv10);
				insertDiv10(sizeof(int64_t),static_cast<int64_t>(std::numeric_limits<int64_t>::min()/10),indiv10);

				insertDiv10(sizeof(int8_t),-static_cast<int64_t>(std::numeric_limits<int8_t>::min()%10),inmod10);
				insertDiv10(sizeof(int16_t),-static_cast<int64_t>(std::numeric_limits<int16_t>::min()%10),inmod10);
				insertDiv10(sizeof(int32_t),-static_cast<int64_t>(std::numeric_limits<int32_t>::min()%10),inmod10);
				insertDiv10(sizeof(int64_t),-static_cast<int64_t>(std::numeric_limits<int64_t>::min()%10),inmod10);
			}
			
			template<typename N> N parseUnsignedNumber(char const *a, char const *e) const
			{
				char const * const aa = a;
				
				if ( a == e )
				{		
					libmaus::exception::LibMausException lme;
					lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (unexpected EOS)" << '\n';
					lme.finish();
					throw lme;
				}
				
				while ( expect_false(a+1 != e && *a == '0') )
					++a;

				// length of number in decimal representation
				unsigned int const nl = e-a;
				// all symbols digits?
				bool ok = true;
								
				// if number does not reach maximal length for type
				if ( expect_true(nl < uposlentable[sizeof(N)]) )
				{
					N const d0 = *(a++);
					N v = d0-'0';
					ok = ok && DT[static_cast<uint8_t>(d0)];
					
					while ( a != e )
					{
						v *= 10;	
						N const d = *(a++);
						ok = ok && DT[static_cast<uint8_t>(d)];
						v += d - '0';
					}
					
					if ( ! ok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (invalid characters)" << '\n';
						lme.finish();
						throw lme;
					}
					
					return v;
				}
				else if ( expect_true(nl == uposlentable[sizeof(N)]) )
				{
					N const d0 = *(a++);
					N v = d0-'0';
					ok = ok && DT[static_cast<uint8_t>(d0)];
					char const * const ee = e-1;
					
					while ( a != ee )
					{
						v *= 10;	
						N const d = *(a++);
						ok = ok && DT[static_cast<uint8_t>(d)];
						v += d - '0';
					}
					
					N const d = static_cast<uint8_t>(*a);
					ok = ok && DT[d];
					
					if ( ! ok )
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (invalid characters)" << '\n';
						lme.finish();
						throw lme;			
					}
					
					if ( expect_true(v < udiv10[sizeof(N)]) )
					{
						v *= 10;
						v += d-'0';
						return v;			
					}
					else
					{
						if ( expect_true(v == udiv10[sizeof(N)] ) )
						{
							N const dd = d-'0';
							
							if ( expect_true(dd <= umod10[sizeof(N)]) )
							{		
								v *= 10;
								v += d-'0';
								return v;
							}
							else
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (number out of range for type, mod)" << '\n';
								lme.finish();
								throw lme;					
							}
						}
						else
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (number out of range for type, div)" << '\n';
							lme.finish();
							throw lme;				
						}
					}
				}
				else // nl > uposlentable[sizeof(N)]
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (number out of range for type, number is too long)" << '\n';
					lme.finish();
					throw lme;								
				}
			}
			
			template<typename N> N parseSignedNumber(char const * a, char const * e) const
			{	
				char const * const aa = a;
			
				bool neg = false;
				uint8_t s;
				if ( expect_true(a != e) && PM[s=static_cast<uint8_t>(*a)] )
				{
					neg = M[s];
					++a;
				}
				if ( expect_false(a == e) )
				{
					libmaus::exception::LibMausException lme;
					lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (unexpected EOS)" << '\n';
					lme.finish();
					throw lme;
				}
				
				while ( expect_false ( (a+1!=e) && (*a == '0') ) )
					++a;
				
				if ( neg )
				{
					// length of number in decimal representation
					unsigned int const nl = e-a;
					// all symbols digits?
					bool ok = true;
								
					// if number does not reach maximal length for type
					if ( expect_true(nl < ineglentable[sizeof(N)]) )
					{
						N const d0 = static_cast<uint8_t>(*a);
						N v = -(d0-'0');
						ok = ok && DT[d0];
						
						while ( ++a != e )
						{
							v *= 10;
							N const d = static_cast<uint8_t>(*a);
							v -= (d-'0');
							ok = ok && DT[d0];
						}
						
						if ( ! ok )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (invalid characters)" << '\n';
							lme.finish();
							throw lme;
						}
						
						return v;
					}
					else if ( expect_true(nl == iposlentable[sizeof(N)]) )
					{
						N const d0 = static_cast<uint8_t>(*a);
						N v = -(*a-'0');
						ok = ok && DT[d0];
						char const * ee = e-1;
						
						while ( ++a != ee )
						{
							v *= 10;
							N const d = static_cast<uint8_t>(*a);
							v -= (*a-'0');
							ok = ok && DT[d];
						}
						
						ok = ok && DT[static_cast<uint8_t>(*a)];
						
						if ( ! ok )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (invalid characters)" << '\n';
							lme.finish();
							throw lme;				
						}
						
						assert ( a + 1 == e );

						if ( expect_false(v <= indiv10[sizeof(N)]) )
						{
							if ( expect_false(v < indiv10[sizeof(N)]) )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (number out range for type, prefix " << static_cast<int64_t>(v) << " min " << indiv10[sizeof(N)] << ")" << '\n';
								lme.finish();
								throw lme;		
							}
							else
							{
								assert ( v == indiv10[sizeof(N)] );
								
								if ( *a - '0' > inmod10[sizeof(N)] )
								{
									libmaus::exception::LibMausException lme;
									lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (number out range for type)" << '\n';
									lme.finish();
									throw lme;			
								}
								else
								{
									v *= 10;
									v -= *a - '0';
									
									return v;
								}
							}
						}
						else
						{
							v *= 10;
							v -= *a - '0';
							
							return v;
						}
					}
					else
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (too long for type)" << '\n';
						lme.finish();
						throw lme;
					}
				}
				else
				{
					// length of number in decimal representation
					unsigned int const nl = e-a;
					// all symbols digits?
					bool ok = true;
					
					// if number does not reach maximal length for type
					if ( expect_true(nl < iposlentable[sizeof(N)]) )
					{
						N const d0 = static_cast<uint8_t>(*a);
						N v = d0-'0';
						ok = ok && DT[d0];
						
						while ( ++a != e )
						{
							v *= 10;
							N const d = static_cast<uint8_t>(*a);
							v += (d-'0');
							ok = ok && DT[d0];
						}
						
						if ( ! ok )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (invalid characters)" << '\n';
							lme.finish();
							throw lme;
						}
						
						return v;
					}
					else if ( expect_true(nl == iposlentable[sizeof(N)]) )
					{	
						N const d0 = static_cast<uint8_t>(*a);
						N v = *a-'0';
						ok = ok && DT[d0];
						char const * ee = e-1;
						
						while ( ++a != ee )
						{
							v *= 10;
							N const d = static_cast<uint8_t>(*a);
							v += (*a-'0');
							ok = ok && DT[d];
						}
						
						ok = ok && DT[static_cast<uint8_t>(*a)];
						
						if ( ! ok )
						{
							libmaus::exception::LibMausException lme;
							lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (invalid characters)" << '\n';
							lme.finish();
							throw lme;				
						}
						
						assert ( a + 1 == e );

						if ( expect_false(v >= idiv10[sizeof(N)]) )
						{
							if ( expect_false(v > idiv10[sizeof(N)]) )
							{
								libmaus::exception::LibMausException lme;
								lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (number out range for type, div)" << '\n';
								lme.finish();
								throw lme;		
							}
							else
							{
								assert ( v == idiv10[sizeof(N)] );
								
								if ( *a - '0' > imod10[sizeof(N)] )
								{
									libmaus::exception::LibMausException lme;
									lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (number out range for type, mod)" << '\n';
									lme.finish();
									throw lme;			
								}
								else
								{
									v *= 10;
									v += *a - '0';
									
									return v;
								}
							}
						}
						else
						{
							v *= 10;
							v += *a - '0';
							
							return v;
						}
					}
					else
					{
						libmaus::exception::LibMausException lme;
						lme.getStream() << "DecimalNumberParser: cannot parse " << std::string(aa,e) << " (too long for type)" << '\n';
						lme.finish();
						throw lme;		
					
					}
				}
			}
		};
	}
}
#endif
