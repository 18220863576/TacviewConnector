#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>
#include <QQueue>
#include <QDateTime>
#include <QMutex>
#include <iostream>

//Ϊ��֤�����Զ���˺꣬ʵ�����������У�FD�Ŷ���Ŀ�����ô˺꣩
#define FD_VERSION 0

#if FD_VERSION
#include "../Interface/DefineDoc.h"
#include "../Interface/StructGFT.h"
#endif

//flareĿǰ����Ч��ԭ��δ֪���������޸���һ�������������ع��ܴ���
#define USE_FLARE 1
//����chaff��flare��ʾ���������������ֵԽ�󣬼��ԽС��ɢ�г̶�Խ��
#define INTERVAL 150.0f
//�����Ƿ�ʹ���ػ�ģʽ����ʹ���ػ�ģʽʱ����ɾ���Ѵ��ڵĶ���Ȼ�����»���
//�������ػ�ģʽʱ���Ѵ��ڶ��󲻻�ɾ�����ǻ������ƶ����µ�λ��
//Ŀǰ����Դ������ͺ�·�������Ч��GRD�ݲ�������Ʒ�Χ
#define REDRAW 1
//����GRD�ĳ���ʱ�䡣����Ϊ��λ
#define GRD_TIME 1.5

class TacviewServer  : public QObject
{
	Q_OBJECT


public:
	//��Ӫö��
	enum ECoalition
	{
		Red,
		Blue
	};
	static const char* Enum2String(ECoalition value)
	{
		static const QMap<ECoalition, const char*> enumStrings
		{
		   {ECoalition::Red, "Red"},
		   {ECoalition::Blue, "Blue"}
		};
		return enumStrings.value(value, "Unknown");
	}

	//����ö��
	enum EAircraftType
	{
		SU_27,
		F35,
		F18,
		F22,
		AIM_120C,
	};
	static const char* Enum2String(EAircraftType value)
	{
		static const QMap<EAircraftType, const char*> enumStrings
		{
		   {EAircraftType::SU_27, "Su-37"},
		   {EAircraftType::F35, "F35"},
			{EAircraftType::F18, "F18"},
			{EAircraftType::F22, "F22"},
			{EAircraftType::AIM_120C,"AIM-120C"}
		};
		return enumStrings.value(value, "Unknown");
	}
		

#pragma pack(push, 1)

//λ��
struct Position
{
		//����
	double longitude;
	//γ��
	double latitude;
	//�߶�
	double altidude;
	Position()
	{
		longitude = 0;
		latitude = 0;
		altidude = 0;
	}
	Position(double lon, double lat, float alt) 
	{	longitude = lon; 
		latitude = lat; 
		altidude = alt; 
	}

	Position operator+(const Position& posB) const
	{
		return Position(this->longitude + posB.longitude, this->latitude + posB.latitude, this->altidude + posB.altidude);
	}
	Position operator-(const Position& posB) const
	{
		return Position(this->longitude - posB.longitude, this->latitude - posB.latitude, this->altidude - posB.altidude);
	}
	Position operator*(const double scalar) const
	{
		return Position(this->longitude * scalar, this->latitude * scalar, this->altidude * scalar);
	}
	Position operator/(const double scalar) const
	{
		if (scalar != 0)
		{
			return Position(this->longitude / scalar, this->latitude / scalar, this->altidude / scalar);
		}
		else
		{
			std::cerr << "Error: Division by zero." << std::endl;
			return *this; // ���ص�ǰ����ʵ��Ӧ���п�����Ҫ���������׳��쳣
		}
		
	}
};
//��̬
struct Attitude
{
	//����
	float heading;
	//������
	float pitch;
	//��ת��
	float roll;
	Attitude()
	{
		heading = 0;
		pitch = 0;
		roll = 0;
	}
	Attitude(float _heading, float _pitch, float _roll)
	{
		heading = _heading;
		pitch = _pitch;
		roll = _roll;
	}
};
//�ӳ�
struct AOV 
{
	float horizontal;
	float vertical;
	float radius;

