#include "chatui.h"
#include "helpers.h"
#include "message_mappings.h"
#include <spdlog/spdlog.h>



using namespace ftxui;
namespace ChatApp
{
	ChatUI::ChatUI(std::shared_ptr<spdlog::logger> _file_logger, std::shared_ptr<ChatLogic> _chatLogic)
	{
		file_logger = _file_logger;
		chatLogic = _chatLogic;

		
	}

	void ChatUI::Update()
	{
		auto clientUser = chatLogic->GetClient();

		//INPUT
		auto input_component = Input(&currentInput, "Type your message here");
		auto inputContent = Renderer([=] {
			return vbox(
				{
					input_component->Render()
				}) | size(WidthOrHeight::WIDTH, Constraint::EQUAL, 100) | borderLight;
			});

		//HEADER
		auto headerContent = Renderer([=] {
			Element channelName = text(clientUser.roomname() == "" ? "Default Room" : clientUser.roomname() + " Room") | ftxui::color(colorOfRoomText);
			return vbox(
				{
					channelName | size(WidthOrHeight::WIDTH, Constraint::EQUAL, 100),
					separator()
				});
			});

		//MESSAGES VIEW
		auto textContent = Renderer([=] {
			Elements message_elements;

			auto chatHistory = chatLogic->GetChatHistory();
			for (const auto& msg : chatHistory) {

				char timeString[std::size("hh:mm:ssZ")];
				std::strftime(std::data(timeString), std::size(timeString),
					"%T", std::gmtime(&msg.receiveTime));
				std::string time_str = "[" + std::string(timeString) + "]";
				auto styled_time = ftxui::text(time_str) | ftxui::color(timeChatMessageColor);
				std::string sender = msg.chatMessage.sender();
				auto styled_sender = ftxui::text(sender) | ftxui::color(GetEnumValue(msg.chatMessage.messagetype(), colorDependingOnChatMessage, colorChatMessageDefault));
				std::string message = msg.chatMessage.message();
				auto styled_message = ftxui::paragraph(message);


				message_elements.push_back(hbox(
					{
						styled_time,
						styled_sender | align_right | size(WidthOrHeight::WIDTH, Constraint::EQUAL, 30) ,
						separator(),
						styled_message,
					}));
			}
			return vbox(message_elements) | focusPositionRelative(0, chatScrollValue) | frame | flex;
			});


		//CHAT SLIDER
		SliderOption<float> chatSliderOption;
		chatSliderOption.value = &chatScrollValue;
		chatSliderOption.min = 0.f;
		chatSliderOption.max = 1.f;
		chatSliderOption.increment = 0.01f;
		chatSliderOption.direction = Direction::Down;
		chatSliderOption.color_active = Color::Yellow;
		chatSliderOption.color_inactive = Color::YellowLight;
		auto chatScrollBarY = Slider(chatSliderOption);

		//ALL USERS SLIDER
		SliderOption<float> allUserOption;
		allUserOption.value = &allUsersScrolllValue;
		allUserOption.min = 0.f;
		allUserOption.max = 1.f;
		allUserOption.increment = 0.01f;
		allUserOption.direction = Direction::Down;
		allUserOption.color_active = Color::Yellow;
		allUserOption.color_inactive = Color::YellowLight;
		auto allUserScrollBarY = Slider(allUserOption);

		//ALL USERS VIEW
		auto allUsersContent = Renderer([=] {
			Elements allUsersElements;
		
			auto allUsers = chatLogic->GetAllUsers();
			for (auto a : allUsers.users())
			{
				std::string userText = a.name() + "#" + std::to_string(a.id());
				allUsersElements.push_back(hbox(text(userText)));
			}
			return vbox(allUsersElements) | focusPositionRelative(0, allUsersScrolllValue) | frame | flex;
			});


		//ORGANIZE ALL OF THEM
		auto main_component = Container::Vertical({
			headerContent ,
			Container::Horizontal({
				Container::Horizontal({
					textContent,
					chatScrollBarY
				}) | borderLight | flex,
				Container::Horizontal({
					allUsersContent,
					allUserScrollBarY
				}) | borderLight | flex | size(WidthOrHeight::WIDTH, Constraint::EQUAL, 20)
			  }) | flex,
			input_component | size(WidthOrHeight::WIDTH, Constraint::EQUAL, 100) | borderLight,
			}) | borderLight | size(WidthOrHeight::HEIGHT, Constraint::EQUAL, 100);


		//RENDER IT
		auto main_renderer = Renderer(main_component, [&] {
			return vbox({
				main_component->Render()
				});
			});

		//LISTEN TO EVENTS
		auto component = CatchEvent(main_renderer, [&](Event event) {
			if (event == Event::Return) {
				HandleInput();
				return true;
			}
			return false;
			});

		//SETUP SCREEN
		auto screen = ScreenInteractive::Fullscreen();

		//REFRESHING EVERY X SECONDS
		std::thread autoRefreshScreen([&screen] {
			while (true)
			{
				using namespace std::chrono_literals;
				screen.PostEvent(Event::Custom);
				std::this_thread::sleep_for(0.2s);
			}
			});


		//RENDERING LOOP
		screen.Loop(component);
	}

	void ChatUI::HandleInput() {
		if (currentInput.empty())
			return;

		if (currentInput[0] == '/') // command
		{

			SPDLOG_LOGGER_INFO(file_logger, "Sending command: {}", currentInput);

			// commented it, because it was making chat less readable
			// 
			//auto msg = GetChatMessage(currentInput, GetEnumValue(MessageSendType::LOCAL, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
			//AddChatMessage(msg);

			std::istringstream iss(currentInput);
			std::vector<std::string> words;
			std::string word;
			// The extraction operator (>>) reads whitespace-separated words
			while (iss >> word) {
				words.push_back(word);
			}

			Envelope envelope{};

			envelope.set_type(MessageType::COMMAND);
			CommandRequest cr{};
			cr.set_request(words[0]);

			for (size_t i = 1; i < words.size(); ++i) {
				cr.add_requestparameters(words[i]);
			}

			envelope.set_payload(cr.SerializeAsString());
			chatLogic->SendToServer(envelope);
		}
		else
		{
			auto clientUser = chatLogic->GetClient();
			if (clientUser.id() != -1 && clientUser.connectedroomid() != -1)
			{

				SPDLOG_LOGGER_INFO(file_logger, "Send chat message: {}", currentInput);
				std::string name = std::format("{}#{}", clientUser.name(), clientUser.id());
				auto msg = chatLogic->GetChatMessage(currentInput, name, ChatMessageType::MESSAGE_IN_ROOM);

				Envelope envelope{};
				envelope.set_type(MessageType::CHAT_MESSAGE);
				envelope.set_sendtype(MessageSendType::WITHIN_ROOM_EXCEPT_THIS);
				envelope.set_payload(msg.SerializeAsString());

				if (chatLogic->SendToServer(envelope))
				{
					chatLogic->AddChatMessage(msg);
				}
			}
			else 
			{
				SPDLOG_LOGGER_WARN(file_logger, "Cannot send chat message: {}", currentInput);
				auto msg = chatLogic->GetChatMessage("You need to set nickname and join any room to send messages.", GetEnumValue(MessageSendType::LOCAL, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
				chatLogic->AddChatMessage(msg);
			}
		}


		currentInput.clear();
	}
}