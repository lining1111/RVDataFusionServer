//
// Created by lining on 2023/11/30.
//

#include <gflags/gflags.h>
#include <Poco/AutoPtr.h>
#include <Poco/DOM/Document.h> //for Poco::XML::Document
#include <Poco/DOM/Element.h>  //for Poco::XML::Element
#include <Poco/DOM/Text.h>       //for Poco::XML::Text
#include <Poco/DOM/CDATASection.h>    //for Poco::XML::CDATASection
#include <Poco/DOM/ProcessingInstruction.h> //for Poco::XML::ProcessingInstruction
#include <Poco/DOM/Comment.h>  //for Poco::XML::Comment
#include <Poco/DOM/DOMWriter.h> //for Poco::XML::DOMWriter
#include <Poco/DOM/DOMParser.h>
#include <Poco/XML/XMLWriter.h> //for Poco::XML::XMLWriter
#include <sstream>

DEFINE_string(input, "bin/config.xml", "input file");

int main(int argc, char ** argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    //1.打开XML文件
    Poco::XML::DOMParser parser;
    parser.setFeature(Poco::XML::DOMParser::FEATURE_FILTER_WHITESPACE, true);
    Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(FLAGS_input);//解析xml文件
    Poco::AutoPtr<Poco::XML::ProcessingInstruction> pi = pDoc->createProcessingInstruction("xml","version='1.0' encoding='UTF-8'" );
    pDoc->appendChild(pi);
    //2.添加节点
    //通过Path获取元素
    Poco::XML::Element* pNode = (Poco::XML::Element*)pDoc->getNodeByPath("CTPT");
    //app
    Poco::XML::Element* node_app = pDoc->createElement("app");
    node_app->setAttribute("parkingAreaType", "0");
    //app_name
    Poco::XML::Text* node_app_name = pDoc->createTextNode("name");;
    node_app_name->setNodeValue("RVDataFusionServer");
    node_app->appendChild(node_app_name);
    //app_path
    Poco::XML::Text* node_app_path = pDoc->createTextNode("path");
    node_app_path->setNodeValue("/home/nvidianx/");
    node_app->appendChild(node_app_path);
    //app_command
    Poco::XML::Text* node_app_command = pDoc->createTextNode("command");;
    node_app_command->setNodeValue("./start.sh> /dev/null 2>&amp;1 &amp;");
    node_app->appendChild(node_app_command);
    //添加到CTPT
    pNode->appendChild(node_app);
    //3.保存文件
    Poco::XML::DOMWriter writer;
    writer.setOptions(Poco::XML::XMLWriter::PRETTY_PRINT);// PRETTY_PRINT = 4
    writer.writeNode(FLAGS_input, pDoc);//直接写进文件
}