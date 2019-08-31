#include <cstdio>
#include <fstream>
#include <memory>

#include "chip8.h"


namespace
{
	void usage(const char * name)
	{
		printf("Usage: %s rom\n", name);
	}
}



int main(int argc, char**argv)
{
	if (argc != 2)
	{
		usage(argv[0]);
		return 0;
	}
	
	
	// Try and load the ROM
	emu::ROM rom;
	{
		const char * romPath = argv[1];
		std::ifstream file(romPath, std::ios_base::binary);
		
		// Open it
		if (!file.is_open())
		{
			printf("Failed to open file: \"%s\"\n", romPath);
			return 1;
		}
		
		// Get the size
		file.seekg(0, std::ios_base::end);
		const auto size = file.tellg();
		file.seekg(0, std::ios_base::beg);
		
		// Copy the data to a temporary buffer
		// This should really be part of ROM
		auto buffer = std::make_unique<char[]>(size);
		file.read(buffer.get(), size);
		
		// Load it into a ROM
		if (!rom.Load(buffer.get(), size))
		{
			printf("Failed to load ROM data.\n");
			return 1;
		}
	}
	
	
	// Load the ROM into the emulator
	emu::CHIP8 chip8;
	if (!chip8.Load(rom))
	{
		printf("Failed to load ROM into emulator.\n");
		return 1;
	}
	
	// Run it
	while (true)
	{
		chip8.Step(1);
	}
	
	
	return 0;
}
