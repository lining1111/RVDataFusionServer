//
// Created by lining on 12/26/22.
//

#include "signalControlCom.h"
#include <stdexcept>
#include "common/CRC.h"

using namespace std;

//转义字符问题处理
static bool isInSet(uint8_t in, vector<uint8_t> set) {
    bool ret = false;
    for (auto iter : set) {
        if (iter == in) {
            ret = true;
            break;
        }
    }
    return ret;
}

vector<uint8_t> TransferMean(vector<uint8_t> in, uint8_t tf, vector<uint8_t> needSet) {
    vector<uint8_t> out;
    out.assign(in.begin(), in.end());
    for (int i = 0; i < out.size(); i++) {
        if (isInSet(out.at(i), needSet)) {
            out.insert(out.begin() + i, tf);
            i++;
        }
    }
    return out;
}

vector<uint8_t> DeTransferMean(vector<uint8_t> in, uint8_t tf, vector<uint8_t> needSet) {
    vector<uint8_t> out;
    out.assign(in.begin(), in.end());
    for (int i = 0; i < out.size(); i++) {
        if (out.at(i) == tf && i < (out.size() - 1)) {
            if (isInSet(out.at(i + 1), needSet)) {
                out.erase(out.begin() + i);
                out.shrink_to_fit();
            }
        }
    }
    return out;
}

//大小端字节转换
static uint16_t reverse16(uint16_t value) {
    return __bswap_constant_16(value);
}

static uint32_t reverse32(uint32_t value) {
    return __bswap_constant_32(value);
}

static uint64_t reverse64(uint64_t value) {
    return __bswap_constant_64(value);
}


namespace ComFrame_GBT20999_2017 {

    int DataItem::getFromBytes(vector<uint8_t> in) {
        int needLen = 2;
        if (in.size() < needLen) {
//        throw std::length_error("ComFrame_GBT20999_2017::DataItem::getFromByte len less " + to_string(needLen));
            return -1;
        }
        int index = 0;
        //数据值索引
        this->index = in[index++];
        //数据值长度
        this->length = in[index++];
        if (this->length == 0) {
            return 0;
        }
        //数据类ID
        this->typeID = in[index++];
        //对象ID
        this->objID = in[index++];
        //属性ID
        this->attrID = in[index++];
        //元素ID
        this->elementID = in[index++];
        //数据值
        if (index < in.size()) {
            this->data.assign(in.begin() + index, in.end());
        }
        return 0;
    }

    int DataItem::setToBytes(vector<uint8_t> &out) {
        //数据值索引
        out.push_back(this->index);
        //数据值长度
        out.push_back(this->length);
        if (this->length == 0) {
            return 0;
        }
        //数据类ID
        out.push_back(this->typeID);
        //对象ID
        out.push_back(this->objID);
        //属性ID
        out.push_back(this->attrID);
        //元素ID
        out.push_back(this->elementID);
        //数据值
        out.insert(out.end(), this->data.begin(), this->data.end());
        return 0;
    }

    int FrameAll::getFromBytes(vector<uint8_t> in) {
        int needLen = 17;
        if (in.size() < needLen) {
//        throw std::length_error("ComFrame_GBT20999_2017::FrameAll::getFromByte len less " + to_string(needLen));
            return -1;
        }

        uint16_t value16;
        uint32_t value32;
        int index = 0;
        //开始字节
        this->begin = in[index++];
        //报文长度
        value16 = ((uint16_t) in[index] << 8) + ((uint16_t) in[index + 1]);
        this->length = value16;
        index += 2;
        //协议版本
        value16 = ((uint16_t) in[index] << 8) + ((uint16_t) in[index + 1]);
        this->version = value16;
        index += 2;
        //上位机ID
        this->controlCenterID = in[index++];
        //信号机ID
        value32 = ((uint32_t) in[index] << 24) + ((uint32_t) in[index + 1] << 16) + ((uint32_t) in[index + 2] << 8) +
                  ((uint32_t) in[index + 3]);
        this->roadTrafficSignalControllerID = value32;
        index += 4;
        //路口ID
        this->roadID = in[index++];
        //帧流水号
        this->sequence = in[index++];
        //帧类型
        this->type = in[index++];
        //数据值数量
        this->dataItemNum = in[index++];
        //根据数据值数量，获取数据值数组
        for (int i = 0; i < this->dataItemNum; i++) {
            DataItem dataItem;
            //先获取数据值长度，然后根据长度分割数组
            uint8_t dataItemLen = in[index + 1] + 2;
            vector<uint8_t> inItem;
            auto beginPos = in.begin() + index;
            auto endPos = beginPos + dataItemLen;
            inItem.assign(beginPos, endPos);
            dataItem.getFromBytes(inItem);
            this->dataItems.push_back(dataItem);

            //更新位置信息
            index += dataItemLen;
        }
        //CRC16校验
        value16 = ((uint16_t) in[index] << 8) + ((uint16_t) in[index + 1]);
        this->crc16 = value16;
        index += 2;
        //结束字节
        this->end = in[index++];
        return 0;
    }

