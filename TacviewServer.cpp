#include "TacviewServer.h"
#include <QDebug>
#include <QRandomGenerator>


TacviewServer::TacviewServer(QObject *parent)
	: QObject(parent)
{

}

TacviewServer::~TacviewServer()
{
	if (client != nullptr)
	{
		client->close();
		client = nullptr;
	}
	if (server != nullptr)
	{
		server->close();
		server = nullptr;
	}
	frameID = 0;
	bHasConnected = false;
	entities.clear();
	pendingUpdateEntities.clear();
	pendingDeletedEntities.clear();
	airRoutes.clear();
	pendingUpdateAirRoutes.clear();
	pendingDeleteAirRoutes.clear();
	sensors.clear();
	pendingUpdateSensor.clear();
	pendingDeleteSensor.clear();
	pendingUpdateEntitiesWithSensor.clear();
	pendingDeleteGRD.clear();
}

bool TacviewServer::StartServer(int tcpServerPort, int _simStep)
{
	bool result = false;
	if (server == nullptr)
	{
		server = new QTcpServer(this);
		result = server->listen(QHostAddress::AnyIPv4, tcpServerPort);
		if (!result)
		{
			qDebug() << "Start TcpServer Listen Failed!\n ErrorMSG:" << server->errorString();
		}
		else
		{
			connect(server, &QTcpServer::newConnection, this, &TacviewServer::OnNewConnection);
			simStep = _simStep;
			qDebug() << "Create a new TcpServer and Listen Success!Listening Port:" << QString::number(server->serverPort());
			
			if (timer == nullptr)
			{
				timer = new QTimer(this);
			}
			connect(timer, &QTimer::timeout, this, &TacviewServer::OnTimeOut);
			timer->start(simStep);
		}

		return result;
	}
	else if (!server->isListening())
	{
		result = server->listen(QHostAddress::AnyIPv4, tcpServerPort);
		if (!result)
		{
			qDebug() << "Start TcpServer Listen Failed! \n ErrorMSG:" << server->errorString();
		}
		else
		{
			qDebug() << "Start TcpServer Listen Success!" << "Listening Port : " << QString::number(server->serverPort());
			simStep = _simStep;
			
			if (timer == nullptr)
			{
				timer = new QTimer(this);
			}
			connect(timer, &QTimer::timeout, this, &TacviewServer::OnTimeOut);
			timer->start(simStep);
		}

		return result;
	}
	else
	{
		if (tcpServerPort == server->serverPort())
		{
			qDebug() << "TcpServer is Listening!" << "Listening Port : " << QString::number(server->serverPort());
			return true;
		}
		else
		{
			server->close();
			server = new QTcpServer(this);
			result = server->listen(QHostAddress::AnyIPv4, tcpServerPort);
			if (!result)
			{
				qDebug() << "Restart and Create A TcpServer Listen Failed!\n ErrorMSG:" << server->errorString();
			}
			else
			{
				qDebug() << "Create a new TcpServer and Listen Success!" << " Listening Port : " << QString::number(server->serverPort());
				simStep = _simStep;
			
				if (timer == nullptr)
				{
					timer = new QTimer(this);
				}
				connect(timer, &QTimer::timeout, this, &TacviewServer::OnTimeOut);
				timer->start(simStep);
			}
		}
	}
}

#if FD_VERSION
TacviewServer::EntityData TacviewServer::get_entity_data(const SimulationInfo& simInfo)
{
	if (simInfo.nPlaneID == 0) {
		qDebug() << "ID ERROR!";
	}
	return EntityData((quint32)simInfo.nPlaneID, QString("Unknow"), F22, Blue, 0, 
		Position(simInfo.sFighterPara.dLongtitude_rad RAD2DEG, 
				 simInfo.sFighterPara.dLatitude_rad RAD2DEG, 
				 simInfo.sFighterPara.fAltitude_m),
		Attitude(simInfo.sFighterPara.fHeading_rad RAD2DEG, 
				 simInfo.sFighterPara.fPitch_rad RAD2DEG, 
				 simInfo.sFighterPara.fRoll_rad RAD2DEG), 
		FlyControl(0, 0, simInfo.sFighterPara.fNormalAcc_g));
}
#endif