	AOV()
	{
		horizontal = 0;
		vertical = 0;
		radius = 0;
	}

	AOV(const float _hor, const float _ver, const float _rad)
	{
		horizontal = _hor;
		vertical = _ver;
		radius = _rad;
	}
};
//�ɿ�
struct FlyControl
{
	//����
	float AOA;
	//�໬��
	float AOS;
	//����
	float G;
	FlyControl()
	{
		AOA = 0;
		AOS = 0;
		G = 0;
	}
	FlyControl(float aoa, float aos, float g)
	{
		AOA = aoa;
		AOS = aos;
		G = g;
	}
};
//ʵ������
struct EntityData
{
	//ʵ��Ψһ��ʶ��
	quint32 ID;
	//ʵ������
	QString name;
	//ʵ���ͺ�
	EAircraftType type;
	//ʵ����Ӫ
	ECoalition coalition;
	//���������ͷŲ�����ʱֵΪ��0ֵ������ÿ���ͷŵ�����
	quint32 chaff;
#if USE_FLARE
	//���յ����ͷ����յ�ʱֵΪ��0ֵ������ÿ���ͷŵ�����
	quint32 flare;
#endif
	Position pos;
	Attitude att;
	FlyControl flyctrl;
	EntityData()
	{
		ID = 0;
		name = "";
		type = EAircraftType::F18;
		coalition = ECoalition::Blue;
		chaff = 0;
#if USE_FLARE
		flare = 0;
#endif	
		pos = Position();
		att = Attitude();
		flyctrl = FlyControl();
	}
	EntityData(quint32 _id, QString _name, EAircraftType _title, ECoalition _camp = ECoalition::Red, quint32 _chaff = 0,
#if USE_FLARE
		quint32 _flare= 0, 
#endif
		Position _pos = Position(), Attitude _att = Attitude(), FlyControl _fc = FlyControl())
	{
		ID = _id;
		name = _name;
		type = _title;
		coalition = _camp;
		chaff = _chaff;
#if USE_FLARE
		flare = _flare;
#endif
		pos = _pos;
		att = _att;
		flyctrl = _fc;
	}
	/// <summary>
	/// ��ʵ���ṹ������ת����Tacview��ʶ������ڸ���ʵ�����ݵ�һ֡�ı����ݸ�ʽ
	/// </summary>
	/// <returns></returns>
	QString toStringUpdate()
	{
		QString toReturn;
		toReturn.append(QString::number(ID));
		toReturn.append(",T=");
		toReturn.append(QString::number(pos.longitude, 'f', 9));
		toReturn.append('|');
		toReturn.append(QString::number(pos.latitude, 'f', 9));
		toReturn.append('|');
		toReturn.append(QString::number(pos.altidude, 'f', 2));
		toReturn.append('|');
		toReturn.append(QString::number(att.roll, 'f', 2));
		toReturn.append('|');
		toReturn.append(QString::number(att.pitch, 'f', 2));
		toReturn.append('|');
		toReturn.append(QString::number(att.heading, 'f', 2));
		toReturn.append('\n');
		if (toReturn.contains("0,T=0.0000"))
		{
			//qDebug() << "Error!";
			toReturn.clear();
		}
		return toReturn;
	}
	/// <summary>
	/// ���ṹ������ת����Tacview��ʶ������ڴ���ʵ�����ݵ�һ֡�ı����ݸ�ʽ
	/// </summary>
	/// <returns></returns>
	QString toStringCreate()
	{
		QString toReturn = toStringUpdate();
		if (toReturn.contains('\n'))
		{
			toReturn.replace('\n', QString());
		}
		toReturn.append(",Name=");
		toReturn.append(TacviewServer::Enum2String(type));
		toReturn.append(",ShortName=");
		toReturn.append(name);
		toReturn.append(",Color=");
		toReturn.append(TacviewServer::Enum2String(coalition));
		toReturn.append("\n");
		return toReturn;
	}
};
//��·��
struct WayPoint
{
	quint32 ID;
	QString name;
	Position pos;

