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
#if ! defined(TRIESTATE_HPP)
#define TRIESTATE_HPP

#include <libmaus2/util/shared_ptr.hpp>
#include <libmaus2/util/unique_ptr.hpp>
#include <libmaus2/util/SimpleHashMap.hpp>
#include <libmaus2/math/ilog.hpp>
#include <libmaus2/math/lowbits.hpp>
#include <vector>
#include <map>
#include <ostream>
#include <deque>
#include <stack>
#include <set>

namespace libmaus2
{
	namespace trie
	{
		template<typename _char_type>
		struct TrieState
		{
			typedef _char_type char_type;
			typedef TrieState<char_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::map < char_type, shared_ptr_type > TT;
			#if defined(TRIESTATET)
			std::map < char_type, this_type * > T;
			#endif
			this_type * F;
			bool final;
			std::set<uint64_t> ids;

			TrieState(bool const rfinal = false)
			: F(0), final(rfinal)
			{

			}

			static shared_ptr_type construct()
			{
				shared_ptr_type state(new this_type);
				return state;
			}

			this_type * addTrieTransition(char_type const c)
			{
				shared_ptr_type newState = construct();
				TT [ c ] = newState;
				#if defined(TRIESTATET)
				T  [ c ] = newState.get();
				#endif
				return newState.get();
			}

			this_type * addTrieTransitionIfNotExists(char_type const c)
			{
				typename std::map < char_type, shared_ptr_type >::const_iterator ita = TT.find(c);

				if ( ita == TT.end() )
				{
					return addTrieTransition(c);
				}
				else
				{
					return ita->second.get();
				}
			}
		};

		template<typename char_type>
		inline std::ostream & operator<<(std::ostream & out, TrieState<char_type> const & TS)
		{
			out << "TrieState(this="<< &TS <<",F=" << TS.F << ",";

			for ( typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_iterator ita = TS.TT.begin();
				ita != TS.TT.end(); ++ita )
				out << "[" << ita->first << "->" << (*(ita->second)) << "]";

			out << ")";

			return out;
		}

		template<typename _char_type>
		struct LinearTrieStateBase
		{
			typedef _char_type char_type;
			typedef LinearTrieStateBase<char_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			int64_t F;
			bool final;
			std::vector<uint64_t> ids;
			int64_t parent;
			int64_t depth;

			LinearTrieStateBase(bool const rfinal = false)
			: F(-1), final(rfinal), parent(-1), depth(0)
			{

			}


			void setFail(int64_t const rF)
			{
				F = rF;
			}

		};

		template<typename _char_type>
		struct LinearTrieState : public LinearTrieStateBase<_char_type>
		{
			typedef _char_type char_type;
			typedef LinearTrieState<char_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::map < char_type, uint64_t > TT;

			LinearTrieState(bool const rfinal = false)
			: LinearTrieStateBase<char_type>(rfinal)
			{

			}

			void addTransition(char_type const c, uint64_t const to)
			{
				TT[c] = to;
			}

		};

		template<typename _char_type>
		struct LinearTrie
		{
			typedef _char_type char_type;
			typedef LinearTrie<char_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			std::vector < LinearTrieState<char_type> > V;

			LinearTrie(std::vector < LinearTrieState<char_type> > rV = std::vector < LinearTrieState<char_type> > ())
			: V(rV) {}
		};

		template<typename _char_type, typename _id_type>
		struct LinearHashTrie
		{
			typedef _char_type char_type;
			typedef _id_type id_type;
			typedef LinearHashTrie<char_type,id_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;

			private:
			LinearHashTrie()
			: V(), H()
			{
			}

			public:
			std::vector < LinearTrieStateBase<char_type> > V;
			typename ::libmaus2::util::SimpleHashMap<uint64_t,id_type>::unique_ptr_type H;

			size_t byteSize()
			{
				return V.size() * sizeof(LinearTrieStateBase<char_type>) +
					sizeof(H) + (H?H->byteSize() : 0);
			}

