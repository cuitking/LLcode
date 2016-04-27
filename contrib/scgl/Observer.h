#pragma once

#define Notify_GameServer 1
#define Notify_Room  2
#define Notify_All   3

namespace scgl
{
	class CPlayerDetail;

	class IObserver
	{
	public:
		virtual void Update(scgl::CPlayerDetail* sender, scgl::Guid oldGuid = 0) = 0;
	};
}