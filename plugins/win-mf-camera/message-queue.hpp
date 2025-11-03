#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <atomic>

#include <util/threading.h>

class MessageQueue;
class MessageImpl;

class MessageImpl {
public:
	virtual ~MessageImpl(void) {}
	typedef std::shared_ptr<MessageImpl> MessageImplPtr;
	template<class F> static MessageImplPtr createMessage(F f);
	template<class F, class R> static MessageImplPtr createMessage(F f, R r);
	virtual void onComplete() = 0;
	void setSyncMessage(bool waitUntilDone)
	{
		m_syncMessage = waitUntilDone;
		if (m_syncMessage) {
			m_syncSignal = std::shared_ptr<std::condition_variable>(new std::condition_variable());
		}
	}
	bool isSyncMessage() { return m_syncMessage; }
	void syncWaitOpt(std::unique_lock<std::mutex> &synclk) { m_syncSignal->wait(synclk); }
	void syncNotifyOpt() { m_syncSignal->notify_one(); }

private:
	std::shared_ptr<std::condition_variable> m_syncSignal;
	bool m_syncMessage = false;
};

typedef MessageImpl::MessageImplPtr MessageImplPtr;

struct ArgPlaceHolder {};

template<class R> class MessageBase : public MessageImpl {
public:
	MessageBase(R r) : m_r(r) {}
	template<class F> void call(F &f) { f(m_r); }
	R m_r;
};

template<class T, class R, typename B = MessageBase<R>> class MessageT : public B {
public:
	MessageT(T op, R r) : m_op(op), B(r) {}

	virtual void onComplete() { B::call(m_op); }
	T m_op;
};

template<class F> inline MessageImplPtr MessageImpl::createMessage(F f)
{
	return MessageImplPtr(new MessageT<F, ArgPlaceHolder>(f, ArgPlaceHolder()));
}

template<class F, class R> inline MessageImplPtr MessageImpl::createMessage(F f, R r)
{
	return MessageImplPtr(new MessageT<F, R>(f, r));
}

class MessageQueue : public std::enable_shared_from_this<MessageQueue> {
public:
	MessageQueue(const char *szName, int priority) : m_strName(szName), m_thread(NULL), m_priority(priority) {}
	virtual ~MessageQueue(void)
	{
		if (m_thread) {
			delete m_thread;
		}
	}
	virtual int Invoke(const MessageImplPtr &pMessage, long long delay_us = 0) = 0;
	virtual int Invoke(MessageImplPtr &&pMessage, long long delay_us = 0) = 0;
	virtual void Close() = 0;
	virtual int size() const = 0;
	std::thread::id getThreadId() { return m_ownerThead; }
	virtual bool isCurrentThread() { return m_ownerThead == std::this_thread::get_id(); }

protected:
	virtual void WorkRoutine()
	{
		m_ownerThead = std::this_thread::get_id();
		os_set_thread_name(m_strName.c_str());
		WorkEnter();
		WorkRun();
		WorkLeave();
	}
	virtual void WorkEnter() { CoInitializeEx(NULL, COINIT_MULTITHREADED); }
	virtual void WorkRun() {

	};
	virtual void WorkLeave() { CoUninitialize(); };
	std::thread *m_thread;
	std::string m_strName;
	std::thread::id m_ownerThead;
	int m_priority;
};

typedef std::shared_ptr<MessageQueue> MessageQueuePtr;
typedef std::weak_ptr<MessageQueue> MessageQueueWPtr;

class NormalMessageQueue : public MessageQueue {
public:
	NormalMessageQueue(const char *szName, int priority = 0) : MessageQueue(szName, priority)
	{
		m_thread = new std::thread(&NormalMessageQueue::WorkRoutine, this);
		m_waiting = false;
	}

	virtual ~NormalMessageQueue(void) {}
	virtual int Invoke(const MessageImplPtr &pMessage, long long delay_us = 0)
	{
		(void)delay_us;
		if (pMessage) {
			std::unique_lock<std::mutex> lk(m_lock);
			if (!m_thread)
				return -1;

			m_message.push(pMessage);
			m_signal.notify_one();
			if (pMessage->isSyncMessage()) {
				pMessage->syncWaitOpt(lk);
			}
		}
		return 0;
	}
	virtual int Invoke(MessageImplPtr &&pMessage, long long delay_us = 0) override
	{
		(void)delay_us;
		if (pMessage) {
			std::unique_lock<std::mutex> lk(m_lock);
			if (!m_thread)
				return -1;
			if (pMessage->isSyncMessage()) {
				m_message.push(pMessage);
				pMessage->syncWaitOpt(lk);
			} else {
				m_message.push(std::move(pMessage));
				m_signal.notify_one();
			}
		}
		return 0;
	}
	virtual void Close()
	{
		std::thread *t = NULL;
		do {
			std::unique_lock<std::mutex> lk(m_lock);
			t = m_thread;
			m_thread = NULL;
			m_message.push(MessageImplPtr());
			m_signal.notify_one();
		} while (0);
		if (t) {
			if (t->joinable())
				t->join();
			delete t;
		}
	}
	virtual int size() const { return (int)m_message.size(); }

protected:
	virtual void WorkRun()
	{
		while (true) {
			MessageImplPtr pMessage = pull();
			if (!pMessage)
				break;
			pMessage->onComplete();
			if (pMessage->isSyncMessage()) {
				std::unique_lock<std::mutex> lk(m_lock);
				pMessage->syncNotifyOpt();
			}
		}
	}
	MessageImplPtr pull()
	{
		std::unique_lock<std::mutex> lk(m_lock);
		while (m_message.empty()) {
			m_signal.wait(lk);
		}
		MessageImplPtr msg = m_message.front();
		m_message.pop();
		return msg;
	}
	std::queue<MessageImplPtr> m_message;
	std::condition_variable m_signal;
	std::mutex m_lock;
	bool m_waiting;
};
