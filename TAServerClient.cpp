#include "TAServerClient.h"

TAServer::Client g_TAServerClient;

namespace TAServer {

	long long netIdToLong(FUniqueNetId id) {
		long long tmpB = id.Uid.B;
		return (tmpB << 32) | (id.Uid.A);
	}

	FUniqueNetId longToNetId(long long id) {
		FUniqueNetId r;
		r.Uid.A = id & 0x00000000FFFFFFFF;
		r.Uid.B = id >> 32;
		return r;
	}

	bool Client::connect(std::string host, int port) {
		tcpClient = std::make_shared<TCP::Client<short> >(ios);

		// Run handlers for reading etc. in a new thread
		iosThread = std::make_shared<std::thread>([&] {
			boost::asio::io_service::work work(ios);
			attachHandlers();
			tcpClient->start(host, port, std::bind(&Client::handler_OnConnect, this));

			ios.run();
		});

		// Start the thread used to poll game information to send
		gameInfoPollingThread = std::make_shared<std::thread>([&] {
			while (true) {
				pollForGameInfoChanges();
				std::this_thread::sleep_for(std::chrono::seconds(2));
				
			}
		});
		gameInfoPollingThread->detach();

		return true;
	}

	bool Client::disconnect() {
		tcpClient->stop();
		iosThread->join();
		return false;
	}

	bool Client::isConnected() {
		return !tcpClient->is_stopped();
	}

	void Client::attachHandlers() {
		tcpClient->add_handler(TASRV_MSG_KIND_LAUNCHER_2_GAME_LOADOUT_MESSAGE, [this](const json& j) {
			handler_Launcher2GameLoadoutMessage(j);
		});
		tcpClient->add_handler(TASRV_MSG_KIND_LAUNCHER_2_GAME_NEXT_MAP_MESSAGE, [this](const json& j) {
			handler_Launcher2GameNextMapMessage(j);
		});
		tcpClient->add_handler(TASRV_MSG_KIND_LAUNCHER_2_GAME_PINGS_MESSAGE, [this](const json& j) {
			handler_Launcher2GamePingsMessage(j);
		});
	}

	void Client::sendProtocolVersion() {
		Game2LauncherProtocolVersionMessage msg;
		json j;
		msg.toJson(j);
		Logger::debug("SendProtocolVersion json: %s", j.dump().c_str());
		tcpClient->send(msg.getMessageKind(), j);
	}

	bool Client::retrieveLoadout(FUniqueNetId uniquePlayerId, int classId, int slot, std::map<int, int>& resultEquipMap) {
		Game2LauncherLoadoutRequest req;
		req.uniquePlayerId = uniquePlayerId;
		req.classId = classId;
		req.slot = slot;
		
		json j;
		req.toJson(j);

		tcpClient->send(req.getMessageKind(), j);

		// Block until a loadout is received or a timeout of 5 seconds occurs
		auto startTime = std::chrono::system_clock::now();
		auto& it = receivedLoadouts.find(netIdToLong(uniquePlayerId));
		while (it == receivedLoadouts.end()) {
			if (std::chrono::system_clock::now() > startTime + std::chrono::seconds(5)) return false;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			it = receivedLoadouts.find(netIdToLong(uniquePlayerId));
		}

		// Delete the entry
		{
			std::lock_guard<std::mutex> lock(receivedLoadoutsMutex);
			receivedLoadouts.erase(netIdToLong(uniquePlayerId));
		}

		Launcher2GameLoadoutMessage response = it->second;
		response.toEquipPointMap(resultEquipMap);

		return true;
	}

	void Client::sendTeamInfo(const std::map<long long, int>& playerToTeamId) {
		Game2LauncherTeamInfoMessage msg;
		for (auto& elem : playerToTeamId) {
			msg.playerToTeamId[elem.first] = elem.second;
		}

		json j;
		msg.toJson(j);
		Logger::debug("SendTeamInfo json: %s", j.dump().c_str());
		tcpClient->send(msg.getMessageKind(), j);
	}

	void Client::sendScoreInfo(int beScore, int dsScore) {
		Game2LauncherScoreInfoMessage msg;
		msg.beScore = beScore;
		msg.dsScore = dsScore;

		json j;
		msg.toJson(j);
		Logger::debug("SendScoreInfo json: %s", j.dump().c_str());
		tcpClient->send(msg.getMessageKind(), j);
	}

	void Client::sendMatchTime(long long matchSecondsLeft, bool counting) {
		Game2LauncherMatchTimeMessage msg;
		msg.secondsRemaining = matchSecondsLeft;
		msg.counting = counting;

		json j;
		msg.toJson(j);
		Logger::debug("SendMatchTime json: %s", j.dump().c_str());
		tcpClient->send(msg.getMessageKind(), j);
	}

	void Client::sendMatchEnded() {
		Game2LauncherMatchEndMessage msg;

		json j = json::object();
		msg.toJson(j);
		Logger::debug("SendMatchEnd json: %s", j.dump().c_str());
		tcpClient->send(msg.getMessageKind(), j);
	}
}
