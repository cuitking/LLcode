#ifndef SCGL_TRIGGER_H
#define SCGL_TRIGGER_H

#include "NullParameter.h"
#include <boost/function.hpp>
#include <boost/call_traits.hpp>
#include <boost/noncopyable.hpp>
#include <map>

namespace scgl
{
	template <typename Event, 	typename Parameter1 = SNullParameter, 	typename Parameter2 = SNullParameter, 	typename Parameter3 = SNullParameter>
	class CTrigger : public boost::noncopyable
	{
	public:
		typedef	typename boost::call_traits<Parameter1>::param_type Parameter1Type;
		typedef	typename boost::call_traits<Parameter2>::param_type	Parameter2Type;
		typedef	typename boost::call_traits<Parameter3>::param_type Parameter3Type;
		typedef boost::function<void(Parameter1Type parameter1, Parameter2Type parameter2, Parameter3Type parameter3)> Action;

	public:
		CTrigger()
		{

		}
		~CTrigger(void)
		{
		}

	public:
		void Process(int groupID, Event event, Parameter1Type parameter1 = TriggerNullParameter(), Parameter2Type parameter2 = TriggerNullParameter(), Parameter3Type parameter3 = TriggerNullParameter());
		bool Register(int groupID, Event event, Action action);
		bool CheckRegister(int groupID, Event event);

	private:
		typedef std::vector<Action>						ActionVector;
		typedef std::map<Event, ActionVector>			EventActionVectorMap;
		typedef std::map<int,	EventActionVectorMap>	IntEventActionMap;

	private:
		IntEventActionMap	m_reactor;
	};

	template <typename Event, typename Parameter1, typename Parameter2, typename Parameter3>
	void CTrigger<Event, Parameter1, Parameter2, Parameter3>::Process(int groupID, Event event,
			Parameter1Type parameter1, Parameter2Type parameter2, Parameter3Type parameter3)
	{
		IntEventActionMap::const_iterator itGroup = m_reactor.find(groupID);
		if (itGroup != m_reactor.end())
		{
			EventActionVectorMap::const_iterator it = itGroup->second.find(event);
			if (it != itGroup->second.end())
			{
				for (ActionVector::const_iterator itAction = it->second.begin(); itAction != it->second.end(); ++itAction)
				{
					(*itAction)(parameter1, parameter2, parameter3);
				}
			}
		}
	}


	template <typename Event, typename Parameter1, typename Parameter2, typename Parameter3>
	bool CTrigger<Event, Parameter1, Parameter2, Parameter3>::Register(int groupID, Event event, Action action)
	{
		m_reactor[groupID][event].push_back(action);
		return true;
	}

	template <typename Event, 	typename Parameter1, 	typename Parameter2, 	typename Parameter3>
	bool CTrigger<Event, Parameter1, Parameter2, Parameter3>::CheckRegister( int groupID, Event event )
	{
		EventActionMap::const_iterator it = m_reactor[groupID].find(event);
		if (it != m_reactor[groupID].end())
		{
			return true;
		}
		return false;
	}
}

#endif
