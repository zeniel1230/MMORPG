#pragma once
#include <queue>
#include "../SpinLock.h"

template <class T>
class SharedQueue
{
private:
	SpinLock					m_spinLock;

	std::queue<T>				m_itemQueue;
public:
	SharedQueue() {}
	~SharedQueue() {
		while (!m_itemQueue.empty())
		{
			m_itemQueue.pop();
		}
	}

public:
	void			AddItem(T _item);
	T&				GetItem(void);

	bool			IsEmpty(void);

	int				GetSize(void) { return m_itemQueue.size(); }
};

template<class T>
inline void SharedQueue<T>::AddItem(T _item)
{
	m_spinLock.Enter();

	m_itemQueue.push(_item);

	m_spinLock.Leave();
}

template<class T>
inline T& SharedQueue<T>::GetItem(void)
{
	m_spinLock.Enter();

	T& item = m_itemQueue.front();
	m_itemQueue.pop();

	m_spinLock.Leave();

	return item;
}

template<class T>
inline bool SharedQueue<T>::IsEmpty(void)
{
	return m_itemQueue.empty();
}