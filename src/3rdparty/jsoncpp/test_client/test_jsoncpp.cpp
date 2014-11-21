#include <stdio.h>
#include <iostream>
#include "json/json.h"

void Serialize();

int main(int cargc, char** argv)
{
    std::string strValue = "{\"key1\":\"pecywang\",\"array\":[{\"key2\":\"ethanhu\"},"
        "{\"key2\":\"jimhuang\"},{\"key2\":\"shaohuali\"}]}"; 

    Json::Reader reader; 
    Json::Value value; 

    if (!reader.parse(strValue, value)) {
        std::cout << "Failed parse json string!" << std::endl;
        return 1;
    }

    std::string out = value["key1"].asString();
    std::cout << out << std::endl;
    const Json::Value arrayObj = value["array"];
    for (unsigned int i = 0; i < arrayObj.size(); ++i) {
        out = arrayObj[i]["key2"].asString();
        std::cout << out;
        if (i != arrayObj.size() - 1) {
            std::cout << std::endl;
        }
    }

    std::cout << std::endl;

    Serialize();

    return 0;
}

// 序列化Json对象
void Serialize()
{
     Json::Value root;
     Json::Value arrayObj; 
     Json::Value item; 
  
     for (int i = 0; i < 10; i ++) { 
      item["key"] = i; 
      arrayObj.append(item); 
     } 

     root["key1"] = "value1";
     root["key2"] = "value2";
     root["array"] = arrayObj;
     //root.toStyledString();

     std::string out = root.toStyledString();
     std::cout << out << std::endl;
}
