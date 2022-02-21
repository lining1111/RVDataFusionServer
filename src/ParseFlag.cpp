//
// Created by lining on 2/21/22.
//

#include <iostream>
#include "ParseFlag.h"

ParseFlag::ParseFlag(map<string, string> useAge) {
    this->useAgeAll.insert(useAge.begin(), useAge.end());
    this->useAgeSet.clear();
}

ParseFlag::~ParseFlag() {
    this->useAgeAll.clear();
    this->useAgeSet.clear();
}

void ParseFlag::Parse(int argc, char **argv) {
    if (argc < 2) {
        return;
    }
    int index = 1;
    do {
        auto iter = useAgeAll.find(string(argv[index]));
        if (iter != useAgeAll.end()) {
            //找到了
            if (index + 1 < argc) {
                if (string(argv[index + 1]).at(0) != '-') {
                    useAgeSet[iter->first] = argv[index + 1];
                    index += 2;
                } else {
                    index++;
                    cout << iter->first + "not set" << endl;
                }
            } else {
                index++;
                cout << iter->first + "not set" << endl;
            }
        } else {
            if (string(argv[index]).at(0) == '-') {
                cout << string(argv[index]) + " not find" << endl;
            }
            index++;
        }
    } while (index < argc);
}
