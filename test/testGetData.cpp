//
// Created by lining on 3/3/22.
//
#include "GetData.h"

using namespace std;

int main(int argc, char **argv) {

//    vector<string> files;
//    GetFiles("./test",files);

    string file = "./test/data.txt";
    GetData *getData = new GetData(file);
    for (int i = 0; i < getData->data.size(); i++) {
        vector<string> iter = getData->data.at(i);
        for (int j = 0; j < iter.size(); j++) {
            printf("%s \t", iter.at(j).c_str());
        }
        printf("\n");
    }
    delete getData;
    return 0;

}