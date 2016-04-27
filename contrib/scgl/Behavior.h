#ifndef SCGL_BEHAVIOR_H
#define SCGL_BEHAVIOR_H

#include "NullParameter.h"
#include "SingletonHolder.h"
#include <loki/CachedFactory.h>
#include <boost/call_traits.hpp>
#include <set>

namespace scgl
{
	template <
	template <typename, typename, typename, typename> class Reactor,
			 template <typename, typename, typename, typename> class Trigger,
			 typename Event,
			 typename Parameter1 = SNullParameter,
			 typename Parameter2 = SNullParameter,
			 typename Parameter3 = SNullParameter >
	class IBehavior :
		public Reactor<Event, Parameter1, Parameter2, Parameter3>,
		public Trigger<Event, Parameter1, Parameter2, Parameter3>
	{
	public:
		typedef scgl::SingletonHolder < Loki::Factory <
		IBehavior<Reactor, Trigger, Event, Parameter1, Parameter2, Parameter3>,
				  std::string > >	Factory;
		typedef	typename boost::call_traits<Parameter1>::param_type Parameter1Type;
		typedef	typename boost::call_traits<Parameter2>::param_type Parameter2Type;
		typedef	typename boost::call_traits<Parameter3>::param_type	Parameter3Type;


	public:
		virtual ~IBehavior()
		{
			delete m_impl;
		}

		bool CheckTransaction(Event event)
		{
			std::set<Event>::iterator it = transactionSet.find(event);
			if (it == transactionSet.end())
			{
				return false;
			}
			return true;
		}

		void AddChild(IBehavior* child)
		{
			if (child != NULL)
			{
				child->SetParent(this);
				m_impl->children.push_back(child);
			}
		}

		bool Check(Event event)
		{
			if (CReactor::Check(event))
			{
				return true;
			}
			if (m_impl->currentBehavior != NULL)
			{
				return m_impl->currentBehavior->Check(event);
			}
			return false;
		}

		const IBehavior* GetCurrentBehavior() const
		{
			if (m_impl->currentBehavior != NULL)
			{
				return m_impl->currentBehavior->GetCurrentBehavior();
			}
			return this;
		}

		IBehavior* GetParent() const
		{
			return m_impl->parent;
		}

		void SetParent(IBehavior* parent)
		{
			m_impl->parent = parent;
		}

		void RemoveChild(IBehavior* child)
		{
			if (child == NULL)
			{
				return;
			}

			for (BehaviorPointerVector::iterator it = m_impl->children.begin(); it != m_impl->children.end(); ++it)
			{
				if ((*it) == child)
				{
					m_impl->children.erase(it);
					return;
				}
			}
		}

		void Process(int group , Event event,
					 Parameter1Type parameter1 = SNullParameter(),
					 Parameter2Type parameter2 = SNullParameter(),
					 Parameter3Type parameter3 = SNullParameter())
		{
			// 所有的子节点要响应一遍
			for (BehaviorPointerVector::iterator it = m_impl->children.begin(); it != m_impl->children.end(); ++it)
			{
				(*it)->Process(group, event, parameter1, parameter2, parameter3);
			}

			// 当前行为要响应一遍
			if (m_impl->currentBehavior != NULL)
			{
				m_impl->currentBehavior->Process(group, event, parameter1, parameter2, parameter3);
			}

			// 自己响应一遍
			CTrigger::Process(group, event, parameter1, parameter2, parameter3);
			CReactor::Process(event, parameter1, parameter2, parameter3);

			// 判断是否要切换行为
			if (ChangeBehavior(event))
			{
				//当前行为再响应一次
				m_impl->currentBehavior->Process(group, event, parameter1, parameter2, parameter3);
			}
		}

	protected:
		explicit IBehavior()
			: m_impl(new Impl)
		{
		}

	protected:
		template <typename T>
		bool InitializeBehavior()
		{
			if (m_impl->currentBehavior == NULL)
			{
				IBehavior* initBehavior = Factory::Instance().CreateObject(typeid(T).name());
				if (initBehavior != NULL)
				{
					m_impl->currentBehavior = initBehavior;
					return true;
				}
			}

			return false;
		}

		template <typename CurrentType, typename NextType>
		void RegisterBehaviorTransaction(Event event)
		{
			m_impl->reactions.insert(BehaviorMap::value_type(std::make_pair(event, typeid(CurrentType).name()), typeid(NextType).name()));
			transactionSet.insert(event);
		}

	private:

		typedef std::vector<IBehavior*>								BehaviorPointerVector;
		typedef	std::map<std::pair<Event, std::string>, std::string>	BehaviorMap;
		struct Impl
		{
			BehaviorMap							reactions;
			IBehavior*							currentBehavior;
			BehaviorPointerVector				children;
			IBehavior*							parent;
			Impl()
				: currentBehavior(NULL)
				, parent(NULL)
			{

			}
			~Impl()
			{
				if (currentBehavior != NULL)
				{
					delete currentBehavior;
					currentBehavior = NULL;
				}
			}
		};

	private:
		const char* FindTransition(Event event, const char* typeName)
		{
			BehaviorMap::iterator it = m_impl->reactions.find(std::make_pair(event, typeName));
			if (it != m_impl->reactions.end())
			{
				return it->second.c_str();
			}
			return NULL;
		}

		bool ChangeBehavior(Event event)
		{
			if (m_impl->currentBehavior != NULL)
			{
				const char* newStateName = FindTransition(event, typeid(*m_impl->currentBehavior).name());
				if (newStateName != NULL)
				{
					delete m_impl->currentBehavior;
					m_impl->currentBehavior = Factory::Instance().CreateObject(newStateName);
					return true;
				}
			}
			return false;
		}

	private:
		Impl*	m_impl;
		static std::set<Event> transactionSet;
	};

	template <typename T>
	T* Creator()
	{
		return new T;
	}

	template <typename T>
	bool RegisterToFactory()
	{
		return T::Factory::Instance().Register(typeid(T).name(), &scgl::Creator<T>);
	}
}

#endif
