#ifndef LOG_H
#define LOG_H

#include <string>
#include <mutex>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "blockqueue.h"
#include "../Buffer/buffer.h"

class Log{
    private:
        Log();
        void AppendLogLevelTitle_(int level);
        virtual ~Log();
        void AsyncWrite_();                 //异步写日志

        static const int LOG_PATH_LEN = 256;                    //日志文件最长文件名
        static const int LOG_NAME_LEN = 256;                    //日志最长名字
        static const int MAX_LINES = 50000;                        //日志文件内的最长日志条数

        const char* path_;
        const char* suffix_;
        
        int MAX_LINES_;                                                              //最大日志行数
        int lineCount_;                                                                 //日志行数记录
        int toDay_;                                                                         //按当天日期区分文件

        bool isOpen_;

        Buffer buff_;                                                                       //缓冲区
        int level_;                                                                             //日志等级
        bool isAsync_;                                                                    //是否开启异步日志

        FILE* fp_;                                                                                                         //打开log的文件指针
        std::unique_ptr<BlockQueue<std::string>> deque_;                    //阻塞队列
        std::unique_ptr<std::thread> writeThread_;       //写线程的指针
        std::mutex mtx_;                                                                                            //同步日志必需的互斥量
    public:
        void init(int level,const char* path ="./log",
            const char* suffix=".log",
            int maxQueueCapacity = 1024
        );
        static Log* Instance();
        static void FlushLogThread();

        void write(int level,const char* format,...);
        void flush();

        int GetLevel();
        void SetLevel(int level);
        bool IsOpen(){
            return isOpen_;
        }
};

#define LOG_BASE(level,format, ...)\
    do {\
        Log* log = Log::Instance();\
        if ( log->IsOpen() && log->GetLevel() <= level){\
              log->write(level,format,##__VA_ARGS__);\
              log->flush();\ 
        }\
    } while(0);


#define LOG_DEBUG(format,...) do {LOG_BASE(0,format,##__VA_ARGS__)} while(0);
#define LOG_INFO(format,...) do {LOG_BASE(1,format,##__VA_ARGS__)} while(0);
#define LOG_WARN(format,...) do {LOG_BASE(2,format,##__VA_ARGS__)} while (0);
#define LOG_ERROR(format,...) do {LOG_BASE(3,format,##__VA_ARGS__)} while (0);

    
#endif