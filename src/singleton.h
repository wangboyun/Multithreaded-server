/*
 * @Description: 单例模式封装
 * @Version: 0.1
 * @Autor: Wyz
 * @Date: 2021-09-23 20:19:04
 */
#ifndef __WYZ__SINGLETON__H__
#define __WYZ__SINGLETON__H__

#include <memory>

namespace wyz {

template<class T>
class Singleton{
	
public:
	static T * GetInstance()
	{
		static T  m_singel;
		return &m_singel;
	}
};

template<class T>
class Singletonptr{
public:
	static std::shared_ptr<T> GetInstance()
	{
		static std::shared_ptr<T> m_singel;
		m_singel = std::shared_ptr<T>(new T);	
		return m_singel;
	}
};

}

#endif