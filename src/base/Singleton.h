#ifndef _SRC_BASE_SINGLETON_H_
#define _SRC_BASE_SINGLETON_H_

#include <cstddef>
namespace tinyRPC{

template<class T>
class Singleton
{
public:
    //返回单件句柄
    static inline T* Instance()
    {
        T*& ptr = GetInstancePtr();

        if (ptr == NULL)
        {
            ptr = new T;
        }

        return const_cast<T*>(ptr);
    }

    static bool HasInstance()
    {
        return NULL != GetInstancePtr();
    }

    static void DeleteInstnace()
    {
        if (NULL == GetInstancePtr())
        {
            return;
        }
        delete GetInstancePtr();
        GetInstancePtr() = NULL;
    }

private:
    static inline T*& GetInstancePtr()
    {
        //局部静态变量
        static T* ptr = 0;
        return ptr;
    }
};

} // namespace tinyRPC

#endif