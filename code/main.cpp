#include <unistd.h>
#include "Server/webserver.h"

int main(){
    WebServer server(1316,3,60000,false,3306,"root","123456","webserver",12,6,true,1,1024);
    server.Start();
    return 0;
}