    int FrameAll::setToBytes(vector<uint8_t> &out) {
        uint16_t value16;
        uint32_t value32;
        //开始字节
        out.push_back(this->begin);
        //报文长度
        value16 = this->length;
        out.push_back((value16 & 0xff00) >> 8);
        out.push_back((value16 & 0x00ff));
        //协议版本
        value16 = this->version;
        out.push_back((value16 & 0xff00) >> 8);
        out.push_back((value16 & 0x00ff));
        //上位机ID
        out.push_back(this->controlCenterID);
        //信号机ID
        value32 = this->roadTrafficSignalControllerID;
        out.push_back((value32 & 0xff000000) >> 24);
        out.push_back((value32 & 0x00ff0000) >> 16);
        out.push_back((value32 & 0x0000ff00) >> 8);
        out.push_back((value32 & 0x000000ff));
        //路口ID
        out.push_back(this->roadID);
        //帧流水号
        out.push_back(this->sequence);
        //帧类型
        out.push_back(this->type);
        //数据值数量
        out.push_back(this->dataItemNum);
        //根据数据至数量，这里根据的是vector的成员个数,将数据压入
        for (auto iter:this->dataItems) {
            vector<uint8_t> outItem;
            iter.setToBytes(outItem);
            out.insert(out.end(), outItem.begin(), outItem.end());
        }
        //CRC16校验
        value16 = this->crc16;
        out.push_back((value16 & 0xff00) >> 8);
        out.push_back((value16 & 0x00ff));
        //结束字节
        out.push_back(this->end);
        return 0;
    }

    void FrameAll::UpdateFrameLength() {
        //计算长度
        vector<uint8_t> outBytes;
        this->setToBytes(outBytes);
        int len = outBytes.size();
        //去掉头尾
        this->length = (len - 2);
        this->isUpdateLength = true;
    }

    void FrameAll::UpdateFrameCRC16() {
        //crc16,不包含头尾
        vector<uint8_t> outBytes;
        this->setToBytes(outBytes);
        vector<uint8_t> inBytes;
        inBytes.assign(outBytes.begin() + 1, outBytes.end() - 3);
        uint16_t crc = Crc16TabCCITT(inBytes.data(), inBytes.size());
        this->crc16 = crc;
        this->isUpdateCRC = true;
    }

    int FrameAll::UpdateByteLength(vector<uint8_t> &bytes) {
        int needLen = 17;
        if (bytes.size() < needLen) {
//        throw std::length_error("ComFrame_GBT20999_2017::UpdateByteLength len less " + to_string(needLen));
            return -1;
        }
        uint16_t value16 = bytes.size() - 4;
        bytes.at(2) = ((value16 & 0xff00) >> 8);
        bytes.at(3) = ((value16 & 0x00ff));

        return 0;
    }

    int FrameAll::UpdateByteCRC16(vector<uint8_t> &bytes) {
        int needLen = 17;
        if (bytes.size() < needLen) {
//        throw std::length_error("ComFrame_GBT20999_2017::UpdateByteLength len less " + to_string(needLen));
            return -1;
        }
        uint8_t tf = 0x5c;
        vector<uint8_t> needSet = {0x7e, 0x7d, 0x5c};
        auto bytesWithoutTF = DeTransferMean(bytes, tf, needSet);

        vector<uint8_t> inBytes;
        inBytes.assign(bytesWithoutTF.begin() + 1, bytesWithoutTF.end() - 3);
        uint16_t value16 = Crc16TabCCITT(inBytes.data(), inBytes.size());
        bytes.at(bytes.size() - 4) = ((value16 & 0xff00) >> 8);
        bytes.at(bytes.size() - 3) = ((value16 & 0x00ff));
        return 0;
    }

