#pragma once
#include "../../generated/communication.pb.h"
#include <set>
class RoomContainer
{
public:
	RoomContainer(Room* room);
	Room* room;
	std::set<ClientUser*> usersInRoom;
	void AddUser(ClientUser* user);
	void RemoveUser(ClientUser* user);
};