			unique_ptr_type uclone() const
			{
				unique_ptr_type O(new this_type);
				O->V = V;
				O->H = UNIQUE_PTR_MOVE(H->uclone());
				return UNIQUE_PTR_MOVE(O);
			}

			shared_ptr_type sclone() const
			{
				shared_ptr_type O(new this_type);
				O->V = this->V;
				typename ::libmaus2::util::SimpleHashMap<uint64_t,id_type>::unique_ptr_type tOH(this->H->uclone());
				O->H = UNIQUE_PTR_MOVE(tOH);
				return O;
			}

			static unsigned int ensureSize(uint64_t const numedges, unsigned int log)
			{
				while ( numedges > (1ull << log) )
					++log;

				// std::cerr << "numedges " << numedges << " space " << (1ull << log) << std::endl;

				return log;
			}

			LinearHashTrie(uint64_t const rnumedges, double const lf = 0.7)
			: V(), H(new ::libmaus2::util::SimpleHashMap<uint64_t,id_type>(ensureSize(rnumedges,::libmaus2::math::ilog( static_cast<uint64_t>( rnumedges * (1.0/lf)) ) ) ))
			{}

			private:
			LinearHashTrie & operator=(LinearHashTrie const & /* o */)
			{
				return this;
			}


			public:
			static uint64_t pairToHashValue(uint64_t const from, char_type const & c)
			{
				return (from<<32)|c;
			}

			void addTransition(uint64_t const from, char_type const & c, uint64_t const to)
			{
				H -> insert ( pairToHashValue(from,c), to );
			}

			void fillDepthParent()
			{
				std::vector < uint64_t > edges;

				for (
					typename ::libmaus2::util::SimpleHashMap<uint64_t,id_type>::pair_type const * P = H->begin();
					P != H->end();
					++P )
					if ( P->first != ::libmaus2::util::SimpleHashMap<uint64_t,id_type>::unused() )
					{
						uint64_t const from = (P->first >> 32);
						// uint64_t const c = P->first & 0xFFFFFFFFull;
						uint64_t const to = P->second;

						V[to].parent = from;

						edges.push_back(P->first);
					}

				std::sort(edges.begin(),edges.end());

				for ( uint64_t i = 0; i < edges.size(); ++i )
				{
					uint64_t const from = (edges[i] >> 32);

					if ( V[from].parent != -1 )
						V[from].depth = V[V[from].parent].depth+1;
				}
			}

			bool hasTransition(uint64_t const from, char_type const & c) const
			{
				return H->contains(pairToHashValue(from,c));
			}

			uint64_t targetByFailure(int64_t cur, char_type const c) const
			{
				typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_iterator ita;

				while ( (cur != -1) && (!hasTransition(cur,c)) )
					cur = V[cur].F;
				if ( cur < 0 )
					return 0;
				else
					return H->get(pairToHashValue(cur,c));
			}

			template<typename iterator>
			int64_t searchCompleteNoFailure(iterator ita, iterator ite) const
			{
				int64_t cur = 0;

				for ( iterator itc = ita; (cur != -1) && itc != ite; ++itc )
					cur = hasTransition(cur,*itc) ? H->get(pairToHashValue(cur,*itc)) : -1;

				if ( cur != -1 && cur < static_cast<int64_t>(V.size()) && V[cur].final && V[cur].ids.size() == 1 )
					return V[cur].ids[0];
				else
					return -1;
			}

			template<typename iterator>
			int64_t searchCompleteNoFailureZ(iterator ita) const
			{
				int64_t cur = 0;

				for ( iterator itc = ita; (cur != -1) && (*itc); ++itc )
					cur = hasTransition(cur,*itc) ? H->get(pairToHashValue(cur,*itc)) : -1;

				if ( cur != -1 && cur < static_cast<int64_t>(V.size()) && V[cur].final && V[cur].ids.size() == 1 )
					return V[cur].ids[0];
				else
					return -1;
			}

			template<typename container>
			int64_t searchCompleteNoFailure(container const & cont) const
			{
				return searchCompleteNoFailure(cont.begin(),cont.end());
			}

