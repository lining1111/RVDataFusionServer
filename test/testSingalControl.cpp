//
// Created by lining on 12/26/22.
//
#include <string>
#include <algorithm>

#include "../signalControl/signalControlCom.h"
#include "../signalControl/SignalControl.h"

void ComTest() {
    //转移字符为0x5c 要转义的字节集为 0x7e 0x7d 0x5c
    vector<uint8_t> in = {0x7e, 0x12, 0x34, 0x56, 0x5c, 0x24, 0x7d};
    uint8_t tf = 0x5c;
    vector<uint8_t> needSet = {0x7e, 0x7d, 0x5c};

    auto out = TransferMean(in, tf, needSet);
    for (int i = 0; i < out.size(); i++) {
        printf("%02x ", out.at(i));
    }
    printf("\n");

    auto result = DeTransferMean(out, tf, needSet);
    for (int i = 0; i < result.size(); i++) {
        printf("%02x ", result.at(i));
    }
    printf("\n");
}

void ComFrame_GBT20999_2017Test() {
    using namespace ComFrame_GBT20999_2017;
    FrameAll frame;
    frame.version = 0x0102;
    frame.controlCenterID = 1;
    frame.roadTrafficSignalControllerID = 0x12345678;
    frame.roadID = 2;
    frame.sequence = 1;
    frame.type = ComFrame_GBT20999_2017::Type_Query;
    frame.dataItemNum = 2;
    ComFrame_GBT20999_2017::DataItem dataItem;
    dataItem.index = 1;
    dataItem.length = 5;
    dataItem.typeID = 1;
    dataItem.objID = 2;
    dataItem.attrID = 3;
    dataItem.elementID = 4;
    dataItem.data.push_back(5);
    frame.dataItems.push_back(dataItem);
    ComFrame_GBT20999_2017::DataItem dataItem1;
    dataItem1.index = 2;
    dataItem1.length = 0;
    frame.dataItems.push_back(dataItem1);

    vector<uint8_t> plainBytes;
    frame.setToBytes(plainBytes);
    printf("plainBytes:");
    for (auto iter: plainBytes) {
        printf("%02x ", iter);
    }
    printf("\n");

    vector<uint8_t> sendBytes;
    frame.setToBytesWithTF(sendBytes);
    printf("sendBytes:");
    for (auto iter: sendBytes) {
        printf("%02x ", iter);
    }
    printf("\n");

    FrameAll frameGet;
    frameGet.getFromBytesWithTF(sendBytes);

    vector<uint8_t> getPlainBytes;
    frameGet.setToBytes(getPlainBytes);

    printf("getPlainBytes:");
    for (auto iter: getPlainBytes) {
        printf("%02x ", iter);
    }
    printf("\n");

    printf("over\n");
}

int main() {
//    ComTest();
//    ComFrame_GBT20999_2017Test();

    std::map<int, string> mapa = {
            {1, "1"},
            {2, "2"},
            {3, "3"},
    };
    string value = "1";

    auto iter = std::find_if(mapa.begin(), mapa.end(), [value](const std::map<int, string>::value_type &item) {
        return (item.second == value);
    });
    if (iter != mapa.end()) {
        printf("1 find\n");
    } else {
        printf("1 not find\n");
    }

    string value1 = "6";

    auto iter1 = std::find_if(mapa.begin(), mapa.end(), [value1](const std::map<int, string>::value_type &item) {
        return (item.second == value1);
    });
    if (iter1 != mapa.end()) {
        printf("6 find\n");
    } else {
        printf("6 not find\n");
    }

}