	WayPoint(const quint32 _id, const QString _name, const Position _pos)
	{
		ID = _id;
		name = _name;
		pos = _pos;
	}

};
//��·
struct AirRoute
{
	quint32 ID;
	QString name;
	ECoalition coalition;
	QList<WayPoint> waypoints;

	QString toString()
	{
		QString toReturn;
		for (size_t i = 0; i < waypoints.count(); i++)
		{
			toReturn.append(QString::number(waypoints[i].ID) + QString(",Type=Navaid+Static+Waypoint,T="));
			toReturn.append(QString::number(waypoints[i].pos.longitude, 'f', 9) + QString("|"));
			toReturn.append(QString::number(waypoints[i].pos.latitude, 'f', 9) + QString("|"));
			toReturn.append(QString::number(waypoints[i].pos.altidude, 'f', 9) + QString(","));
			toReturn.append(QString("Name=") + waypoints[i].name + QString(","));
			toReturn.append(QString("Color=") + Enum2String(coalition));
			//���һ����·��û��Next
			if (i < waypoints.count() - 1)
			{
				//Next����ͨ������ָ����һ����·�㣬����·��������
				toReturn.append(QString(",Next=") + QString::number(waypoints[i + 1].ID));
			}
			toReturn.append("\n");
		}
		return toReturn;
	}
};
//̽������Χ����в����
struct Sensor
{
	quint32 ID;
	QString name;
	ECoalition coalition;
	Position pos;
	Attitude att;
	AOV aov;
	Sensor()
	{
		ID = 0;
		name = "";
		coalition = ECoalition::Red;
		pos = Position();
		att = Attitude();
		aov = AOV();
	}

	Sensor(const quint32 _id, const QString _name, const ECoalition _coa, const Position _pos, const Attitude _att, const AOV _aov)
	{
		ID = _id;
		name = _name;
		coalition = _coa;
		pos = _pos;
		att = _att;
		aov = _aov;
	}

	QString toString()
	{
		QString toReturn;
		toReturn.append(QString::number(ID) + QString(",T="));
		toReturn.append(QString::number(pos.longitude, 'f', 9) + QString("|"));
		toReturn.append(QString::number(pos.latitude, 'f', 9) + QString("|"));
		toReturn.append(QString::number(pos.altidude, 'f', 9) + QString(","));
		toReturn.append(QString("Name=") + name + QString(","));
		toReturn.append(QString("Color=") + Enum2String(coalition) + QString(","));
		toReturn.append(QString("Visible=1") + QString(","));
		toReturn.append(QString("RadarMode=1,"));
		toReturn.append(QString("RadarAzimuth=") + QString::number(att.heading) + QString(","));
		toReturn.append(QString("RadarElevation=") + QString::number(att.pitch) + QString(","));
		toReturn.append(QString("RadarRoll=") + QString::number(att.roll) + QString(","));
		toReturn.append(QString("RadarHorizontalBeamwidth=") + QString::number(aov.horizontal) + QString(","));
		toReturn.append(QString("RadarVerticalBeamwidth=") + QString::number(aov.vertical) + QString(","));
		toReturn.append(QString("RadarRange=") + QString::number(aov.radius));
		toReturn.append("\n");
		return toReturn;
	}
};

#pragma pack(pop)

public:
	TacviewServer(QObject *parent);
	~TacviewServer();
	/// <summary>
	/// �����ȴ�Tacview�ͻ������ӵ�TCP�����
	/// </summary>
	/// <param name="tcpServerPort">ָ����������Tacview�ͻ��˵�TCP��������ı����˿�</param>
	/// <param simStep="tcpServerPort">�Ժ���Ƶķ��沽��</param>
	/// <returns></returns>
	bool StartServer(int tcpServerPort, int _simStep = 30);

	//void run() override;

#if FD_VERSION
	//  ת��SimulationInfo
	TacviewServer::EntityData get_entity_data(const SimulationInfo& simInfo);
#endif

