/*
 * Licensed to Green Energy Corp (www.greenenergycorp.com) under one or
 * more contributor license agreements. See the NOTICE file distributed
 * with this work for additional information regarding copyright ownership.
 * Green Energy Corp licenses this file to you under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This project was forked on 01/01/2013 by Automatak, LLC and modifications
 * may have been made to this file. Automatak, LLC licenses these modifications
 * to you under the terms of the License.
 */
#include <asiodnp3/DNP3Manager.h>
#include <asiodnp3/ConsoleLogger.h>

#include <opendnp3/LogLevels.h>
#include <iostream>
#include <thread>

#include "ExampleListenCallbacks.h"

#include <map>
#include <functional>
#include <sstream>

using namespace std;
using namespace openpal;
using namespace asiopal;
using namespace asiodnp3;
using namespace opendnp3;

using command_handler_t = std::function<void (ExampleListenCallbacks&, const vector<string>& commands)>;
using command_handler_map_t = std::map<std::string, command_handler_t>;

vector<string> tokenize(const std::string& line);
void init_handlers(command_handler_map_t& map);
uint16_t parse_uint16(const std::string& value);

int main(int argc, char* argv[])
{
	command_handler_map_t handler_map;

	init_handlers(handler_map);

	// silence the chatty INFO logging as it doesn't work so well in a command line app
	const uint32_t FILTERS = flags::WARN | flags::ERR | flags::EVENT;

	const auto NUM_THREAD = std::thread::hardware_concurrency();

	const auto callbacks = std::make_shared<ExampleListenCallbacks>();

	// This is the main point of interaction with the stack
	DNP3Manager manager(NUM_THREAD, ConsoleLogger::Create());

	std::error_code ec;
	const auto server1 = manager.CreateListener("server-20000", FILTERS, IPEndpoint::AllAdapters(20000), callbacks, ec);

	if (ec)
	{
		std::cout << ec.message() << std::endl;
		return ec.value();
	}

	std::cout << "Type 'help' for a list of commands" << std::endl;
	do
	{
		std::cout << ">";

		std::string line;
		std::getline(std::cin, line);
		const auto tokens = tokenize(line);
		if(tokens.empty()) continue;

		if(tokens[0] == "exit") {
			return 0;
		}
		else {
			const auto handler = handler_map.find(tokens[0]);
			if(handler == handler_map.end())
			{
				std::cerr << "Unknown command: " << tokens[0] << std::endl;
			}
			else
			{
				try {
					handler->second(*callbacks, tokens);
				}
				catch(const std::exception& ex) {
					std::cerr << ex.what() << std::endl;
				}
			}
		}
	}
	while(true);

	return 0;
}

vector<string> tokenize(const std::string& line)
{
	stringstream ss(line);
	string s;
	vector<string> tokens;

	while(getline(ss, s, ' ')) {
		tokens.push_back(s);
	}

	return tokens;
}

void init_handlers(command_handler_map_t& map)
{
	map["help"] = [](ExampleListenCallbacks&, const vector<string>&) {
		std::cout << "help" << " - prints this message" << std::endl;
		std::cout << "exit" << " - exits the programhel" << std::endl;
		std::cout << "latch_on <address> <index>" << " - sends a CROB w/ LATCH_ON to the specified outstation address" << std::endl;
		std::cout << "integrity <address>" << " - perform an integrity (class 3/2/1/0) on the specified outstation address" << std::endl;
	};

	map["latch_on"] = [](ExampleListenCallbacks& callbacks, const vector<string>& commands) {
		if(commands.size() != 3) {
			throw std::runtime_error("latch_on expects exactly 2 arguments");
		}

		const auto address = parse_uint16(commands[1]);
		const auto index = parse_uint16(commands[2]);

		const auto session = callbacks.get_outstation_session(address);
		if(session) {
			session->DirectOperate(
					ControlRelayOutputBlock(ControlCode::LATCH_ON),
					index,
					[](const ICommandTaskResult&) {} // normally you would handle the result in some fashion
			);
		}
		else {
			throw std::runtime_error("Unknown outstation address");
		}
	};

	map["integrity"] = [](ExampleListenCallbacks& callbacks, const vector<string>& commands) {
		if(commands.size() != 2) {
			throw std::runtime_error("integrity expects exactly 1 argument");
		}

		const auto address = parse_uint16(commands[1]);

		const auto session = callbacks.get_outstation_session(address);
		if(session) {
			session->ScanClasses(
					ClassField::AllClasses()
			);
		}
		else {
			throw std::runtime_error("Unknown outstation address");
		}
	};
}

uint16_t parse_uint16(const std::string& value)
{
	return static_cast<uint16_t>(std::stoi(value));
}