#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

class Epoller{
    private:
        int epollFd_;
        std::vector<struct epoll_event> events_;
    public:
        explicit Epoller(int maxEvent = 1024);
        ~Epoller();

        bool AddFd(int fd,uint32_t events_);
        bool ModFd(int fd,uint32_t events_);
        bool DelFd(int fd);
        int Wait(int timeoutMs = -1);
        int GetEventFd(size_t i) const;
        uint32_t GetEvents(size_t i) const;
};

#endif