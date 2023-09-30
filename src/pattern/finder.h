#ifndef _PATTERN_FINDER_H_
#define _PATTERN_FINDER_H_

#include <stdint.h>

class PatternFinder
{
public:
	bool SetupLibrary(const void* lib, bool handle = false);
	void *Find(const void* pattern, size_t size);

private:
	uint8_t* m_baseAddr = nullptr;
	size_t m_size = 0;
};

#endif // _PATTERN_FINDER_H_
