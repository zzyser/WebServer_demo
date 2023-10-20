#include "log.h"

using namespace std;

Log::Log(){
    fp_=nullptr;
    deque_=nullptr;
    writeThread_=nullptr;
    lineCount_=0;
    toDay_=0;
    isAsync_=false;
}

Log::~Log(){
    while(!deque_->empty()){
        deque_->flush();                //唤醒消费者，处理剩下的任务
    }
    deque_->Close();                    //关闭队列
    writeThread_->join();           //等待当前线程完成任务
    if(fp_){
        lock_guard<mutex> locker(mtx_);
        flush();                        //清空缓冲区中的数据
        fclose(fp_);                //关闭日志文件
    }
}

//唤醒阻塞队列消费者，开始写日志
void Log::flush(){
    if(isAsync_){           //只有异步日志才会用到deque
        deque_->flush();
    }
    fflush(fp_);            //清空输入缓冲区
}

//懒汉模式，局部静态变量法，不需要加锁和解锁操作
Log* Log::Instance(){
    static Log log;
    return& log;
}

//写线程的执行函数
void Log::AsyncWrite_(){
    string str="";
    while (deque_->pop(str))
    {
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(),fp_);
    }
    
}

//初始化日志实例
void Log::init(int level,const char* path,const char* suffix,int maxQueCapacity){
    isOpen_=true;
    level_=level;
    path_=path;
    suffix_=suffix;
    if(maxQueCapacity){         //异步
        isAsync_=true;
        if(!deque_){            //若为空则创建一个
            unique_ptr<BlockQueue<string>> newQue(new BlockQueue<string>);
            //因为unique_ptr不支持普通的拷贝或赋值操作，所以采用move（）
            //将动态申请的内存交给deque，newQue被释放
            deque_=move(newQue);

            unique_ptr<thread> newThread(new thread(FlushLogThread));
            writeThread_=move(newThread);
        }
    }else{
        isAsync_=false;
    }

    lineCount_=0;
    time_t timer=time(nullptr);
    struct tm* systime = localtime(&timer);
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName,LOG_NAME_LEN-1,"%s/%04d_%02d%s",
    path_,systime->tm_year+1900,systime->tm_mon+1,systime->tm_mday,suffix_);
    toDay_=systime->tm_mday;
    //打开文件
    {
        lock_guard<mutex> locker(mtx_);
        buff_.RetrieveAll();
        if(fp_){
            flush();
            fclose(fp_);
        }
        fp_=fopen(fileName,"a");
        if(fp_==nullptr){
            mkdir(path_,0777);
            fp_=fopen(fileName,"a");
        }
        assert(fp_!=nullptr);
    }
}

void Log::write(int level,const char* format,...){
    struct timeval now = {0,0};
    gettimeofday(&now,nullptr);
    time_t tSec = now.tv_sec;
    struct tm* sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    //日志日期  日志行数
    if(toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))){
        unique_lock<mutex> locker(mtx_);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36]={0};
        snprintf(tail,36,"%04d_%02d_%02d",t.tm_year+1900,t.tm_mon+1,t.tm_mday);

        if(toDay_!=t.tm_mday){
            snprintf(newFile,LOG_NAME_LEN-72,"%s/%s%s",path_,tail,suffix_);
            toDay_=t.tm_mday;
            lineCount_=0;
        }else{
            snprintf(newFile,LOG_NAME_LEN-72,"%s/%s-%d%s",path_,tail,(lineCount_ / MAX_LINES),suffix_);  
        }

        locker.lock();
        flush();
        fclose(fp_);
        fp_=fopen(newFile,"a");
        assert(fp_!=nullptr);
    }

    //在buffer中生成一条对应的日志信息
    {
        unique_lock<mutex> locker(mtx_);
        lineCount_++;
        int n= snprintf(buff_.BeginWrite(),128,"%d-%02d-%02d %02d:%02d:%02d.%06ld",
                t.tm_year+1900,t.tm_mon+1,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec,now.tv_usec);
        buff_.HasWritten(n);
        AppendLogLevelTitle_(level);

        va_start(vaList,format);
        int m = vsnprintf(buff_.BeginWrite(),buff_.WritableBytes(),format,vaList);
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.Append("\n\0",2);

        if(isAsync_ && deque_ && !deque_->full()){
            deque_->push_back(buff_.RetrieveAllToStr());
        }else{
            fputs(buff_.Peek(),fp_);
        }
        buff_.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle_(int level){
    switch (level)
    {
    case 0:
        buff_.Append("[debug]: ",9);
        break;
    case 1:
        buff_.Append("[info]: ",9);
        break;
    case 2:
        buff_.Append("[warn]: ",9);
        break;
    case 3:
        buff_.Append("[error]: ",9);
        break;
    default:
        buff_.Append("[info]: ",9);
        break;
    }
}

int Log::GetLevel(){
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level){
    lock_guard<mutex> locker(mtx_);
    level_=level;
}

void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}