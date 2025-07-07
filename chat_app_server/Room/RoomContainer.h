#pragma once
#include "../../generated/communication.pb.h"
#include <set>
class RoomContainer
{
public:
	RoomContainer(Room* room, bool destroyOnEmpty);
	~RoomContainer();
	Room* room;
	std::set<ClientUser*> usersInRoom;
	bool destroyOnEmpty;
	void AddUser(ClientUser* user);
	void RemoveUser(ClientUser* user);
};
