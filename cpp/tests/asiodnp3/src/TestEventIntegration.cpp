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

#include <catch.hpp>

#include <asiodnp3/DNP3Manager.h>
#include <asiodnp3/ConsoleLogger.h>

#include <memory>
#include <iostream>
#include <thread>

#include "mocks/StackPair.h"

using namespace opendnp3;
using namespace asiodnp3;

#define SUITE(name) "EventIntegrationTestSuite - " name

TEST_CASE(SUITE("TestEventIntegration"))
{
	const uint16_t START_PORT = 20000;
	const uint16_t NUM_STACK_PAIRS = 1;

	const uint16_t NUM_POINTS_PER_TYPE = 50;
	const uint16_t EVENTS_PER_ITERATION = 50;
	const int NUM_ITERATIONS = 1;

	const uint32_t LEVELS = levels::ALL;// flags::ERR | flags::WARN;

	const auto TEST_TIMEOUT = std::chrono::seconds(5);
	const auto STACK_TIMEOUT = openpal::TimeDuration::Seconds(1);

	// run with at least a concurrency of 2, but more if there are more cores
	const auto concurreny = std::max<unsigned int>(std::thread::hardware_concurrency(), 2);

	DNP3Manager manager(concurreny, ConsoleLogger::Create());

	std::vector<std::unique_ptr<StackPair>> pairs;

	for (uint16_t i = 0; i < NUM_STACK_PAIRS; ++i)
	{
		auto pair = std::make_unique<StackPair>(LEVELS, STACK_TIMEOUT, manager, START_PORT + i, NUM_POINTS_PER_TYPE, EVENTS_PER_ITERATION);
		pairs.push_back(std::move(pair));
	}

	for (auto& pair : pairs)
	{
		pair->WaitForChannelsOnline(TEST_TIMEOUT);
	}

	const auto start = std::chrono::steady_clock::now();

	for (int i = 0; i < NUM_ITERATIONS; ++i)
	{

		for (auto& pair : pairs)
		{
			pair->SendRandomValues();
		}

		for (auto& pair : pairs)
		{
			pair->WaitToRxValues(TEST_TIMEOUT);
		}
	}

	const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

	const auto total_events_transferred = static_cast<uint64_t>(NUM_STACK_PAIRS) * static_cast<uint64_t>(EVENTS_PER_ITERATION) * static_cast<uint64_t>(NUM_ITERATIONS);

	const auto rate = (total_events_transferred / milliseconds.count()) * 1000;

	//std::cout << total_events_transferred << " in " << milliseconds.count() << " ms == " << rate << " events per/sec" << std::endl;
}

