#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include "TacviewServerTester.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qDebug() << "Hello World!";

    TacviewServerTester* tester = new TacviewServerTester(&a);
    tester->Start();

   /* TacviewServer* server = new TacviewServer(&a);
    server->StartServer(10009);

    TacviewServer::EntityData f35Red((quint32)10001, "F35Red", TacviewServer::EAircraftType::F35, TacviewServer::ECoalition::Red, TacviewServer::Position(121.0, 31.0, 1050), TacviewServer::Attitude(0, 0, 90));

    TacviewServer::EntityData f35Blue((quint32)20001, "F35Blue", TacviewServer::EAircraftType::F35, TacviewServer::ECoalition::Blue, TacviewServer::Position(121.00001, 31.00001, 1060), TacviewServer::Attitude(0, 0, 90));

    QThread::msleep(1000);

    while (true)
    {
        if (server->HasConnectedClient() <= 0)
        {
            QThread::msleep(1000);
            qDebug() << "Has no Client, Waiting for Tacview Client Connected...";
        }
        else
        {
            QThread::msleep(30);
            f35Red.pos.longitude += 0.00000001;
            f35Red.pos.latitude += 0.00000001;
            f35Blue.pos.longitude += 0.00000001;
            f35Blue.pos.latitude += 0.00000001;
            QList<TacviewServer::EntityData> toSend;
            toSend.append(f35Red);
            toSend.append(f35Blue);
            server->SendEntities(toSend);
            qDebug() << "Entities Data has been Send.";
        }
       
    }*/

    return a.exec();
}