			template<typename iterator>
			int64_t searchComplete(iterator ita, iterator ite) const
			{
				int64_t cur = 0;
				iterator itc = ita;

				while ( itc != ite )
				{
					cur = targetByFailure(cur,*(itc++));

					#if 0
					if ( V[cur].final )
						for ( uint64_t i = 0; i < V[cur].ids.size(); ++i )
							cb(itc-ita,V[cur].ids[i]);
					#endif
				}

				if ( V[cur].final )
					return cur;
				else
					return -1;
			}

			template<typename container>
			int64_t searchComplete(container const & cont) const
			{
				return searchComplete(cont.begin(),cont.end());
			}

			template<typename iterator, typename callback>
			void search(iterator ita, iterator ite, callback & cb) const
			{
				int64_t cur = 0;
				iterator itc = ita;

				if ( V[cur].final )
					for ( uint64_t i = 0; i < V[cur].ids.size(); ++i )
						cb(itc-ita,V[cur].ids[i]);

				while ( itc != ite )
				{
					cur = targetByFailure(cur,*(itc++));

					if ( V[cur].final )
						for ( uint64_t i = 0; i < V[cur].ids.size(); ++i )
							cb(itc-ita,V[cur].ids[i]);
				}
			}

			template<typename container, typename callback>
			void search(container const & C, callback & cb) const
			{
				search(C.begin(),C.end(),cb);
			}
		};

		template<typename char_type,typename id_type>
		inline std::ostream & operator<<(std::ostream & out, LinearHashTrie<char_type,id_type> const & T)
		{
			out << "LinearHashTrie(" << std::endl;

			for ( uint64_t i = 0; i < T.V.size(); ++i )
			{
				LinearTrieStateBase<char_type> const & state = T.V[i];

				out << "\t["<<i<<"]=";
				out << "LinearTrieStateBase(";

				out << "F=" << state.F;
				// out << "final=" << state.final;
				if ( state.final )
				{
					out << ",ids=[";
					for ( uint64_t j = 0; j < state.ids.size(); ++j )
						out << state.ids[j] << ";";
					out << "]";
				}
				out << ")";
				out << "\n";
			}

			out << "\t";
			for ( std::pair<uint64_t,id_type> const * ita = T.H->begin(); ita != T.H->end(); ++ita )
				 if ( ita->first != T.H->unused() )
				 {
				 	id_type const from = (ita->first) >> 32;
				 	char_type const sym = (ita->first) & ::libmaus2::math::lowbits(32);
				 	id_type const to = (ita->second);
				 	out << "(" << from << "," << sym << ")->" << to << ";";
				 }
			out << "\n";

			out << ")" << std::endl;
			return out;
		}



		template<typename char_type>
		inline std::ostream & operator<<(std::ostream & out, LinearTrie<char_type> const & T)
		{
			out << "LinearTrie(" << std::endl;

			for ( uint64_t i = 0; i < T.V.size(); ++i )
			{
				LinearTrieState<char_type> const & state = T.V[i];

				out << "\t["<<i<<"]=";
				out << "LinearTrieState(";

				out << "F=" << state.F << ",";
				out << "final=" << state.final << ",";
				out << "ids=[";
				for ( uint64_t j = 0; j < state.ids.size(); ++j )
					out << state.ids[j] << ";";
				out << "],";
				out << "[";

				for ( typename std::map < char_type, uint64_t >::const_iterator ita = state.TT.begin(); ita != state.TT.end(); ++ita )
				{
					out << ita->first << "->" << ita->second << ";";
				}

				out << "]";
				out << ")";
				out << "\n";
			}

			out << ")" << std::endl;
			return out;
		}

		template<typename _char_type>
		struct Trie;
		template<typename char_type>
		inline std::ostream & operator<<(std::ostream & out, Trie<char_type> const & T);