void TacviewServer::UpdateEntity(const EntityData entitieData)
{
	pendingUpdateEntities.enqueue(entitieData);
}

void TacviewServer::UpdateEntityWithSensor(const EntityData entitieData, const Sensor buildingData)
{
	EntityData temp = entitieData;
	QByteArray toSend;
	toSend.append(temp.toStringUpdate());
	//删除末尾的换行符
	toSend.remove(toSend.length() - 1, 1);
	//拼接传感器字符
	toSend.append(QString("Visible=1") + QString(","));
	toSend.append(QString("RadarMode=1,"));
	toSend.append(QString("RadarAzimuth=") + QString::number(buildingData.att.heading) + QString(","));
	toSend.append(QString("RadarElevation=") + QString::number(buildingData.att.pitch) + QString(","));
	toSend.append(QString("RadarRoll=") + QString::number(buildingData.att.roll) + QString(","));
	toSend.append(QString("RadarHorizontalBeamwidth=") + QString::number(buildingData.aov.horizontal) + QString(","));
	toSend.append(QString("RadarVerticalBeamwidth=") + QString::number(buildingData.aov.vertical) + QString(","));
	toSend.append(QString("RadarRange=") + QString::number(buildingData.aov.radius));
	toSend.append("\n");
	pendingUpdateEntitiesWithSensor.append(toSend);
}

void TacviewServer::DeleteEntity(const quint32 entitieID)
{
	pendingDeletedEntities.enqueue(entitieID);
}

void TacviewServer::UpdateAirRoute(const AirRoute airRouteData)
{
	pendingUpdateAirRoutes.enqueue(airRouteData);
}

void TacviewServer::DeleteAirRoute(const quint32 airRouteID)
{
	pendingDeleteAirRoutes.enqueue(airRouteID);
}

void TacviewServer::UpdateSensor(const Sensor buildingData)
{
	pendingUpdateSensor.enqueue(buildingData);
}

void TacviewServer::DeleteSensor(const quint32 buildingID)
{
	pendingDeleteSensor.enqueue(buildingID);
}

QByteArray TacviewServer::Entities2TacviewFormate()
{
	if (entities.count() > 3)
	{
		qDebug() << "Error!";
	}
	//自动锁，在离开作用域时自动解锁
	QMutexLocker locker(&entitiesMutex);
	QByteArray toReturn;
	if (!pendingUpdateEntities.isEmpty())
	{
		QMap<int ,EntityData> addEntities;
		//将数据缓冲池中的数据分类，更新数据直接更新，新增数据先收集起来延迟后续处理
		while (!pendingUpdateEntities.isEmpty())
		{
			EntityData temp = pendingUpdateEntities.dequeue();
			auto it = entities.find(temp.ID);
			if (it != entities.end())
			{
				it.value() = temp;
			}
			else
			{
				addEntities.insert(temp.ID, temp);
			}
			
		}
		//先转换更新后的实体数据
		/*for each (EntityData var in entities)
		{
			if (var.ID != 0)
			{
				toReturn.append(var.toStringUpdate());
			}
			
		}*/
		//上述代码在Release模式下出错替换为下述遍历形式
		for (QMap<int, EntityData>::iterator iter = entities.begin(); iter != entities.end(); iter++)
		{
			if (iter.key() != 0)
			{
				toReturn.append(iter.value().toStringUpdate());
			}
		}
		
		if (addEntities.count() > 3)
		{
			qDebug() << "ERROR!";
		}

		//然后转换新增的实体数据,同时吧新增实体添加到entities中
		for (QMap<int, EntityData>::iterator iter = addEntities.begin(); iter != addEntities.end(); iter++)
		{
			if (iter.key() != 0)
			{
				toReturn.append(iter.value().toStringCreate());
				entities.insert(iter.value().ID, iter.value());
			}
		}
	}
	else//数据池无需要处理的数据
	{
		/*for each (EntityData var in entities)
		{
			toReturn.append(var.toStringUpdate());
		}*/
		for (QMap<int, EntityData>::iterator iter = entities.begin(); iter != entities.end(); iter++)
		{
			toReturn.append(iter.value().toStringUpdate());
		}
	}
	return toReturn;
}

