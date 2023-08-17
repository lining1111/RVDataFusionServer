//
// Created by lining on 2/18/22.
//

#include "Poco/ByteOrder.h"
#include "Poco/Net/IPAddress.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/TCPServer.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/TCPServerConnectionFactory.h"
#include "Poco/Net/TCPServerConnection.h"
#include "Poco/Net/TCPServerParams.h"
#include "Poco/AutoPtr.h"
#include "Poco/Exception.h"
#include "Poco/Types.h"
#include "Poco/ByteOrder.h"
#include <iostream>
#include <string>
#include <Poco/Thread.h>

typedef Poco::AutoPtr<Poco::Net::TCPServerParams> Ptr;
using namespace std;

class createconnection : public Poco::Net::TCPServerConnection {
public:
    bool isActive;

    createconnection(const Poco::Net::StreamSocket &s) :
            Poco::Net::TCPServerConnection(s) {
    };

    static void thread1(void *p){
        auto local = (createconnection *) p;
        Poco::Timespan timeOut(1, 0);
        unsigned char Buffer[1000];
        while (local->isActive) {
            if (local->socket().poll(timeOut, Poco::Net::Socket::SELECT_READ) == false) {
                // 空闲状态，累加做发送
                static int num = 0;
                num = num + 1;
                if (20000 == num) {
                    // 到时间可以发送

                }
                //cout << " No Data Recieved till Timeout span . Client connection status : Active "<< flush <<endl;
            } else {
                cout << " Client Message : " << flush;
                int recBytes = -1;

                try {
                    recBytes = local->socket().receiveBytes(Buffer, sizeof(Buffer));
                }
                catch (Poco::Exception &exception) {
                    //Handle network errors.
                    cerr << "Network error: " << exception.displayText() << endl;
                    local->isActive = false;
                }


                if (recBytes == 0) {
                    cout << "Client closes connection!" << endl << flush;
                    local->isActive = false;
                } else {
                    cout << "Receiving recBytes: " << recBytes << endl << flush;
                    cout << "String Recieved : " << Buffer << endl;
                }
            }
            //延时执行一个命令；
            //socket().
        }
    }

    void run() {
        Poco::Net::SocketAddress addr = socket().peerAddress();
        cout << "connection from " << addr.toString() << endl;
        isActive = true;
        socket().setKeepAlive(true);

        Poco::Thread thread;
        thread.start(thread1,this);
        thread.join();

        cout << "Connection finished!" << endl << flush;
    }
};

int main(int argc, char **argv) {       //Setting Server IP address
    //For Windows OS IPAddress("127.0.0.1") might not work hence use of inet_addr is mandatory
    Poco::UInt32 ip = inet_addr("127.0.0.1");
    Poco::Net::IPAddress ip1(&ip, sizeof(ip));
    Poco::Net::SocketAddress ss(ip1, 1234);
    Poco::Net::ServerSocket s(ss, 20);
    Ptr p = new Poco::Net::TCPServerParams();

    //Setting Server Parameters This will allow scaling .

    p->setMaxThreads(64);
    p->setMaxQueued(64);
    p->setThreadIdleTime(50);

    //Setting TcpServer constructor

    Poco::Net::TCPServer t(new Poco::Net::TCPServerConnectionFactoryImpl<createconnection>(), s, p);

    //Starting TCP Server
    t.start();
    cout << "Server Started" << endl;
    cout << "Ready To Accept the connections" << endl;

    while (1);

}