#include "TacviewServerTester.h"
#include <QRandomGenerator>

/// <summary>
/// 生成范围内随机整数，默认值0~100
/// </summary>
/// <param name="min">最小值</param>
/// <param name="max">最大值</param>
/// <returns></returns>
int RandomInt(int min = 0, int max = 100)
{
    return QRandomGenerator::global()->bounded(min, max + 1);
}
/// <summary>
/// 生成范围内随机浮点数，默认值0~1.0
/// </summary>
/// <param name="min">最小值</param>
/// <param name="max">最大值</param>
/// <returns></returns>
double RandomDouble(double min = 0, double max = 1)
{
    return min + (max - min) * QRandomGenerator::global()->generateDouble();
}

TacviewServerTester::TacviewServerTester(QObject *parent)
	: QThread(parent)
{

}

TacviewServerTester::~TacviewServerTester()
{

}

void TacviewServerTester::Start()
{
    server = new TacviewServer(this);
    server->StartServer(22222,100);
    __super::start();
}

void TacviewServerTester::run()
{
    QThread::msleep(100);
    int frameCount = 0;

    TacviewServer::AirRoute routeRed, routeBlue;
    routeRed.ID = 999999999;
    routeRed.name = "红方航线";
    routeRed.coalition = TacviewServer::ECoalition::Red;
    routeRed.waypoints.append(TacviewServer::WayPoint(111111111, "RedWP1", TacviewServer::Position(121.2, 29, 3100)));
    routeRed.waypoints.append(TacviewServer::WayPoint(111111112, "RedWP2", TacviewServer::Position(121.5, 29.3, 4000)));
    routeRed.waypoints.append(TacviewServer::WayPoint(111111113, "RedWP3", TacviewServer::Position(122, 29.5, 6500)));
    routeRed.waypoints.append(TacviewServer::WayPoint(111111114, "RedWP4", TacviewServer::Position(122.4, 29.7, 4000)));
    routeRed.waypoints.append(TacviewServer::WayPoint(111111115, "RedWP5", TacviewServer::Position(122.8, 30.2, 2500)));

    routeBlue.ID = 888888888;
    routeBlue.name = "蓝方航线";
    routeBlue.coalition = TacviewServer::ECoalition::Blue;
    routeBlue.waypoints.append(TacviewServer::WayPoint(222222221, "BlueWP1", TacviewServer::Position(121, 25, 2500)));
    routeBlue.waypoints.append(TacviewServer::WayPoint(222222222, "BlueWP1", TacviewServer::Position(123, 23, 3000)));
    routeBlue.waypoints.append(TacviewServer::WayPoint(222222223, "BlueWP1", TacviewServer::Position(124, 26, 3300)));
    routeBlue.waypoints.append(TacviewServer::WayPoint(222222224, "BlueWP1", TacviewServer::Position(126, 29, 2500)));
    routeBlue.waypoints.append(TacviewServer::WayPoint(222222225, "BlueWP1", TacviewServer::Position(127, 28, 4000)));


    TacviewServer::Sensor redBuild(555555555, "RedSensor", TacviewServer::ECoalition::Red, TacviewServer::Position(121, 29, 500), TacviewServer::Attitude(0, 30, 0), TacviewServer::AOV(360, 180, 20000));

    while (true)
    {
        frameCount += 1;
        QThread::msleep(100);
        f35Blue.pos.longitude += 0.001;
        f35Blue.pos.latitude += 0.001;
        f35Blue.pos.altidude = sin(frameCount / 30.0f) * 1000 + 3000;
        f35Blue.att.heading = 45.0f;
        f35Blue.att.pitch = cos(frameCount / 30.0f) * 15.0;
       
        server->UpdateEntity(f35Blue);
        f22Red.pos.longitude += 0.001;
        f22Red.pos.latitude += 0.0009;
        f22Red.pos.altidude = -sin(frameCount / 30.0f) * 2000 + 6000;
        f22Red.att.heading = 42.0f;
        f22Red.att.pitch = -cos(frameCount / 30.0f) * 30.0;
        if (frameCount % 80 == 0 )
        {
            f22Red.chaff = 5;
#if USE_FLARE
            f35Blue.flare = 5;
#endif
        }
        else
        {
            f22Red.chaff = 0;
#if USE_FLARE 
            f35Blue.flare = 0;
#endif
        }
        
       

        //第101帧发射DD
        if (frameCount >= 101 && frameCount <= 201)
        {
            if (frameCount == 101)
            {
                aim120c.pos = f22Red.pos;
            }
            aim120c.pos = aim120c.pos + TacviewServer::Position(0.0012, 0.0013, 0);
            aim120c.att.heading = 45;
            server->UpdateEntity(aim120c);
            //201帧删除DD
            if (frameCount == 201)
            {
                server->DeleteEntity(aim120c.ID);
            }
        }
        
        //第110帧添加航路
        if (frameCount == 110)
        {
            server->UpdateAirRoute(routeRed);
        }
        if (frameCount == 120)
        {
            server->UpdateAirRoute(routeBlue);
        }
        ////第150帧添加建筑（威胁区）
        //if (frameCount == 150)
        //{
        //    server->UpdateSensor(redBuild);
        //}
        
        if (frameCount % 150 == 0)
        {
            server->DeleteAirRoute(routeRed.ID);
        }

        if (frameCount % 190 == 0)
        {
            routeRed.waypoints.clear();
            int waypointCount = RandomInt(3,15);
            for (size_t i = 0; i < waypointCount; i++)
            {
                routeRed.waypoints.append(TacviewServer::WayPoint(111111111 + i, QString("RedWP") + QString::number(i), TacviewServer::Position(121.2 + RandomDouble(-2, 2), 29 + RandomDouble(-2, 2), 3100 + RandomInt(-2000, 2000))));
            }
            server->UpdateAirRoute(routeRed);
        }

        //第200帧红方开启传感器
        if (frameCount >= 200)
        {
            server->UpdateEntityWithSensor(f22Red, redBuild);
        }
        else//200帧之前红方不开传感器
        {
            server->UpdateEntity(f22Red);
        }
    }
}