QByteArray TacviewServer::AirRoute2TacviewFormate()
{
	QByteArray toReturn;
	while (pendingUpdateAirRoutes.size() >0 )
	{
		AirRoute temp = pendingUpdateAirRoutes.dequeue();
		if (airRoutes.contains(temp.ID))
		{
#if REDRAW
			//先删除旧的航路点
			for (size_t i = 0; i < airRoutes[temp.ID].waypoints.count(); i++)
			{
				toReturn.append("-");
				toReturn.append(QString::number(airRoutes[temp.ID].waypoints[i].ID));
				toReturn.append("\n");
			}
#endif
			//更新替换航路数据
			airRoutes[temp.ID] = temp;
			//生成新的航路点
			toReturn.append(airRoutes[temp.ID].toString());
		}
		else//添加新的航路
		{
			toReturn.append(temp.toString());
			airRoutes.insert(temp.ID, temp);
		}
	}
	return toReturn;
}

QByteArray TacviewServer::ReleaseChaff()
{
	QByteArray toReturn;
	if (entities.count() >= 1)
	{
		for (QMap<int, EntityData>::iterator iter = entities.begin(); iter != entities.end(); iter++)
		{
			if (iter.value().ID != 0)
			{
				if (iter.value().chaff >= 1)
				{
					quint32 toDeleteFrameID = GRD_TIME * 1000.0 / simStep + frameID;
					QList<quint32> toDeleteGRDs;
					for (size_t j = iter.value().chaff; j > 0; j--)
					{
						//实测需要先删除已经存在的flare对象
						toReturn.append(QString("-") + QString::number(iter.value().ID + 111111 + j));
						toReturn.append("\n");
						toReturn.append(QString::number(iter.value().ID + 111111 + j) + QString(",Type=Misc+Decoy+Chaff,T="));
						toReturn.append(QString::number(iter.value().pos.longitude + QRandomGenerator::global()->generateDouble() / INTERVAL, 'f', 9) + QString("|"));
						toReturn.append(QString::number(iter.value().pos.latitude + QRandomGenerator::global()->generateDouble() / INTERVAL, 'f', 9) + QString("|"));
						toReturn.append(QString::number(iter.value().pos.altidude + QRandomGenerator::global()->generateDouble() / INTERVAL, 'f', 9) + QString(","));
						toReturn.append(QString("Color=") + Enum2String(iter.value().coalition) + QString(","));
						toReturn.append(QString("Parent=") + QString::number(iter.value().ID));
						toReturn.append("\n");
						toDeleteGRDs.append(iter.value().ID + 111111 + j);
					}
					pendingDeleteGRD.append(QPair(toDeleteFrameID, toDeleteGRDs));
				}
			}
		}
	}
	return toReturn;
}

QByteArray TacviewServer::Sensor2TacviewFormate()
{
	QByteArray toReturn;
	while (pendingUpdateSensor.size() > 0)
	{
		Sensor temp = pendingUpdateSensor.dequeue();
		if (sensors.contains(temp.ID))
		{
#if REDRAW
			//先删除已经存在的build
			toReturn.append(QString("-") + QString::number(temp.ID));
			toReturn.append("\n");
#endif
			//添加更新的
			toReturn.append(temp.toString());
			sensors[temp.ID] = temp;
		}
		else
		{
			toReturn.append(temp.toString());
			sensors.insert(temp.ID, temp);
		}
	}
	return toReturn;
}

