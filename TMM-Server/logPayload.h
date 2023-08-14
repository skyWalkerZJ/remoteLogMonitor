#include <string>
#include <unistd.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/writer.h>
#include <iostream>
using namespace std;
using namespace Json;
class LogPayload{
private:
    string user;
    int pid;
    string timestamp;
    int type;
    string data;
    string instruction;
public:
    LogPayload()
    {
        this->pid = getpid();
        this->user = "root";
        this->timestamp = "11:11:11";
    }
    void setUser(string user)
    {
        this->user = user;
    }
    void setTimeStamp(string timestamp)
    {
        this->timestamp = timestamp;
    }
    void setType(int type)
    {
        this->type = type;
    }
    void setLog(string data)
    {
        this->data = data;
    }
    void setInstruction(string instruction)
    {
        this->instruction = instruction;
    }
    void setData(string data)
    {
        this->data = data;
    }
    string getUser()
    {
        return this->user;
    }
    int getPid()
    {
        return getpid();
    }

    string getTimeStamp()
    {
        return this->timestamp;
    }
    int getType()
    {
        return this->type;
    }
    string getData()
    {
        return this->data;
    }
    string getInstruction()
    {
        return this->instruction;
    }

    string toJsonString()
    {
        Value root;
        root["pid"] = getpid();
        root["TimeStamp"] = "11:11";
        root["user"] = this->user;
        root["type"] = this->type;//1 == control
        root["instruction"] = this->instruction;
        root["data"] = this->data;
        FastWriter writer;
        string json_file = writer.write(root);
        return json_file;
    }

    LogPayload* parseJsonToClass(string srcJson)
    {
        Reader reader;
        Value root;
        reader.parse(srcJson,root);
        this->user = root["user"].asString();
        this->type = root["type"].asInt();
        this->timestamp = root["timestamp"].asString();         
        this->instruction = root["instruction"].asString();
        this->data = root["data"].asString();
        return this;
    }
};