    int FrameAll::getFromBytesWithTF(vector<uint8_t> in, bool isCheckCRC) {
        int needLen = 17;
        if (in.size() < needLen) {
//        throw std::length_error("ComFrame_GBT20999_2017::getFromBytes len less " + to_string(needLen));
            return -1;
        }
        //判断长度信息是否正确
        uint16_t calLen = in.size() - 4;//去掉头尾字节包括转义字符
        uint16_t len = ((uint16_t) in.at(2) << 8) + ((uint16_t) in.at(3));
        if (calLen != len) {
            printf("len error calLen:%d,len:%d\n", calLen, len);
            return -1;
        }


        //1.先去掉转义字符
        //转移字符为0x5c 要转义的字节集为 0x7e 0x7d 0x5c
        uint8_t tf = 0x5c;
        vector<uint8_t> needSet = {0x7e, 0x7d, 0x5c};
        auto bytes = DeTransferMean(in, tf, needSet);
        //2.根据通信字节流获取帧内容
        this->getFromBytes(bytes);
        if (isCheckCRC) {
            vector<uint8_t> inBytes;
            inBytes.assign(bytes.begin() + 1, bytes.end() - 3);
            uint16_t calCRC16 = Crc16TabCCITT(inBytes.data(), inBytes.size());
            //字节流内的校验值
            uint16_t crc16 = (((uint16_t) bytes.at(bytes.size() - 3)) << 8) + ((uint16_t) bytes.at(bytes.size() - 2));
            if (calCRC16 != crc16) {
                printf("crc error cal:%d,crc:%d\n", calCRC16, crc16);
                return -1;
            }
        }
        return 0;
    }

    int FrameAll::setToBytesWithTF(vector<uint8_t> &out) {
        vector<uint8_t> bytes;
        //1.根据具体的帧内容变换成字节流
        this->setToBytes(bytes);
        //2.添加转义字符
        uint8_t tf = 0x5c;
        vector<uint8_t> needSet = {0x7e, 0x7d, 0x5c};
        vector<uint8_t> bytesWithTF;
        bytesWithTF = TransferMean(bytes, tf, needSet);
        //3.根据实际情况更新长度
        if (!isUpdateLength) {
            UpdateByteLength(bytesWithTF);
        }
        //4.根据实际情况更新校验值
        if (!isUpdateCRC) {
            UpdateByteCRC16(bytesWithTF);
        }
        out.assign(bytesWithTF.begin(), bytesWithTF.end());

        return 0;
    }

    DataItem ReqGetManufacturer() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_DeviceInfo;
        dataItem.objID = DataItemObjID_Manufacturer;
        dataItem.attrID = 0;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetDeviceVersion() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_DeviceInfo;
        dataItem.objID = DataItemObjID_DeviceVersion;
        dataItem.attrID = 0;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetDeviceNo() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_DeviceInfo;
        dataItem.objID = DataItemObjID_DeviceNo;
        dataItem.attrID = 0;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetProductionDate() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_DeviceInfo;
        dataItem.objID = DataItemObjID_ProductionDate;
        dataItem.attrID = 0;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetConfigDate() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_DeviceInfo;
        dataItem.objID = DataItemObjID_ConfigDate;
        dataItem.attrID = 0;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqSetConfigDate(vector<uint8_t> data) {
        DataItem dataItem;
        dataItem.length = 4 + data.size();
        dataItem.typeID = DataItemTypeID_DeviceInfo;
        dataItem.objID = DataItemObjID_ConfigDate;
        dataItem.attrID = 0;
        dataItem.elementID = 0;
        dataItem.data.assign(data.begin(), data.end());
        return dataItem;
    }

    DataItem ReqGetRSCFixRoad() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_BaseInfo;
        dataItem.objID = DataItemObjID_RSCFixRoad;
        dataItem.attrID = 0;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetRSCNet4ConfigIP() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_BaseInfo;
        dataItem.objID = DataItemObjID_RSCNet4Config;
        dataItem.attrID = 1;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetRSCNet4ConfigMask() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_BaseInfo;
        dataItem.objID = DataItemObjID_RSCNet4Config;
        dataItem.attrID = 2;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetRSCNet4ConfigGateway() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_BaseInfo;
        dataItem.objID = DataItemObjID_RSCNet4Config;
        dataItem.attrID = 3;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetCCNet4ConfigIP() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_BaseInfo;
        dataItem.objID = DataItemObjID_CCNet4Config;
        dataItem.attrID = 1;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetCCNet4ConfigMask() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_BaseInfo;
        dataItem.objID = DataItemObjID_CCNet4Config;
        dataItem.attrID = 2;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetCCNet4ConfigGateway() {
        DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_BaseInfo;
        dataItem.objID = DataItemObjID_CCNet4Config;
        dataItem.attrID = 3;
        dataItem.elementID = 0;
        return dataItem;
    }

    DataItem ReqGetRSCTimeZone() {
        ComFrame_GBT20999_2017::DataItem dataItem;
        dataItem.length = 4;
        dataItem.typeID = DataItemTypeID_BaseInfo;
        dataItem.objID = DataItemObjID_RSCTimeZone;
        dataItem.attrID = 0;
        dataItem.elementID = 0;
        return dataItem;
    }
}