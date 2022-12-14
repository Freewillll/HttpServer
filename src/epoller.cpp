#include "epoller.h"

Epoller::Epoller(int maxevents)
    : m_epollFd(epoll_create(1)), m_epEves(maxevents) {}

int Epoller::wait(int timeout)
{
    int nums = epoll_wait(m_epollFd, &m_epEves[0], static_cast<int>(m_epEves.size()), timeout);    //  wait fd event
    return nums;    // the nums of event
}
bool Epoller::add(int fd, uint32_t ev)    // add
{
    if (fd < 0)
    {
        return false;
    }
    struct epoll_event event;
    event.events = ev;
    event.data.fd = fd;
    epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event);
    return true;
}
bool Epoller::mod(int fd, uint32_t ev)    //  update
{
    if (fd < 0)
    {
        return false;
    }
    struct epoll_event event;
    event.events = ev;
    event.data.fd = fd;
    epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &event);
    return true;
}
bool Epoller::del(int fd)    //   del
{
    if (fd < 0)
    {
        return false;
    }
    epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, NULL);
    return true;
}
//对外要使用的接口
int Epoller::getSockFd(int i) const
{
    if (i < 0 || i > static_cast<int>(m_epEves.size()))
    {
        return false;
    }
    return m_epEves[i].data.fd;
}
uint32_t Epoller::getFdEvent(int i) const
{
    if (i < 0 || i > static_cast<int>(m_epEves.size()))
    {
        return false;
    }
    return m_epEves[i].events;
}