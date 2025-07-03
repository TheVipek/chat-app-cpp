#include "RoomContainer.h"

RoomContainer::RoomContainer(Room* _room)
{
	room = _room;
	usersInRoom = std::set<ClientUser*>();
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