#pragma once

#include "NullParameter.h"
#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/call_traits.hpp>
#include <map>

namespace scgl
{
	template < typename Event,
			 typename Parameter1 = SNullParameter,
			 typename Parameter2 = SNullParameter,
			 typename Parameter3 = SNullParameter >
	class CReactor : public boost::noncopyable
	{
	public:
		typedef	typename boost::call_traits<Parameter1>::param_type Parameter1Type;
		typedef	typename boost::call_traits<Parameter2>::param_type	Parameter2Type;
		typedef	typename boost::call_traits<Parameter3>::param_type Parameter3Type;
		typedef boost::function<void(Parameter1Type parameter1, Parameter2Type parameter2, Parameter3Type parameter3)> Action;

	public:
		CReactor()
			: m_state(0)
		{

		}
		virtual ~CReactor(void)
		{
		}

	public:
		bool Check(Event event)
		{
			if (GetReaction(event, GetState()) != NULL)
			{
				return true;
			}
			return false;
		}

		void Process(Event event, Parameter1Type parameter1 = SNullParameter(), Parameter2Type parameter2 = SNullParameter(), Parameter3Type parameter3 = SNullParameter());

		bool RegisterReaction(int currentState, Event event, int nextState, Action beforeStateChangeAction = NULL, Action afterStateChangeAction = NULL);

		int GetState()
		{
			return m_state;
		}

		void SetState(int state)
		{
			m_state = state;
		}

	private:
		struct	SReaction
		{
			Event		event;
			int			currentState;
			int			nextState;
			Action		beforeStateChangeAction;
			Action		afterStateChangeAction;
			SReaction(Event _event, int _currentState, int _nextState, Action _beforeStateChangeAction, Action _afterStateChangeAction)
				: event(_event)
				, currentState(_currentState)
				, nextState(_nextState)
				, beforeStateChangeAction(_beforeStateChangeAction)
				, afterStateChangeAction(_afterStateChangeAction)
			{

			}
		};
		struct EventIDStateID {};

		typedef boost::multi_index_container < SReaction,
				boost::multi_index::indexed_by <
				boost::multi_index::hashed_unique <
				boost::multi_index::tag<EventIDStateID>,
				boost::multi_index::composite_key <
				SReaction,
				boost::multi_index::member<typename SReaction, typename Event, &SReaction::event>,
				boost::multi_index::member<typename SReaction, int, &SReaction::currentState>
				>
				>
				>
				> EventIDStateReactorMap;


	private:

		const SReaction* GetReaction(Event event, int stateID)
		{
			EventIDStateReactorMap::iterator reaction = m_reactor.get<EventIDStateID>().find(boost::make_tuple(event, stateID));
			if (reaction != m_reactor.end())
			{
				return &(*reaction);
			}

			return NULL;
		}

	private:
		EventIDStateReactorMap		m_reactor;
		int							m_state;
	};

	template <typename Event, typename Parameter1, typename Parameter2, typename Parameter3>
	void CReactor<Event, Parameter1, Parameter2, Parameter3>::Process(Event event,
			Parameter1Type parameter1, Parameter2Type parameter2, Parameter3Type parameter3)
	{

		const SReaction* reactor = GetReaction(event, GetState());
		if (reactor != NULL)
		{
			if (reactor->beforeStateChangeAction != NULL)
			{
				reactor->beforeStateChangeAction(parameter1, parameter2, parameter3);
			}
			if (reactor->nextState != 0)
			{
				SetState(reactor->nextState);
			}
			if (reactor->afterStateChangeAction != NULL)
			{
				reactor->afterStateChangeAction(parameter1, parameter2, parameter3);
			}
		}
	}


	template <typename Event, typename Parameter1, typename Parameter2, typename Parameter3>
	bool CReactor<Event, Parameter1, Parameter2, Parameter3>::RegisterReaction(int currentState,
			Event event,
			int nextState,
			Action beforeStateChangeAction,
			Action afterStateChangeAction)
	{
		return m_reactor.insert(SReaction(event, currentState, nextState, beforeStateChangeAction, afterStateChangeAction)).second;
	}

}