#if USE_FLARE
QByteArray TacviewServer::ReleaseFlare()
{
	QByteArray toReturn;
	if (entities.count() >= 1)
	{
		for (QMap<int, EntityData>::iterator iter = entities.begin(); iter != entities.end(); iter++)
		{
			if (iter.value().ID != 0)
			{
				if (iter.value().flare >= 1)
				{
					quint32 toDeleteFrameID = GRD_TIME * 1000.0 / simStep + frameID;
					QList<quint32> toDeleteGRDs;
					for (size_t j = iter.value().flare; j > 0; j--)
					{
						//实测需要先删除已经存在的flare对象
						toReturn.append(QString("-") + QString::number(iter.value().ID + 222222 + j) + QString("\n"));
						toReturn.append(QString::number(iter.value().ID + 222222 + j) + QString(",Type=Misc+Decoy+Flare,T="));
						toReturn.append(QString::number(iter.value().pos.longitude + QRandomGenerator::global()->generateDouble() / INTERVAL, 'f', 9) + QString("|"));
						toReturn.append(QString::number(iter.value().pos.latitude + QRandomGenerator::global()->generateDouble() / INTERVAL, 'f', 9) + QString("|"));
						toReturn.append(QString::number(iter.value().pos.altidude + QRandomGenerator::global()->generateDouble() / INTERVAL, 'f', 9) + QString(","));
						toReturn.append(QString("Color=") + Enum2String(iter.value().coalition) + QString(","));
						toReturn.append(QString("Parent=") + QString::number(iter.value().ID));
						toReturn.append("\n");
						toDeleteGRDs.append(iter.value().ID + 222222 + j);
					}
					pendingDeleteGRD.append(QPair(toDeleteFrameID, toDeleteGRDs));
				}
			}
		}
	}
	return toReturn;
}
#endif
void TacviewServer::OnReadyRead()
{
	QByteArray data = client->readAll();
	QString MSG = QString::fromUtf8(data);
	qDebug() << "Log: Received Tacview Client Reply MSG: " << MSG << " * From IPEndpoint:" << client->peerAddress().toString() << QString::number(client->peerPort());
	if (true)
	{
		QByteArray toSend = "FileType=text/acmi/tacview\nFileVersion=2.2\n";
		startTime = QDateTime::currentDateTime();
		bHasConnected = true;
		QString formattedTime = startTime.toString("yyyy-MM-ddTHH:mm:ssZ\n");
		// 添加前缀
		QString result = QString("0,ReferenceTime=%1").arg(formattedTime);
		toSend.append(result);
		client->write(toSend);
		qDebug() << toSend.constData();
	}
}

void TacviewServer::OnDisConnected()
{
	qDebug() << "Tacview Client Disconnected!";
	bHasConnected = false;
	client->close();
	client = nullptr;
}