		template<typename _char_type>
		struct Trie
		{
			typedef _char_type char_type;
			typedef Trie<char_type> this_type;
			typedef typename ::libmaus2::util::unique_ptr<this_type>::type unique_ptr_type;
			typedef typename ::libmaus2::util::shared_ptr<this_type>::type shared_ptr_type;
			typedef TrieState<char_type> state_type;
			typedef typename state_type::shared_ptr_type state_ptr_type;
			typedef typename state_type::unique_ptr_type initial_ptr_type;

			initial_ptr_type initial;

			template<typename iterator>
			state_type * insert(iterator ita, iterator ite, int64_t const id = -1)
			{
				state_type * cur = initial.get();

				for ( ; ita != ite ; ++ita )
					cur = cur->addTrieTransitionIfNotExists(static_cast<char_type>(*ita));

				cur->final = true;

				if ( id >= 0 )
					cur->ids.insert(id);

				return cur;
			}

			template<typename container_type>
			void insert(container_type const & container, int64_t const id = -1)
			{
				insert ( container.begin(), container.end(), id );
			}

			template<typename container_iterator_type>
			void insertContainer(container_iterator_type ita, container_iterator_type ite)
			{
				for ( uint64_t id = 0 ; ita != ite; ++ita, ++id )
					insert ( *ita, id );
			}

			template<typename container_type>
			void insertContainer(container_type const & container)
			{
				insertContainer(container.begin(),container.end());
			}

			Trie()
			: initial(new state_type())
			{

			}

			void extractWordSet(
				std::vector<char_type> & V,
				std::vector < std::basic_string<char_type> > & wordset
				) const
			{
				return extractWordSet(initial.get(),V,wordset);
			}

			std::vector < std::basic_string<char_type> > extractWordSet() const
			{
				std::vector<char_type> V;
				std::vector < std::basic_string<char_type> > wordset;
				extractWordSet(V,wordset);
				return wordset;
			}

			void extractWordSet(
				state_type const * state,
				std::vector<char_type> & V,
				std::vector < std::basic_string<char_type> > & wordset
				) const
			{
				if ( state->final )
					wordset.push_back( std::basic_string<char_type>(V.begin(),V.end()) );

				for ( typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_iterator ita = state->TT.begin();
					ita != state->TT.end(); ++ita )
				{
					V.push_back(ita->first);
					extractWordSet(ita->second.get(),V,wordset);
					V.pop_back();
				}
			}

			state_type const * targetByFailure(state_type const * cur, char_type const c) const
			{
				typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_iterator ita;
				while ( cur && ((ita = cur->TT.find(c)) == cur->TT.end()) )
					cur = cur->F;
				if ( ! cur )
					return initial.get();
				else
					return ita->second.get();
			}

			state_type * targetByFailure(state_type * cur, char_type const c)
			{
				typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::iterator ita;
				while ( cur && ((ita = cur->TT.find(c)) == cur->TT.end()) )
					cur = cur->F;
				if ( ! cur )
					return initial.get();
				else
					return ita->second.get();
			}

			void fillFailureFunction()
			{
				std::deque < state_type * > F;
				F.push_back(initial.get());

				while ( F.size() )
				{
					state_type * t = F.front(); F.pop_front();

					for ( typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::iterator ita = t->TT.begin();
						ita != t->TT.end(); ++ita )
					{
						char_type const a = ita->first;
						state_type * p = ita->second.get();
						state_type * r = targetByFailure(t->F,a);
						p->F = r;
						if ( r->final )
						{
							p->final = r->final;
							for ( std::set<uint64_t>::const_iterator sita = r->ids.begin(); sita != r->ids.end(); ++sita )
								p->ids.insert(*sita);
						}
						F.push_back(p);
					}
				}

				optimiseFailureFunction();
			}

			static bool isTransSubSet(state_type const * t, state_type const * f)
			{
				bool issub = true;

				// iterate over transitions of f
				for ( typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_iterator ita = f->TT.begin();
					ita != f->TT.end(); ++ita )
					if ( t->TT.find(ita->first) == t->TT.end() )
						issub = false;

				return issub;
			}

