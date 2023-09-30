#include <stdint.h>
#include <string.h>

#ifdef _WIN32 // Windows
#include <Windows.h>
#elif __linux__ // Linux
#include <unistd.h>
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#define PAGE_SIZE			4096
#define PAGE_ALIGN_UP(x)	((x + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#else // Unknown
#error Unsupported operating system
#endif

#include "finder.h"

bool PatternFinder::SetupLibrary(const void* lib, bool handle)
{
	this->m_baseAddr = nullptr;
	this->m_size = 0;
	
	if(!lib)
	{
		return false;
	}

#ifdef _WINDOWS

#	ifdef PLATFORM_X86
	const WORD PE_FILE_MACHINE = IMAGE_FILE_MACHINE_I386;
	const WORD PE_NT_OPTIONAL_HDR_MAGIC = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
#	elif defined(PLATFORM_64BITS)
	const WORD PE_FILE_MACHINE = IMAGE_FILE_MACHINE_AMD64;
	const WORD PE_NT_OPTIONAL_HDR_MAGIC = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
#	else
#		error Unsupported platform
#	endif

	MEMORY_BASIC_INFORMATION info;
	IMAGE_DOS_HEADER *dos;
	IMAGE_NT_HEADERS *pe;
	IMAGE_FILE_HEADER *file;
	IMAGE_OPTIONAL_HEADER *opt;

	if (!VirtualQuery(libPtr, &info, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return nullptr;
	}

	baseAddr = reinterpret_cast<uintptr_t>(info.AllocationBase);

	/* All this is for our insane sanity checks :o */
	dos = reinterpret_cast<IMAGE_DOS_HEADER *>(baseAddr);
	pe = reinterpret_cast<IMAGE_NT_HEADERS *>(baseAddr + dos->e_lfanew);
	file = &pe->FileHeader;
	opt = &pe->OptionalHeader;

	/* Check PE magic and signature */
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE || opt->Magic != PE_NT_OPTIONAL_HDR_MAGIC)
	{
		return nullptr;
	}

	/* Check architecture */
	if (file->Machine != PE_FILE_MACHINE)
	{
		return nullptr;
	}

	/* For our purposes, this must be a dynamic library */
	if ((file->Characteristics & IMAGE_FILE_DLL) == 0)
	{
		return nullptr;
	}

	/* Finally, we can do this */
	lib.memorySize = opt->SizeOfImage;

#elif defined _LINUX
	uintptr_t baseAddr;
	
	if(handle)
	{
		baseAddr = reinterpret_cast<uintptr_t>(static_cast<const link_map*>(lib)->l_addr);
	}
	else
	{
		Dl_info info;
		
		if (!dladdr(lib, &info))
		{
			return false;
		}
		
		if (!info.dli_fname)
		{
			return false;
		}
		
		baseAddr = reinterpret_cast<uintptr_t>(info.dli_fbase);
	}

#	ifdef PLATFORM_X86
	typedef Elf32_Ehdr ElfHeader;
	typedef Elf32_Phdr ElfPHeader;
	const unsigned char ELF_CLASS = ELFCLASS32;
	const uint16_t ELF_MACHINE = EM_386;
#	elif defined(PLATFORM_64BITS)
	typedef Elf64_Ehdr ElfHeader;
	typedef Elf64_Phdr ElfPHeader;
	const unsigned char ELF_CLASS = ELFCLASS64;
	const uint16_t ELF_MACHINE = EM_X86_64;
#	else
#		error Unsupported platform
#	endif

	Dl_info info;
	ElfHeader *file;
	ElfPHeader *phdr;
	uint16_t phdrCount;

	if (!dladdr(lib, &info))
	{
		return false;
	}

	if (!info.dli_fbase || !info.dli_fname)
	{
		return false;
	}

	/* This is for our insane sanity checks :o */
	baseAddr = reinterpret_cast<uintptr_t>(info.dli_fbase);
	file = reinterpret_cast<ElfHeader *>(baseAddr);

	/* Check ELF magic */
	if (memcmp(ELFMAG, file->e_ident, SELFMAG) != 0)
	{
		return false;
	}

	/* Check ELF version */
	if (file->e_ident[EI_VERSION] != EV_CURRENT)
	{
		return false;
	}
	
	/* Check ELF endianness */
	if (file->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		return false;
	}

	/* Check ELF architecture */
	if (file->e_ident[EI_CLASS] != ELF_CLASS || file->e_machine != ELF_MACHINE)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library/shared object */
	if (file->e_type != ET_DYN)
	{
		return false;
	}

	phdrCount = file->e_phnum;
	phdr = reinterpret_cast<ElfPHeader *>(baseAddr + file->e_phoff);

	for (uint16_t i = 0; i < phdrCount; i++)
	{
		ElfPHeader &hdr = phdr[i];

		/* We only really care about the segment with executable code */
		if (hdr.p_type == PT_LOAD && hdr.p_flags == (PF_X|PF_R))
		{
			/* From glibc, elf/dl-load.c:
			 * c->mapend = ((ph->p_vaddr + ph->p_filesz + GLRO(dl_pagesize) - 1) 
			 *             & ~(GLRO(dl_pagesize) - 1));
			 *
			 * In glibc, the segment file size is aligned up to the nearest page size and
			 * added to the virtual address of the segment. We just want the size here.
			 */
			this->m_size = PAGE_ALIGN_UP(hdr.p_filesz);

			break;
		}
	}
#else
#	error Unsupported platform
#endif

	this->m_baseAddr = reinterpret_cast<uint8_t*>(baseAddr);

	return true;
}

void *PatternFinder::Find(const void* pattern, size_t size)
{
	if(!this->m_baseAddr)
	{
		return nullptr;
	}
	
	uint8_t* ptr = this->m_baseAddr;
	uint8_t* end = this->m_baseAddr + this->m_size - size;
	const uint8_t* _pattern = reinterpret_cast<const uint8_t*>(pattern);
	
	do
	{
		if(*ptr != _pattern[0] || *(ptr + size - 1) != _pattern[size - 1])
		{
			continue;
		}

		bool find = true;

		for(size_t i = 1; i < size - 1; i++)
		{
			if(_pattern[i] != ptr[i] && _pattern[i] != '\x2A')
			{
				find = false;
				
				break;
			}
		}

		if(find)
		{
			return reinterpret_cast<void *>(ptr);
		}
	}
	while(++ptr < end);

	return nullptr;
}