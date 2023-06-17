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

    // 7e 0022 0100 05 000c48fc 01 13 30 03
    // 01 05 11 01 02 01 01
    // 02 05 11 01 03 01 01
    // 03 05 11 01 04 01 21
    // e443 7d

    using namespace ComFrame_GBT20999_2017;
    FrameAll frame;
    frame.version = 0x0100;
    frame.controlCenterID = 0x05;
    frame.roadTrafficSignalControllerID = 0x000c48fc;
    frame.roadID = 0x01;
    frame.sequence = 0x13;
    frame.type = ComFrame_GBT20999_2017::Type_Set;
    frame.dataItemNum = 0x03;
    ComFrame_GBT20999_2017::DataItem dataItem;
    dataItem.index = 1;
    dataItem.length = 5;
    dataItem.typeID = 0x11;
    dataItem.objID = 0x01;
    dataItem.attrID = 0x02;
    dataItem.elementID = 0x01;
    dataItem.data.push_back(0x01);
    frame.dataItems.push_back(dataItem);
    ComFrame_GBT20999_2017::DataItem dataItem1;
    dataItem1.index = 1;
    dataItem1.length = 5;
    dataItem1.typeID = 0x11;
    dataItem1.objID = 0x01;
    dataItem1.attrID = 0x03;
    dataItem1.elementID = 0x01;
    dataItem1.data.push_back(0x01);
    frame.dataItems.push_back(dataItem1);
    ComFrame_GBT20999_2017::DataItem dataItem2;
    dataItem2.index = 1;
    dataItem2.length = 5;
    dataItem2.typeID = 0x11;
    dataItem2.objID = 0x01;
    dataItem2.attrID = 0x04;
    dataItem2.elementID = 0x01;
    dataItem2.data.push_back(0x21);
    frame.dataItems.push_back(dataItem2);

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
    ComFrame_GBT20999_2017Test();

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
        printf("1 key:%d\n",iter->first);
        printf("1 find\n");
    } else {
        printf("1 not find\n");
    }

    string value1 = "6";

    auto iter1 = std::find_if(mapa.begin(), mapa.end(), [value1](const std::map<int, string>::value_type &item) {
        return (item.second == value1);
    });
    if (iter1 != mapa.end()) {
        printf("6 key:%d\n",iter1->first);
        printf("6 find\n");
    } else {
        printf("6 not find\n");
    }

}