void TacviewServer::OnTimeOut()
{
	if (bHasConnected)
	{
		//删除实体
		while (pendingDeletedEntities.count() > 0)
		{
			int todeleteID = pendingDeletedEntities.dequeue();
			QByteArray toSend = "-";
			toSend.append(QString::number(todeleteID));
			toSend.append("\n");
			client->write(toSend, toSend.size());
			client->flush();
			entities.remove(todeleteID);
		}
		//删除航路
		while (pendingDeleteAirRoutes.count() > 0)
		{
			int todeleteID = pendingDeleteAirRoutes.dequeue();
			if (airRoutes.contains(todeleteID))
			{
				for (size_t i = 0; i < airRoutes[todeleteID].waypoints.count(); i++)
				{
					QByteArray toSend = "-";
					toSend.append(QString::number(airRoutes[todeleteID].waypoints[i].ID));
					toSend.append("\n");
					client->write(toSend, toSend.size());
					client->flush();
				}
				airRoutes.remove(todeleteID);
			}
		}

		QDateTime simTimeNow = startTime;
		QString formattedRCDTime = simTimeNow.addMSecs(simStep * frameID).toString("yyyy-MM-ddTHH:mm:ssZ\n");
		// 添加前缀
		QString timeStr = QString("0,RecordingTime=%1").arg(formattedRCDTime);
		QByteArray toSend;
		toSend.append(timeStr.toUtf8());
		//实际线程上下文的切换会有约2ms耗时
		QString aaa = QString::number(simStep * frameID / 1000.0f);
		QString frameStr = "#" + QString::number(simStep * frameID / 1000.0f) + "\n";
		toSend.append(frameStr.toUtf8().constData());
		QString releaseChaff = ReleaseChaff();
		if (releaseChaff != "")
		{
			QByteArray chaff = toSend;
			chaff.append(releaseChaff);
			quint64 result = client->write(chaff, chaff.size());
			if (result != chaff.size())
			{
				qDebug() << "Error:Send Entity MSG failed! Error MSG:" << client->errorString();
			}
			else
			{
				qDebug() << chaff.constData();
			}
		}
#if USE_FLARE
		QString releaseFlare = ReleaseFlare();
		if (releaseFlare != "")
		{
			QByteArray flare = toSend;
			flare.append(releaseFlare);
			quint64 result = client->write(flare, flare.size());
			if (result != flare.size())
			{
				qDebug() << "Error:Send Entity Flare Event failed! Error MSG:" << client->errorString();
			}
			else
			{
				qDebug() << flare.constData();
			}
		}
#endif
		QString airRouteStr = AirRoute2TacviewFormate();
		if (airRouteStr != "")
		{
			QByteArray airroute = toSend;
			airroute.append(airRouteStr);
			quint64 result = client->write(airroute, airroute.size());
			if (result != airroute.size())
			{
				qDebug() << "Error:Send Entity MSG failed! Error MSG:" << client->errorString();
			}
			else
			{
				qDebug() << airroute.constData();
			}
		}
		QString buildStr = Sensor2TacviewFormate();
		if (buildStr != "")
		{
			QByteArray build = toSend;
			build.append(buildStr);
			quint64 result = client->write(build, build.size());
			if (result != build.size())
			{
				qDebug() << "Error:Send Entity MSG failed! Error MSG:" << client->errorString();
			}
			else
			{
				qDebug() << build.constData();
			}
		}
		QString entitesStr = Entities2TacviewFormate();
		if (entitesStr != "")
		{
			QByteArray entite = toSend;
			entite.append(entitesStr);
			quint64 result = client->write(entite, entite.size());
			if (result != entite.size())
			{
				qDebug() << "Error:Send Entity MSG failed! Error MSG:" << client->errorString();
			}
			else
			{
				qDebug() << entite.constData();
			}
		}
		while (pendingUpdateEntitiesWithSensor.count() >= 1)
		{
			QByteArray entiteWithSensor = toSend;
			entiteWithSensor.append(pendingUpdateEntitiesWithSensor.dequeue());
			quint64 result = client->write(entiteWithSensor, entiteWithSensor.size());
			if (result != entiteWithSensor.size())
			{
				qDebug() << "Error:Send EntityWithSensor MSG failed! Error MSG:" << client->errorString();
			}
			else
			{
				qDebug() << entiteWithSensor.constData();
			}
		}
		if (pendingDeleteGRD.count() > 0)
		{
			if (pendingDeleteGRD.begin()->first <= frameID)
			{
				QPair<quint32, QList<quint32>> temp = pendingDeleteGRD.dequeue();
				QByteArray GRD = toSend;
				for (size_t i = 0; i < temp.second.count(); i++)
				{
					GRD.append(QString("-") + QString::number(temp.second[i]));
					GRD.append("\n");
				}
				client->write(GRD, GRD.size());
				client->flush();
			}
		}
		

		frameID += 1;
	}
}

void TacviewServer::OnNewConnection()
{
	client = server->nextPendingConnection();
	if (client != nullptr)
	{
		qDebug() << "New Tacview Client Connected, IPEndpoint:" << client->peerAddress().toString() << ":" << QString::number(client->peerPort());
		connect(client, &QTcpSocket::readyRead, this, &TacviewServer::OnReadyRead);
		connect(client, &QTcpSocket::disconnected, this, &TacviewServer::OnDisConnected);
		//发送握手数据
		QByteArray handsShake = "XtraLib.Stream.0\nTacview.RealTimeTelemetry.0\nhunter\n";
		//服务端握手数据。在TCP握手完成后接收客户端连接后第一帧发从给客户端，等待客户端确认(末尾的\0会被截断，发送时append补充)
		handsShake.append('\0');
		client->write(handsShake, handsShake.size());
		client->flush();
	}
}
