#include "RoomContainer.h"

RoomContainer::RoomContainer(Room* _room, bool _destroyOnEmpty)
{
	room = _room;
	destroyOnEmpty = _destroyOnEmpty;
	usersInRoom = std::set<ClientUser*>();
}
RoomContainer::~RoomContainer()
{
	delete room;
	usersInRoom.clear();
}
void RoomContainer::AddUser(ClientUser* user)
{
	if (!usersInRoom.contains(user))
	{
		usersInRoom.insert(user);
	}
}

void RoomContainer::RemoveUser(ClientUser* user) 
{
	if (usersInRoom.contains(user))
	{
		usersInRoom.erase(user);
	}
}