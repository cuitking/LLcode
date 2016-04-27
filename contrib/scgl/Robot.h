#ifndef SCGL_ROBOT_H
#define SCGL_ROBOT_H

namespace scgl
{

	class CBrain;
	class IBehavior;
	class IRobot
	{
	public:
		virtual ~IRobot() = 0;
		virtual void Initialize(long long nNow) = 0;
	public:
		void Update(long long nNow);
		void Terminate();
		bool IsTerminated() const;
		long long GetUpdateTime() const;
		void ReactiveCurrentBehavior(long long nNow);

	protected:
		IRobot();
	protected:
		void AddBehavior(IBehavior* pBehavior);
		CBrain* GetBrain() const
		{
			return m_pBrain;
		}

	private:
		IRobot(const IRobot&);
		const IRobot& operator=(const IRobot&);
	private:
		virtual void DoUpdate(long long nNow) = 0;
	private:
		CBrain*     m_pBrain;
		bool        m_bTerminated;
		long long   m_nUpdateTime;
	};

}

#endif
