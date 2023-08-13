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
    string log;
    string instruction;
public:
    void setUser(string user)
    {
        this->user = user;
    }
    void setPid(int pid)
    {
        this->pid = pid;
    }
    void setTimeStamp(string timestamp)
    {
        this->timestamp = timestamp;
    }
    void setType(int type)
    {
        this->type = type;
    }
    void setLog(string log)
    {
        this->log = log;
    }
    void setInstruction(string instruction)
    {
        this->instruction = instruction;
    }

    string getUser()
    {
        return this->user;
    }
    int getPid()
    {
        return this->pid;
    }

    string getTimeStamp()
    {
        return this->timestamp;
    }
    int getType()
    {
        return this->type;
    }
    string getLog()
    {
        return this->log;
    }
    string getInstruction()
    {
        return this->instruction;
    }

    string toJsonString()
    {
        Value root;
        root["pid"] = pid;
        root["TimeStamp"] = "11:11";
        root["user"] = "zhangsan";
        root["type"] = 1;//1 == control
        root["instruction"] = "exit";

        FastWriter writer;
        string json_file = writer.write(root);

        return json_file;
    }

    void parseJsonToClass()
    {
        
    }
};
int main()
{
    LogPayload* lp = new LogPayload();
    cout << lp->toJsonString();
    return 0;
}