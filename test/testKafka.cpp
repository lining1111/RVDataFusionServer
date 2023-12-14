//
// Created by lining on 2023/12/14.
//
#include <gflags/gflags.h>
#include "kafka/KafkaProducer.h"
#include "kafka/KafkaCom.h"

DEFINE_string(topic, "cross_phaseStatus", "topic");
int main(int argc, char ** argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    string brokers = "192.168.150.129:9092";

    KafkaProducer *kafkaProducer = new KafkaProducer(brokers, FLAGS_topic);

    TabStageWithHeader tabStageWithHeader;
    tabStageWithHeader.frameHeader.NoTab = 4;
    tabStageWithHeader.frameHeader.NumByte = sizeof(TabStage);
    tabStageWithHeader.tabStage.NoJunc = 137;

    auto const ptr = reinterpret_cast<unsigned char*>(&tabStageWithHeader);

    std::vector<char> myVector( ptr, ptr + sizeof(tabStageWithHeader));
    kafkaProducer->_notify.assign(myVector.begin(), myVector.end());

    if (kafkaProducer->init() != 0) {
        LOG(ERROR) << "kafka生产初始化失败";
        delete kafkaProducer;
    } else {
        kafkaProducer->startBusiness();
    }
}