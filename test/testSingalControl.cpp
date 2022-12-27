//
// Created by lining on 12/26/22.
//
#include <string>

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
    ComFrame_GBT20999_2017 frame;
    frame.frameAll.version = 0x0102;
    frame.frameAll.controlCenterID = 1;
    frame.frameAll.roadTrafficSignalControllerID = 0x12345678;
    frame.frameAll.roadID = 2;
    frame.frameAll.sequence = 1;
    frame.frameAll.type = ComFrame_GBT20999_2017::Type_Query;
    frame.frameAll.dataItemNum = 2;
    ComFrame_GBT20999_2017::DataItem dataItem;
    dataItem.index = 1;
    dataItem.length = 5;
    dataItem.typeID = 1;
    dataItem.objID = 2;
    dataItem.attributeID = 3;
    dataItem.elementID = 4;
    dataItem.data.push_back(5);
    frame.frameAll.dataItems.push_back(dataItem);
    ComFrame_GBT20999_2017::DataItem dataItem1;
    dataItem1.index = 1;
    dataItem1.length = 0;
    frame.frameAll.dataItems.push_back(dataItem1);

    vector<uint8_t> sendBytes;
    frame.setToBytes(sendBytes);
    for (auto iter: sendBytes) {
        printf("%02x ", iter);
    }
    printf("\n");

    ComFrame_GBT20999_2017 frameGet;
    frameGet.getFromBytes(sendBytes);
    printf("over\n");
}

int main() {
//    ComTest();
    ComFrame_GBT20999_2017Test();
}