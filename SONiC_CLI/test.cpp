#include <iostream>
#include <stdlib.h>
#include <string>
#include "logPayload.h"
using namespace std;
int main()
{
    LogPayload Payload;
    Payload.setInstruction("hello world");
    printf("%s \n",Payload.toJsonString().data());
    return 0;
}