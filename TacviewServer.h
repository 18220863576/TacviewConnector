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

//为保证兼容性定义此宏，实际生产环境中（FD团队项目中启用此宏）
#define FD_VERSION 0

#if FD_VERSION
#include "../Interface/DefineDoc.h"
#include "../Interface/StructGFT.h"
#endif

//flare目前不生效，原因未知。待后续修复这一问题后再启用相关功能代码
#define USE_FLARE 1
//控制chaff和flare显示的随机抛射间隔，数值越大，间隔越小，散列程度越低
#define INTERVAL 150.0f
//控制是否使用重绘模式，当使用重绘模式时会先删除已存在的对象，然后重新绘制
//不适用重绘模式时，已存在对象不会删除而是会线性移动到新的位置
//目前仅针对传感器和航路点对象生效，GRD暂不纳入控制范围
#define REDRAW 1
//定义GRD的持续时间。以秒为单位
#define GRD_TIME 1.5

class TacviewServer  : public QObject
{
	Q_OBJECT


public:
	//阵营枚举
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

	//机型枚举
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

//位置
struct Position
{
		//经度
	double longitude;
	//纬度
	double latitude;
	//高度
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
			return *this; // 返回当前对象，实际应用中可能需要处理错误或抛出异常
		}
		
	}
};
//姿态
struct Attitude
{
	//航向
	float heading;
	//俯仰角
	float pitch;
	//滚转角
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
//视场
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
//飞控
struct FlyControl
{
	//攻角
	float AOA;
	//侧滑角
	float AOS;
	//过载
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
//实体数据
struct EntityData
{
	//实体唯一标识符
	quint32 ID;
	//实体名称
	QString name;
	//实体型号
	EAircraftType type;
	//实体阵营
	ECoalition coalition;
	//箔条弹，释放箔条弹时值为非0值，代表每次释放的数量
	quint32 chaff;
#if USE_FLARE
	//热诱弹，释放热诱弹时值为非0值，代表每次释放的数量
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
	/// 将实例结构体数据转换成Tacview可识别的用于更新实体数据的一帧文本数据格式
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
	/// 将结构体数据转换成Tacview可识别的用于创建实体数据的一帧文本数据格式
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
//航路点
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
//航路
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
			//最后一个航路点没有Next
			if (i < waypoints.count() - 1)
			{
				//Next属性通常用于指定下一个航路点，将航路点连起来
				toReturn.append(QString(",Next=") + QString::number(waypoints[i + 1].ID));
			}
			toReturn.append("\n");
		}
		return toReturn;
	}
};
//探测器范围（威胁区）
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
	/// 启动等待Tacview客户端连接的TCP服务端
	/// </summary>
	/// <param name="tcpServerPort">指定监听来自Tacview客户端的TCP连接请求的本机端口</param>
	/// <param simStep="tcpServerPort">以毫秒计的仿真步长</param>
	/// <returns></returns>
	bool StartServer(int tcpServerPort, int _simStep = 30);

	//void run() override;

#if FD_VERSION
	//  转译SimulationInfo
	TacviewServer::EntityData get_entity_data(const SimulationInfo& simInfo);
#endif

	/// <summary>
	/// 更新一个实体数据
	/// </summary>
	/// <param name="entitiesData">实体数据</param>
	void UpdateEntity(const EntityData entitieData);
	/// <summary>
	/// 更新一个具有传感器的实体数据
	/// </summary>
	/// <param name="entitieData">实体数据</param>
	/// <param name="buildingData">传感器数据</param>
	void UpdateEntityWithSensor(const EntityData entitieData, const Sensor buildingData);
	/// <summary>
	/// 删除一个实体
	/// </summary>
	/// <param name="entitieID">要删除的实体的ID</param>
	void DeleteEntity(const quint32 entitieID); 
	/// <summary>
	/// 更新一条航路数据（可以添加新的航路也可以更新旧航路）
	/// </summary>
	/// <param name="airRouteData"></param>
	void UpdateAirRoute(const AirRoute airRouteData);
	/// <summary>
	/// 删除一条航路
	/// </summary>
	/// <param name="airRouteID"></param>
	void DeleteAirRoute(const quint32 airRouteID);
	/// <summary>
	/// 更新一个传感器（威胁区），可以添加新的，也可以更新已存在的
	/// </summary>
	/// <param name="buildingData"></param>
	void UpdateSensor(const Sensor buildingData);
	/// <summary>
	/// 删除一个已存在的传感器（威胁区）
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
	//保护entities的互斥锁
	QMutex entitiesMutex;
private:
	
	/// <summary>
	/// 每帧调用本函数，将缓冲池中的所有实体信息分类处理
	/// 并转换成Tacview可识别的ACMI格式字符串
	/// </summary>
	/// <returns></returns>
	QByteArray Entities2TacviewFormate();
	/// <summary>
	/// 每帧调用本函数，将缓冲池中的所有航路信息分类处理
	/// 并转换成Tacview可识别的ACMI格式字符串
	/// </summary>
	/// <returns></returns>
	QByteArray AirRoute2TacviewFormate();
	/// <summary>
	/// 每帧调用本函数，检测实体是否有释放箔条弹行为，若有则生成释放箔条弹事件
	/// 发送给Tacview执行释放箔条弹行为
	/// </summary>
	/// <returns></returns>
	QByteArray ReleaseChaff();
	/// <summary>
	/// 每帧调用本函数，将缓冲池中的所有传感器（威胁区）信息分类处理
	/// </summary>
	/// <returns></returns>
	QByteArray Sensor2TacviewFormate();
#if USE_FLARE
	/// <summary>
	/// 每帧调用本函数，检测实体是否有释放热诱弹行为，若有则生成释放热诱弹事件
	/// 发送给Tacview执行释放热诱弹行为
	/// </summary>
	/// <returns></returns>
	QByteArray ReleaseFlare();
#endif

private slots:
	//接受Tacview客户端连接请求的槽函数
	void OnNewConnection();
	//客户端返回的确认数据
	void OnReadyRead();

	void OnDisConnected();

	void OnTimeOut();
};