			void optimiseFailureFunction()
			{
				std::deque < state_type * > F;
				F.push_back(initial.get());

				// traverse trie
				while ( F.size() )
				{
					state_type * t = F.front(); F.pop_front();
					state_type * f = t->F;

					while ( f && isTransSubSet(t,f) )
						f = f->F;

					t->F = f;

					for ( typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::iterator ita = t->TT.begin();
						ita != t->TT.end(); ++ita )
						F.push_back(ita->second.get());
				}
			}

			std::vector< LinearTrieState<char_type> > toLinearStateVector() const
			{
				std::stack<state_type *> S;
				S.push(initial.get());

				std::map< state_type const *, uint64_t > F;

				while ( S.size() )
				{
					state_type * state = S.top(); S.pop();
					uint64_t const id = F.size();
					F [ state ] = id;

					typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_reverse_iterator ite = state->TT.rend();
					for ( typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_reverse_iterator ita = state->TT.rbegin();
						ita != ite; ++ita )
						S.push(ita->second.get());
				}

				std::vector < LinearTrieState<char_type> > V(F.size());
				S.push(initial.get());

				while ( S.size() )
				{
					state_type * state = S.top(); S.pop();
					uint64_t const id = F.find(state)->second;

					if ( state->F )
						V[id].F = F.find(state->F)->second;
					V[id].final = state->final;

					for ( std::set<uint64_t>::const_iterator sita = state->ids.begin(); sita != state->ids.end(); ++sita )
						V[id].ids.push_back(*sita);

					typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_reverse_iterator ite = state->TT.rend();
					for ( typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_reverse_iterator ita = state->TT.rbegin();
						ita != ite; ++ita )
					{
						uint64_t const tid = F.find(ita->second.get())->second;
						V [ id ] . addTransition(ita->first,tid);
						S.push(ita->second.get());
					}
				}

				return V;
			}
			LinearTrie<char_type> toLinear() const
			{
				return LinearTrie<char_type>(toLinearStateVector());
			}

			template<typename id_type>
			typename LinearHashTrie<char_type,id_type>::unique_ptr_type toLinearHashTrie() const
			{
				// std::cerr << "Linearising " << *this << std::endl;

				std::stack<state_type *> S;
				S.push(initial.get());

				std::map< state_type const *, uint64_t > F;
				uint64_t numedges = 0;

				while ( S.size() )
				{
					state_type * state = S.top(); S.pop();
					uint64_t const id = F.size();
					F [ state ] = id;
					numedges += state->TT.size();

					typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_reverse_iterator ite = state->TT.rend();
					for ( typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_reverse_iterator ita = state->TT.rbegin();
						ita != ite; ++ita )
						S.push(ita->second.get());
				}

				// std::cerr << "Number of edges " << numedges << std::endl;

				typename LinearHashTrie<char_type,id_type>::unique_ptr_type LHT(new LinearHashTrie<char_type,id_type>(numedges));

				S.push(initial.get());

				while ( S.size() )
				{
					state_type * state = S.top(); S.pop();
					LHT->V.push_back(LinearTrieStateBase<char_type>());
					uint64_t const id = F.find(state)->second;

					if ( state->F )
						LHT->V[id].F = F.find(state->F)->second;
					LHT->V[id].final = state->final;

					for ( std::set<uint64_t>::const_iterator sita = state->ids.begin(); sita != state->ids.end(); ++sita )
						LHT->V[id].ids.push_back(*sita);

					typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_reverse_iterator ite = state->TT.rend();

					for ( typename std::map < char_type, typename TrieState<char_type>::shared_ptr_type >::const_reverse_iterator ita = state->TT.rbegin();
						ita != ite; ++ita )
					{
						uint64_t const tid = F.find(ita->second.get())->second;
						LHT->addTransition(id,ita->first,tid);
						S.push(ita->second.get());
					}
				}

				LHT->fillDepthParent();

				return UNIQUE_PTR_MOVE(LHT);
			}
		};

		template<typename char_type>
		inline std::ostream & operator<<(std::ostream & out, Trie<char_type> const & T)
		{
			out << "Trie(" << (*(T.initial)) << ")";
			return out;
		}
	}
}
#endif
