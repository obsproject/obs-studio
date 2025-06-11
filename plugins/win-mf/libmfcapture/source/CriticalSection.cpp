class CriticalSection {
	CRITICAL_SECTION mutex;

public:
	inline CriticalSection() { InitializeCriticalSection(&mutex); }
	inline ~CriticalSection() { DeleteCriticalSection(&mutex); }

	inline operator CRITICAL_SECTION *() { return &mutex; }
};

class CriticalScope {
	CriticalSection &mutex;

	CriticalScope() = delete;
	CriticalScope &operator=(CriticalScope &cs) = delete;

public:
	inline CriticalScope(CriticalSection &mutex_) : mutex(mutex_) { EnterCriticalSection(mutex); }

	inline ~CriticalScope() { LeaveCriticalSection(mutex); }
};
