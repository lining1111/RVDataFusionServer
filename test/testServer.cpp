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
#include <thread>

typedef Poco::AutoPtr<Poco::Net::TCPServerParams> Ptr;
using namespace std;

vector<void *> conns;

class createconnection : public Poco::Net::TCPServerConnection {
public:
    bool isActive;
    string recv;
    Poco::Event ev;

    createconnection(const Poco::Net::StreamSocket &s) :
            Poco::Net::TCPServerConnection(s) {
        conns.push_back(this);
    };

    static void getplain(createconnection *local) {
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
                    local->ev.set();//一定要设置下否则其他的退出不了
                    local->isActive = false;
                } else {
                    for (int i = 0; i < recBytes; i++) {
                        local->recv.push_back(Buffer[i]);
                    }
                    local->ev.set();
                }
            }
        }
    }

    static void printplain(createconnection *local) {
        while (local->isActive) {
            local->ev.wait();
            if (!local->recv.empty()) {
                cout << "client " << local->socket().peerAddress().toString() << " : " << local->recv << endl;
            }
        }
    }


    void run() {
        Poco::Net::SocketAddress addr = socket().peerAddress();
        cout << "connection from " << addr.toString() << endl;
        isActive = true;
        socket().setKeepAlive(true);

        thread thget(getplain, this);
        thread thprint(printplain, this);
        thget.join();
        thprint.join();

        cout << "Connection finished!" << endl << flush;

        for (auto iter = conns.begin(); iter != conns.end(); iter++) {
            auto con = (createconnection *) *iter;
            if (con->socket().peerAddress().toString() == this->socket().peerAddress().toString()) {
                cout << "从数组踢出客户端:" << con->socket().peerAddress().toString() << endl;
                iter = conns.erase(iter);
                break;
            }
        }

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

    while (1) {
        Poco::Thread::sleep(5000);
        cout << "server total client:" << t.currentConnections() << endl;

    }

}