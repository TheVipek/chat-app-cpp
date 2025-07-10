#pragma once

//ftxui
#include "ftxui/dom/elements.hpp"  
#include "ftxui/component/screen_interactive.hpp" 
#include "ftxui/dom/node.hpp"      
#include "ftxui/screen/color.hpp"  
#include "ftxui/component/component.hpp"
//^ it needs to be above everything because in chatlogic.h im including some windows headers which are making macros from ftxui not work properly
//protobuf
#include "communication.pb.h"
#include "chatlogic.h"
#include <memory>
#include <string>


class spdlog::logger;

namespace ChatApp
{
	/// <summary>
	/// Responsible for everything on UI side
	/// </summary>
	class ChatUI
	{
	public:
		ChatUI(std::shared_ptr<spdlog::logger> _file_logger, std::shared_ptr<ChatLogic> _chatLogic);
		~ChatUI() = default;
		void Update();
	private:
		void HandleInput();
		
		std::shared_ptr<spdlog::logger> file_logger;
		std::shared_ptr<ChatLogic> chatLogic;
		std::string currentInput;
		/// <summary>
		/// default value is 1.f because it will automatically adjust view when messages are going out of boundaries
		/// </summary>
		float chatScrollValue = 1.f;
		/// <summary>
		/// default value is 1.f because it will automatically adjust view when messages are going out of boundaries
		/// </summary>
		float allUsersScrolllValue = 1.f;
	};
}
