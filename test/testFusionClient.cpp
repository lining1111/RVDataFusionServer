//
// Created by lining on 4/8/22.
//


#include <client/FusionClient.h>
#include <iostream>

int main(int argc, char **argv) {
    FusionClient *client = new FusionClient("127.0.0.1", 10000);
    client->Open();
    client->Run();

    bool isExit = false;

    while (!isExit) {
        string user;

        cout << "please enter(q:quit):" << endl;
        cin >> user;

        if (user == "q") {
            isExit = true;
            continue;
        }

        if (user == "1") {
            Pkg pkg;
            pkg.head.cmd = 0xff;

            pkg.head.len = sizeof(pkg.head) + sizeof(pkg.crc);
            pkg.body = "nihao";
            pkg.head.len += pkg.body.length();

            client->Send(pkg);

        } else if (user == "2") {

        }

    }


}