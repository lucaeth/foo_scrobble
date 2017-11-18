#ifndef _PFC_PROFILER_H_
#define _PFC_PROFILER_H_

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))

#define PFC_HAVE_PROFILER

#include <intrin.h>
namespace pfc {
	class profiler_static {
	public:
		profiler_static(const char * p_name);
		~profiler_static();
		void add_time(t_int64 delta) { total_time += delta;num_called++; }
	private:
		const char * name;
		t_uint64 total_time, num_called;
	};

	class profiler_local {
	public:
		profiler_local(profiler_static * p_owner) {
			owner = p_owner;
			start = __rdtsc();
		}
		~profiler_local() {
			t_int64 end = __rdtsc();
			owner->add_time(end - start);
		}
	private:
		t_int64 start;
		profiler_static * owner;
	};

}
#define profiler(name) \
	static pfc::profiler_static profiler_static_##name(#name); \
	pfc::profiler_local profiler_local_##name(&profiler_static_##name);
	

#endif

#ifdef _WIN32

namespace pfc {
#if _WIN32_WINNT >= 0x600
typedef uint64_t tickcount_t;
inline tickcount_t getTickCount() { return GetTickCount64(); }
#else
#define PFC_TICKCOUNT_32BIT
typedef uint32_t tickcount_t;
inline tickcount_t getTickCount() { return GetTickCount(); }
#endif

class hires_timer {
public:
    hires_timer() : m_start() {}
	void start() {
		m_start = g_query();
	}
	double query() const {
		return _query( g_query() );
	}
	double query_reset() {
		t_uint64 current = g_query();
		double ret = _query(current);
		m_start = current;
		return ret;
	}
	pfc::string8 queryString(unsigned precision = 6) {
		return pfc::format_time_ex( query(), precision ).get_ptr();
	}
private:
	double _query(t_uint64 p_val) const {
		return (double)( p_val - m_start ) / (double) g_query_freq();
	}
	static t_uint64 g_query() {
		LARGE_INTEGER val;
		if (!QueryPerformanceCounter(&val)) throw pfc::exception_not_implemented();
		return val.QuadPart;
	}
	static t_uint64 g_query_freq() {
		LARGE_INTEGER val;
		if (!QueryPerformanceFrequency(&val)) throw pfc::exception_not_implemented();
		return val.QuadPart;
	}
	t_uint64 m_start;
};

class lores_timer {
public:
    lores_timer() : m_start() {}
	void start() {
		_start(getTickCount());
	}

	double query() const {
		return _query(getTickCount());
	}
	double query_reset() {
		tickcount_t time = getTickCount();
		double ret = _query(time);
		_start(time);
		return ret;
	}
	pfc::string8 queryString(unsigned precision = 3) {
		return pfc::format_time_ex( query(), precision ).get_ptr();
	}
private:
	void _start(tickcount_t p_time) {
#ifdef PFC_TICKCOUNT_32BIT
		m_last_seen = p_time;
#endif
		m_start = p_time;
	}
	double _query(tickcount_t p_time) const {
#ifdef PFC_TICKCOUNT_32BIT
		t_uint64 time = p_time;
		if (time < (m_last_seen & 0xFFFFFFFF)) time += 0x100000000;
		m_last_seen = (m_last_seen & 0xFFFFFFFF00000000) + time;
		return (double)(m_last_seen - m_start) / 1000.0;
#else
		return (double)(p_time - m_start) / 1000.0;
#endif
	}
	t_uint64 m_start;
#ifdef PFC_TICKCOUNT_32BIT
	mutable t_uint64 m_last_seen;
#endif
};
}
#else  // not _WIN32

namespace pfc {
    
class hires_timer {
public:
    hires_timer() : m_start() {}
    void start();
    double query() const;
    double query_reset();
	pfc::string8 queryString(unsigned precision = 3);
private:
    double m_start;
};

typedef hires_timer lores_timer;

}

#endif // _WIN32

#ifndef _WIN32
struct timespec;
#endif

namespace pfc {
	uint64_t fileTimeWtoU(uint64_t ft);
	uint64_t fileTimeUtoW(uint64_t ft);
#ifndef _WIN32
    uint64_t fileTimeUtoW( timespec const & ts );
#endif
	uint64_t fileTimeNow();
}

#endif
