// 封装一个【线程池】类
#ifndef __THREADPOOLV1_H__
#define __THREADPOOLV1_H__

// 导入头文件
#include <queue>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
// 类内声明，类外定义
template <typename T>
class ThreadPool
{
public:
    // 构造函数：初始化线程池中线程的数量，工作队列的长度
    ThreadPool(unsigned int t_nums = 8, unsigned int j_nums = 200);

    // 析构函数
    ~ThreadPool();

    // 成员函数：往线程池中【添加】任务事件
    bool append(T *newjob);

private:
    // 核心线程的数量
    unsigned int m_thread_num;
    // 最大线程的数量
    unsigned int m_thread_max;
    // 任务队列长度
    unsigned int m_queue_len;
    // 工作队列（存放线程的）
    pthread_t *m_work_threads; // 用动态数组实现
    // 任务队列（存放线程需要执行的任务《函数》的）
    std::queue<T *> m_job_queue;
    // 用queue，list，数组实现都行
    // 线程的回调函数
    static void *worker(void *arg);
    void m_run();          // worker的封装函数
    void m_add(T *newjob); // append的封装函数
    // 线程同步机制
    sem_t sem_job_src;   // 任务队列资源的个数
    sem_t sem_job_empty; //任务队列空闲位置的个数
    sem_t sem_mutex;     // 互斥锁信号量
    // 设置一个标志位，表示整个线程池是否在进行中
    bool isRun;
};

// 构造函数
template <typename T>
ThreadPool<T>::ThreadPool(unsigned int t_nums, unsigned int j_nums)
{
    // 初始化线程数量等成员变量
    // 判断输入数据的合法性
    if (t_nums <= 0 || j_nums <= 0)
    {
        printf("参数传入错误\n");
        throw std::exception();
    }
    m_thread_num = t_nums;
    m_queue_len = j_nums;
    // 初始化信号量，表示job队列资源的信号量不变，互斥锁的信号量初始化为1
    sem_init(&sem_job_src, 0, 0);    //   pshared:   0  share between threads  !0: share between progress
    sem_init(&sem_job_empty, 0, m_queue_len);
    sem_init(&sem_mutex, 0, 1);
    // 初始化线程池状态
    isRun = true;
    // 申请堆区内存，存放子线程的线程号
    m_work_threads = new pthread_t[m_thread_num];
    if (!m_work_threads)
    {
        // 如果开辟堆区动态内存失败，抛出异常
        isRun = false;
        printf("堆区开辟内存失败\n");
        throw std::exception();
    }
    // 创建 m_thread_num 个子线程
    for (int i = 0; i < m_thread_num; ++i)
    {
        int ret;
        ret = pthread_create(m_work_threads + i, NULL, worker, this);
        if (ret != 0)
        {
            // 创建线程出现异常，终止整个程序，清除资源
            delete[] m_work_threads;
            isRun = false;
            printf("创建线程失败\n");
            throw std::exception();
        }
        else
        {
            // printf("创建第%d个线程...\n",i + 1);
        }
    }
    // 线程创建后，设置线程分离
    for (int i = 0; i < m_thread_num; ++i)
    {
        int ret;
        ret = pthread_detach(m_work_threads[i]);
        if (ret != 0)
        {
            // 创建线程出现异常，终止整个程序，清除资源
            delete[] m_work_threads;
            isRun = false;
            printf("线程分离失败\n");
            throw std::exception();
        }
    }
}
// 析构函数
template <typename T>
ThreadPool<T>::~ThreadPool()
{
    // 销毁指针，释放堆区内存等
    delete[] m_work_threads;
    isRun = false;
}
// public成员函数：append
template <typename T>
bool ThreadPool<T>::append(T *newjob)
{
    // 往内存池中，添加一个工作事件，事件应该被添加到任务队列中
    // 在主线程中，向任务队列中写入数据，必须要加锁
    // 上互斥锁
    // 用信号量判断job队列是否还有空间
    m_add(newjob);
    return true;
}
template <typename T>
void ThreadPool<T>::m_add(T *newjob)
{
    //sem_wait(&sem_job_empty); // 工作队列满了，阻塞在此
    sem_wait(&sem_mutex);
    if(m_job_queue.size() > m_queue_len)
    {
        sem_post(&sem_mutex);
        return;
    }
    m_job_queue.push(newjob);
    sem_post(&sem_mutex);   // 解锁
    sem_post(&sem_job_src); // job资源的信号量加1
}
// private成员函数：m_run
template <typename T>
void *ThreadPool<T>::worker(void *arg)
{
    // 内存池中的线程，需要执行的任务，任务从任务队列中取
    // 从任务队列中取出一个任务，从工作队列（线程s）中取出一个线程，让线程去执行这个任务（函数）
    // 任务的形态应该是什么：
    // 《函数》
    // 其实本质上，仍旧是生产者和消费者模型
    // printf("执行工作任务\n");
    ThreadPool *pool = (ThreadPool *)arg;
    pool->m_run();
    return NULL;
}
template <typename T>
void ThreadPool<T>::m_run()
{
    while (isRun)
    {
        // printf("线程%ld正在尝试获取任务\n",pthread_self());
        // 消耗一个资源，如果任务队列没有资源，阻塞等待(就相当于是让线程睡眠了)
        sem_wait(&sem_job_src);
        sem_wait(&sem_mutex); // 互斥锁

        // 取出一个任务
        // printf("线程%ld成功获取任务\n",pthread_self());
        T *Newjob = m_job_queue.front();
        m_job_queue.pop();

        // 退出，解锁
        sem_post(&sem_mutex);
        sem_post(&sem_job_empty);

        // 拿到job后，在锁外执行job内具体的函数
        // printf("线程成功获取任务\n");
        // 拿到任务后，判断一下任务是否有效：指针指向不为空
        if (Newjob == nullptr)
        {
            printf("invalid NewJob\n");
            continue;
        }
        // printf("线程%ld正在执行任务\n", pthread_self());
        Newjob->handleHTTPConn();
    }
}
#endif