	/// <summary>
	/// ����һ��ʵ������
	/// </summary>
	/// <param name="entitiesData">ʵ������</param>
	void UpdateEntity(const EntityData entitieData);
	/// <summary>
	/// ����һ�����д�������ʵ������
	/// </summary>
	/// <param name="entitieData">ʵ������</param>
	/// <param name="buildingData">����������</param>
	void UpdateEntityWithSensor(const EntityData entitieData, const Sensor buildingData);
	/// <summary>
	/// ɾ��һ��ʵ��
	/// </summary>
	/// <param name="entitieID">Ҫɾ����ʵ���ID</param>
	void DeleteEntity(const quint32 entitieID); 
	/// <summary>
	/// ����һ����·���ݣ���������µĺ�·Ҳ���Ը��¾ɺ�·��
	/// </summary>
	/// <param name="airRouteData"></param>
	void UpdateAirRoute(const AirRoute airRouteData);
	/// <summary>
	/// ɾ��һ����·
	/// </summary>
	/// <param name="airRouteID"></param>
	void DeleteAirRoute(const quint32 airRouteID);
	/// <summary>
	/// ����һ������������в��������������µģ�Ҳ���Ը����Ѵ��ڵ�
	/// </summary>
	/// <param name="buildingData"></param>
	void UpdateSensor(const Sensor buildingData);
	/// <summary>
	/// ɾ��һ���Ѵ��ڵĴ���������в����
	/// </summary>
	/// <param name="sensorID"></param>
	void DeleteSensor(const quint32 sensorID);

private:
	QTcpServer* server = nullptr;
	QTcpSocket* client = nullptr;
	QTimer* timer = nullptr;
	QDateTime startTime;
	int simStep = 30;
	quint32 frameID = 0;
	bool bHasConnected = false;	
	QMap<int, EntityData> entities;
	QQueue<EntityData> pendingUpdateEntities;
	QQueue<quint32> pendingDeletedEntities;
	QMap<int, AirRoute> airRoutes;
	QQueue<AirRoute> pendingUpdateAirRoutes;
	QQueue<quint32> pendingDeleteAirRoutes;
	QMap<int, Sensor> sensors;
	QQueue<Sensor> pendingUpdateSensor;
	QQueue<quint32> pendingDeleteSensor;
	QQueue<QByteArray> pendingUpdateEntitiesWithSensor;
	QQueue<QPair<quint32, QList<quint32>>> pendingDeleteGRD;
	//����entities�Ļ�����
	QMutex entitiesMutex;
private:
	
	/// <summary>
	/// ÿ֡���ñ���������������е�����ʵ����Ϣ���ദ��
	/// ��ת����Tacview��ʶ���ACMI��ʽ�ַ���
	/// </summary>
	/// <returns></returns>
	QByteArray Entities2TacviewFormate();
	/// <summary>
	/// ÿ֡���ñ���������������е����к�·��Ϣ���ദ��
	/// ��ת����Tacview��ʶ���ACMI��ʽ�ַ���
	/// </summary>
	/// <returns></returns>
	QByteArray AirRoute2TacviewFormate();
	/// <summary>
	/// ÿ֡���ñ����������ʵ���Ƿ����ͷŲ�������Ϊ�������������ͷŲ������¼�
	/// ���͸�Tacviewִ���ͷŲ�������Ϊ
	/// </summary>
	/// <returns></returns>
	QByteArray ReleaseChaff();
	/// <summary>
	/// ÿ֡���ñ���������������е����д���������в������Ϣ���ദ��
	/// </summary>
	/// <returns></returns>
	QByteArray Sensor2TacviewFormate();
#if USE_FLARE
	/// <summary>
	/// ÿ֡���ñ����������ʵ���Ƿ����ͷ����յ���Ϊ�������������ͷ����յ��¼�
	/// ���͸�Tacviewִ���ͷ����յ���Ϊ
	/// </summary>
	/// <returns></returns>
	QByteArray ReleaseFlare();
#endif

private slots:
	//����Tacview�ͻ�����������Ĳۺ���
	void OnNewConnection();
	//�ͻ��˷��ص�ȷ������
	void OnReadyRead();

	void OnDisConnected();

	void OnTimeOut();
};
