#pragma once

#include <QThread>
#include "TacviewServer.h"

class TacviewServerTester  : public QThread
{
	Q_OBJECT


private:
	TacviewServer* server = nullptr;

	TacviewServer::EntityData f35Blue = TacviewServer::EntityData((quint32)1001, "F35Blue", TacviewServer::EAircraftType::F35, TacviewServer::ECoalition::Blue, 0, 
#if USE_FLARE
		0,
#endif
		TacviewServer::Position(121.283111, 29.175990, 3000), TacviewServer::Attitude(90, 0, 0));

	TacviewServer::EntityData f22Red = TacviewServer::EntityData((quint32)1002, "F22Red", TacviewServer::EAircraftType::F22, TacviewServer::ECoalition::Red, 0,
#if USE_FLARE
		0, 
#endif
		TacviewServer::Position(121.284222, 29.175990, 3050), TacviewServer::Attitude(90, 0, 0));

	TacviewServer::EntityData aim120c = TacviewServer::EntityData(1003, "AIM_120C", TacviewServer::EAircraftType::AIM_120C, TacviewServer::ECoalition::Red);
public:
	TacviewServerTester(QObject *parent);
	~TacviewServerTester();

	void Start();

	void run() override;
};
