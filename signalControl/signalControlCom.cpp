//
// Created by lining on 12/26/22.
//

#include "signalControlCom.h"
#include <stdexcept>
#include "common/CRC.h"
#include <algorithm>

using namespace std;

//转义字符问题处理
static bool isInSet(uint8_t in, vector<uint8_t> set) {
    bool ret = false;
    for (auto iter: set) {
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
    for (int i = 1; i < out.size() - 1; i++) {
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
    for (int i = 1; i < out.size() - 1; i++) {
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

    uint16_t CRC16(uint8_t *data, int len) {
        if (data == nullptr || len <= 0) {
            return 0;
        }

        return Crc16Cal(data, len, 0x1005, 0x0000, 0x0000, 0);
    }


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

    void DataItem::GetGroup(uint8_t index, uint8_t typeID, uint8_t objID, uint8_t attrID, uint8_t elementID,
                            vector<uint8_t> *data) {
        this->index = index;
        if (data == nullptr) {
            this->length = 4;
            this->data.clear();
        } else {
            this->length = 4 + data->size();
            this->data.assign(data->begin(), data->end());
        }
        this->typeID = typeID;
        this->objID = objID;
        this->attrID = attrID;
        this->elementID = elementID;
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
        for (auto iter: this->dataItems) {
            //检测小帧数据的长度
            auto length_iter = 4 + iter.data.size();
            if (iter.length != length_iter) {
                iter.length = length_iter;
            }
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
        //去掉头尾和长度字节
        this->length = (len - 4);
        this->isUpdateLength = true;
    }

    void FrameAll::UpdateFrameCRC16() {
        //crc16,不包含头尾
        vector<uint8_t> outBytes;
        this->setToBytes(outBytes);
        vector<uint8_t> inBytes;
        inBytes.assign(outBytes.begin() + 1, outBytes.end() - 3);
        uint16_t crc = CRC16(inBytes.data(), inBytes.size());
        this->crc16 = crc;
        this->isUpdateCRC = true;
    }

    int FrameAll::UpdateByteLength(vector<uint8_t> &bytes) {
        int needLen = 17;
        if (bytes.size() < needLen) {
//        throw std::length_error("ComFrame_GBT20999_2017::UpdateByteLength len less " + to_string(needLen));
            return -1;
        }
        //去掉头尾和长度字节
        uint16_t value16 = bytes.size() - 4;
        bytes.at(1) = ((value16 & 0xff00) >> 8);
        bytes.at(2) = ((value16 & 0x00ff));

        return 0;
    }

    int FrameAll::UpdateByteCRC16(vector<uint8_t> &bytes) {
        int needLen = 17;
        if (bytes.size() < needLen) {
//        throw std::length_error("ComFrame_GBT20999_2017::UpdateByteLength len less " + to_string(needLen));
            return -1;
        }
//        uint8_t tf = 0x5c;
//        vector<uint8_t> needSet = {0x7e, 0x7d, 0x5c};
//        auto bytesWithoutTF = DeTransferMean(bytes, tf, needSet);

//        vector<uint8_t> inBytes;
//        inBytes.assign(bytesWithoutTF.begin() + 1, bytesWithoutTF.end() - 3);
        vector<uint8_t> inBytes;
        inBytes.assign(bytes.begin() + 1, bytes.end() - 3);
        uint16_t value16 = CRC16(inBytes.data(), inBytes.size());
        bytes.at(bytes.size() - 3) = ((value16 & 0xff00) >> 8);
        bytes.at(bytes.size() - 2) = ((value16 & 0x00ff));
        return 0;
    }

    int FrameAll::getFromBytesWithTF(vector<uint8_t> in, bool isCheckCRC) {
        int needLen = 17;
        if (in.size() < needLen) {
//        throw std::length_error("ComFrame_GBT20999_2017::getFromBytes len less " + to_string(needLen));
            return -1;
        }
        //判断长度信息是否正确
        uint16_t calLen = in.size() - 4;//去掉头尾字节+长度字节
        uint16_t len = ((uint16_t) in.at(1) << 8) + ((uint16_t) in.at(2));
        if (calLen != len) {
            printf("len error calLen:%d,len:%d\n", calLen, len);
            return -1;
        }


        //1.先去掉转义字符
        //转移字符为0x5c 要转义的字节集为 0x7e 0x7d 0x5c
        vector<uint8_t> bytes;
        bytes.clear();
        if (this->comType != ComType_UDP) {
            uint8_t tf = 0x5c;
            vector<uint8_t> needSet = {0x7e, 0x7d, 0x5c};
            bytes = DeTransferMean(in, tf, needSet);
        } else {
            bytes.assign(in.begin(), in.end());
        }
        //2.根据通信字节流获取帧内容
        this->getFromBytes(bytes);
        if (isCheckCRC) {
            vector<uint8_t> inBytes;
            inBytes.assign(bytes.begin() + 1, bytes.end() - 3);
            uint16_t calCRC16 = CRC16(inBytes.data(), inBytes.size());
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
        vector<uint8_t> bytes_plain;
        bytes_plain.clear();
        if (this->comType != ComType_UDP) {
            uint8_t tf = 0x5c;
            vector<uint8_t> needSet = {0x7e, 0x7d, 0x5c};
            bytes_plain = TransferMean(bytes, tf, needSet);
        } else {
            bytes_plain.assign(bytes.begin(), bytes.end());
        }
        //3.根据实际情况更新长度
        if (!isUpdateLength) {
            UpdateByteLength(bytes_plain);
        }
        //4.根据实际情况更新校验值
        if (!isUpdateCRC) {
            UpdateByteCRC16(bytes_plain);
        }
        out.assign(bytes_plain.begin(), bytes_plain.end());

        return 0;
    }

    int ReqGet(DataItem &dataItem, uint8_t index, DataItemType type) {
        int ret = 0;
        auto iter = DataItemMap.find(type);
        if (iter != DataItemMap.end()) {
            uint32_t value = iter->second.value;
            uint8_t typeID = (value & 0xff000000) >> 24;
            uint8_t objID = (value & 0x00ff0000) >> 16;
            uint8_t attrID = (value & 0x0000ff00) >> 8;
            uint8_t elementID = (value & 0x000000ff);

            dataItem.GetGroup(index, typeID, objID, attrID, elementID);
        } else {
            ret = -1;
        }
        return ret;
    }

    int ReqSet(DataItem &dataItem, uint8_t index, DataItemType type, vector<uint8_t> data) {
        int ret = 0;
        auto iter = DataItemMap.find(type);
        if (iter != DataItemMap.end()) {
            uint32_t value = iter->second.value;
            uint8_t typeID = (value & 0xff000000) >> 24;
            uint8_t objID = (value & 0x00ff0000) >> 16;
            uint8_t attrID = (value & 0x0000ff00) >> 8;
            uint8_t elementID = (value & 0x000000ff);

            dataItem.GetGroup(index, typeID, objID, attrID, elementID, &data);
        } else {
            ret = -1;
        }
        return ret;
    }

    int RspGetType(DataItem dataItem, DataItem::TypeID &typeID, DataItemType &type) {
        if (dataItem.typeID > DataItem::TypeID_Len) {
            return -1;
        }

        typeID = (DataItem::TypeID) dataItem.typeID;

        uint32_t value = ((uint32_t) dataItem.typeID << 24) + ((uint32_t) dataItem.objID << 16) +
                         ((uint32_t) dataItem.attrID << 8) + ((uint32_t) dataItem.elementID);

        auto iter = std::find_if(DataItemMap.begin(), DataItemMap.end(),
                                 [value](const map<DataItemType, DataItemValue>::value_type &item) {
                                     return item.second.value == value;
                                 });

        if (iter != DataItemMap.end()) {
            type = iter->first;
        } else {
            return -1;
        }

        return 0;
    }


}