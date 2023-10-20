#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../Log/log.h"

class SqlConnPool{
    private:
        SqlConnPool() = default;
        ~SqlConnPool(){ClosePool();}

        int MAX_CONN_;

        std::queue<MYSQL *> connQue_;
        std::mutex mtx_;
        sem_t semId_;
        
    public:
        void ClosePool();
        static SqlConnPool* Instance();

        MYSQL* GetConn();
        void FreeConn(MYSQL* conn);
        int GetFreeConnCount();

        void Init(const char* host,int port,
                        const char* user,const char* pwd,
                        const char* dbName,int connSize);

};


#endif