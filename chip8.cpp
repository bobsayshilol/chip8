#include <cstring>
#include "chip8.h"

namespace
{
	uint16_t bswap(uint16_t val) noexcept
	{
		uint16_t o = 0;
		o |= (val >> 8) & 0x00FF;
		o |= (val << 8) & 0xFF00;
		return o;
	}
	
	constexpr uint8_t kCharacterSprites[5 * 16] = {
		// 0
		0b11110000,
		0b10010000,
		0b10010000,
		0b10010000,
		0b11110000,
		
		// 1
		0b01100000,
		0b10100000,
		0b00100000,
		0b00100000,
		0b11110000,
		
		// 2
		0b11110000,
		0b00010000,
		0b11110000,
		0b10000000,
		0b11110000,
		
		// 3
		0b11110000,
		0b00010000,
		0b11110000,
		0b00010000,
		0b11110000,
		
		// 4
		0b10010000,
		0b10010000,
		0b11110000,
		0b00010000,
		0b00010000,
		
		// 5
		0b11110000,
		0b10000000,
		0b11110000,
		0b00010000,
		0b11110000,
		
		// 6
		0b11110000,
		0b10000000,
		0b11110000,
		0b10010000,
		0b11110000,
		
		// 7
		0b11110000,
		0b00010000,
		0b00010000,
		0b00010000,
		0b00010000,
		
		// 8
		0b11110000,
		0b10010000,
		0b11110000,
		0b10010000,
		0b11110000,
		
		// 9
		0b11110000,
		0b10010000,
		0b11110000,
		0b00010000,
		0b00010000,
		
		// A
		0b11110000,
		0b10010000,
		0b11110000,
		0b10010000,
		0b10010000,
		
		// B
		0b11100000,
		0b10010000,
		0b11100000,
		0b10010000,
		0b11100000,
		
		// C
		0b11110000,
		0b10000000,
		0b10000000,
		0b10000000,
		0b11110000,
		
		// D
		0b11100000,
		0b10010000,
		0b10010000,
		0b10010000,
		0b11100000,
		
		// E
		0b11110000,
		0b10000000,
		0b11110000,
		0b10000000,
		0b11110000,
		
		// F
		0b11110000,
		0b10000000,
		0b11110000,
		0b10000000,
		0b10000000,
	};
}

namespace emu
{
	bool ROM::Load(const void* data, std::size_t size)
	{
		mData.resize(size);
		std::memcpy(mData.data(), data, size);
		return true;
	}
	
	
	CHIP8::CHIP8()
		: mRAM{}
		, mDisplayBuffer{}
		, mRegisters{}
		, mPC{}
		, mI{}
		, mDelayTimer{}
		, mSoundTimer{}
		, mKeyboard{}
		, mKeyboardRegister{0xFF}
		, mStackFrames{}
		, mStack{}
	{
	}
	
	bool CHIP8::Load(const ROM& rom, Program type)
	{
		bool loaded = false;
		const auto& data = rom.GetData();
		
		// Different types of programs start at different offsets
		const size_t offset = type == Program::CHIP8 ? 0x200 : 0x600;
		
		if (data.size() + offset < mRAM.size())
		{
			std::copy(data.begin(), data.end(), mRAM.begin() + offset);
			mPC = offset;
			
			std::memcpy(&mRAM[kCharacterSpritesStart], kCharacterSprites, sizeof(kCharacterSprites));
			
			loaded = true;
		}
		
		return loaded;
	}
	
	void CHIP8::Step(std::size_t instructions)
	{
		for (size_t i = 0; i < instructions; i++)
		{
			// Wait for input if we need to
			if (mKeyboardRegister != 0xFF)
			{
				if (!mKeyboard.any())
				{
					// No button presses yet
					break;
				}
				else
				{
					// Find the button that is pressed
					for (size_t i = 0; i < mKeyboard.size(); i++)
					{
						if (mKeyboard[i])
						{
							mRegisters[mKeyboardRegister] = i;
							mKeyboardRegister = 0xFF;
						}
					}
				}
			}
			
			// Read the next instruction
			const Instruction ins = ReadInstruction();
			
			// Handle it
			switch (ins / 0x1000)
			{
#define CASE(x) case 0x ## x : Handle_ ## x (ins); break
				
				CASE(0);	CASE(1);	CASE(2);	CASE(3);
				CASE(4);	CASE(5);	CASE(6);	CASE(7);
				CASE(8);	CASE(9);	CASE(A);	CASE(B);
				CASE(C);	CASE(D);	CASE(E);	CASE(F);
				
#undef CASE
			}
		}
	}
	
	void CHIP8::Tick()
	{
		if (mDelayTimer > 0)
		{
			mDelayTimer--;
		}
		
		if (mSoundTimer > 0)
		{
			mSoundTimer--;
		}
	}
	
	void CHIP8::Dump() const
	{
		printf("CHIP-8 state:\n");
		
		// Registers
		printf("\tRegisters:\n");
		for (size_t i = 0; i < mRegisters.size(); i++)
		{
			if ((i & 3) == 0)
			{
				printf("\t");
			}
			printf("\tV%zX: 0x%02X", i, mRegisters[i]);
			if ((i & 3) == 3)
			{
				printf("\n");
			}
		}
		printf("\t");
		printf("\tPC: 0x%02X", mPC);
		printf("\tI:  0x%02X", mI);
		printf("\tD:  0x%02X", mDelayTimer);
		printf("\tS:  0x%02X", mSoundTimer);
		printf("\n");
		
		// Stack
		printf("\tStack (%zu frames):\n", mStack);
		for (size_t i = 0; i < mStack; i++)
		{
			printf("\t\t%zu:\t0x%03X\n", i, mStackFrames[i]);
		}
	}
	
	
	bool CHIP8::NeedsRedraw() const
	{
		// Grab the base of the display data
		const std::byte * displayData = &mRAM[kDisplayStart];
		
		// See if anything has changed
		return std::memcmp(displayData, mDisplayBuffer.begin(), mDisplayBuffer.size()) != 0;
	}
	
	void CHIP8::Draw()
	{
		// Grab the base of the display data
		const std::byte * displayData = &mRAM[kDisplayStart];
		
		// Update the cached buffer
		std::memcpy(mDisplayBuffer.begin(), displayData, mDisplayBuffer.size());
		
		auto border = []()
		{
			printf("+");
			for (size_t x = 0; x < kDisplayWidth; x++)
			{
				printf("-");
			}
			printf("+\n");
		};
		
		// Print out the pixel display with a border
		border();
		for (size_t y = 0; y < kDisplayHeight; y++)
		{
			printf("|");
			for (size_t x = 0; x < kDisplayWidth / 8; x++)
			{
				// Since the pixels are encoded as bits we can read a byte and deal with that
				uint8_t block = static_cast<uint8_t>(*displayData++);
				for (size_t i = 0; i < 8; i++)
				{
					const bool isSet = block & (1 << (7 - i));
					printf(isSet ? "#" : " ");
				}
			}
			printf("|\n");
		}
		border();
	}
	
	
	void CHIP8::OnError(const char* msg) const
	{
		// Dump the state of the emulator before throwing so we have a chance at seeing what's going on
		Dump();
		throw std::runtime_error(msg);
	}
	
	CHIP8::Instruction CHIP8::ReadInstruction()
	{
		// Check for overflow
		if (mPC + sizeof(Instruction) >= mRAM.size())
		{
			OnError("Program counter left RAM");
		}
		
		// Read the instruction
		Instruction ins;
		memcpy(&ins, &mRAM[mPC], sizeof(ins));
		ins = bswap(ins);
		
		// Update the PC
		mPC += sizeof(Instruction);
		
		return ins;
	}
	
	
	void CHIP8::Unhandled(Instruction ins)
	{
		// Log the unhandled instruction
		char message[64];
		snprintf(message, sizeof(message), "Unhandled instruction: 0x%X", ins);
		OnError(message);
	}
	
	void CHIP8::Handle_0(Instruction ins)
	{
		// Read off the program
		const Address program = ins & 0x0FFF;
		
		switch (program)
		{
			case 0x00E0:
			{
				// Grab the base of the display data
				std::byte * displayData = &mRAM[kDisplayStart];
				
				// Clear it all
				std::memset(displayData, 0, kDisplaySize);
			}
			break;
			
			
			case 0x00EE:
			{
				// Pop the return address from the stack
				if (mStack == 0)
				{
					OnError("Out of stack frames");
				}
				mStack--;
				const Address address = mStackFrames[mStack];
				
				// Check the address hasn't been corrupted somehow
				if (address >= mRAM.size())
				{
					OnError("Invalid address on stack");
				}
				
				// Update PC
				mPC = address;
			}
			break;
			
			
			default:
				Unhandled(ins);
				break;
		}
	}
	
	void CHIP8::Handle_1(Instruction ins)
	{
		// Read off the address
		const Address address = ins & 0x0FFF;
		
		// Update PC
		mPC = address;
	}
	
	void CHIP8::Handle_2(Instruction ins)
	{
		// Read off the address
		const Address address = ins & 0x0FFF;
		
		// Push the current return address onto the stack
		if (mStack + 1 > mStackFrames.size())
		{
			OnError("Out of stack frames");
		}
		mStackFrames[mStack] = mPC;
		mStack++;
		
		// Update PC
		mPC = address;
	}
	
	void CHIP8::Handle_3(Instruction ins)
	{
		// Read off the register and value
		const uint8_t reg = (ins >> 8) & 0x0F;
		const uint8_t val = (ins >> 0) & 0xFF;
		
		if (mRegisters[reg] == val)
		{
			// Skip an instruction
			if (mPC + sizeof(Instruction) >= mRAM.size())
			{
				OnError("Branching outside of RAM");
			}
			mPC += sizeof(Instruction);
		}
	}
	
	void CHIP8::Handle_4(Instruction ins)
	{
		// Read off the register and value
		const uint8_t reg = (ins >> 8) & 0x0F;
		const uint8_t val = (ins >> 0) & 0xFF;
		
		if (mRegisters[reg] != val)
		{
			// Skip an instruction
			if (mPC + sizeof(Instruction) >= mRAM.size())
			{
				OnError("Branching outside of RAM");
			}
			mPC += sizeof(Instruction);
		}
	}
	
	void CHIP8::Handle_5(Instruction ins)
	{
		// Read off the registers and op
		const uint8_t rx = (ins >> 8) & 0x0F;
		const uint8_t ry = (ins >> 4) & 0x0F;
		const uint8_t op = (ins >> 0) & 0x0F;
		
		const uint8_t x = mRegisters[rx];
		const uint8_t y = mRegisters[ry];
		
		switch (op)
		{
			case 0x0:
			{
				if (x == y)
				{
					// Skip an instruction
					if (mPC + sizeof(Instruction) >= mRAM.size())
					{
						OnError("Branching outside of RAM");
					}
					mPC += sizeof(Instruction);
				}
			}
			break;
			
			default:
				Unhandled(ins);
				break;
		}
	}
	
	void CHIP8::Handle_6(Instruction ins)
	{
		// Read off the register and value
		const uint8_t reg = (ins >> 8) & 0x0F;
		const uint8_t val = (ins >> 0) & 0xFF;
		
		// Update the register
		mRegisters[reg] = val;
	}
	
	void CHIP8::Handle_7(Instruction ins)
	{
		// Read off the register and value
		const uint8_t reg = (ins >> 8) & 0x0F;
		const uint8_t val = (ins >> 0) & 0xFF;
		
		// Update the register
		mRegisters[reg] += val;
	}
	
	void CHIP8::Handle_8(Instruction ins)
	{
		// Read off the registers and op
		const uint8_t rx = (ins >> 8) & 0x0F;
		const uint8_t ry = (ins >> 4) & 0x0F;
		const uint8_t op = (ins >> 0) & 0x0F;
		
		uint8_t& x = mRegisters[rx];
		const uint8_t y = mRegisters[ry];
		
		switch (op)
		{
			case 0x0:	x  = y;		break;
			case 0x1:	x |= y;		break;
			case 0x2:	x &= y;		break;
			case 0x3:	x ^= y;		break;
			
			case 0x4:
			{
				if (rx == 0xF || ry == 0xF) OnError("Ordering");
				const bool carry = x + y > 0xFF;
				x += y;
				mRegisters[0xF] = carry ? 1 : 0;
			}
			break;
			
			case 0x5:
			{
				if (rx == 0xF || ry == 0xF) OnError("Ordering");
				const bool borrow = x < y;
				x -= y;
				mRegisters[0xF] = borrow ? 0 : 1;
			}
			break;
			
			case 0x7:
			{
				if (rx == 0xF || ry == 0xF) OnError("Ordering");
				const bool borrow = y < x;
				x = y - x;
				mRegisters[0xF] = borrow ? 0 : 1;
			}
			break;
			
			
			case 0x6:
				if (rx == 0xF || ry == 0xF) OnError("Ordering");
				mRegisters[0xF] = (x >> 0) & 1;
				x >>= 1;
				break;
			
			case 0xE:
				if (rx == 0xF || ry == 0xF) OnError("Ordering");
				mRegisters[0xF] = (x >> 7) & 1;
				x <<= 1;
				break;
			
			
			default:
				Unhandled(ins);
				break;
		}
	}
	
	void CHIP8::Handle_9(Instruction ins)
	{
		// Read off the registers and op
		const uint8_t rx = (ins >> 8) & 0x0F;
		const uint8_t ry = (ins >> 4) & 0x0F;
		const uint8_t op = (ins >> 0) & 0x0F;
		
		const uint8_t x = mRegisters[rx];
		const uint8_t y = mRegisters[ry];
		
		switch (op)
		{
			case 0x0:
			{
				if (x != y)
				{
					// Skip an instruction
					if (mPC + sizeof(Instruction) >= mRAM.size())
					{
						OnError("Branching outside of RAM");
					}
					mPC += sizeof(Instruction);
				}
			}
			break;
			
			default:
				Unhandled(ins);
				break;
		}
	}
	
	void CHIP8::Handle_A(Instruction ins)
	{
		// Read off the address
		const Address address = ins & 0x0FFF;
		
		// Update I
		mI = address;
	}
	
	void CHIP8::Handle_B(Instruction ins)
	{
		// Read off the address
		const Address address = ins & 0x0FFF;
		
		if (mRegisters[0] + address > mRAM.size())
		{
			OnError("Trying to jump out of RAM");
		}
		
		// Update PC
		mPC = mRegisters[0] + address;
	}
	
	void CHIP8::Handle_C(Instruction ins)
	{
		// Read off the register and value
		const uint8_t reg = (ins >> 8) & 0x0F;
		const uint8_t max = (ins >> 0) & 0xFF;
		
		// Generate the random number
		const uint8_t val = rand() & max;
		
		// Update the register
		mRegisters[reg] = val;
	}
	
	void CHIP8::Handle_D(Instruction ins)
	{
		// Read off VX, VY, and N
		const uint8_t vx = (ins >> 8) & 0x0F;
		const uint8_t vy = (ins >> 4) & 0x0F;
		const size_t n   = (ins >> 0) & 0x0F;
		
		// Read X and Y from the registers
		const size_t baseX = mRegisters[vx];
		const size_t baseY = mRegisters[vy];
		
		// Grab the base of the display data
		uint8_t * displayData = reinterpret_cast<uint8_t*>(&mRAM[kDisplayStart]);
		
		// Grab the base of where we're blitting from
		const uint8_t * srcData = reinterpret_cast<uint8_t*>(&mRAM[mI]);
		
		// Sanity check where we're blitting from
		if (mI + n >= mRAM.size())
		{
			OnError("Blitting from outside of RAM");
		}
		
		// Do the blit
		bool flippedOff = false;
		for (size_t srcY = 0; srcY < n; srcY++)
		{
			for (size_t srcX = 0; srcX < 8; srcX++)
			{
				// Out of bounds wraps
				const size_t dispX = (srcX + baseX) % kDisplayWidth;
				const size_t dispY = (srcY + baseY) % kDisplayHeight;
				
				// Calculate where in memory we need to blit to
				const size_t pixelNum = dispY * kDisplayWidth + dispX;
				const size_t pixelBlockNum = pixelNum / 8;
				
				// Pixels are backwards, ie highest bit comes first
				const size_t pixelBlockBit = 7 - (pixelNum - 8 * pixelBlockNum);
				
				// Read the destination block
				uint8_t dstBlock = displayData[pixelBlockNum];
				
				// Read the relevant src bit
				const bool srcBit = *srcData & (1 << (7 - srcX));
				
				// Raise the flag if required
				const bool dstBit = dstBlock & (1 << pixelBlockBit);
				if (srcBit && dstBit)
				{
					flippedOff = true;
				}
				
				// Flip the pixel
				dstBlock ^= (srcBit ? 1 : 0) << pixelBlockBit;
				
				// Save it back
				displayData[pixelBlockNum] = dstBlock;
			}
			
			srcData++;
		}
		
		// Store the result of the flips in VF
		mRegisters[0xF] = flippedOff ? 1 : 0;
	}
	
	void CHIP8::Handle_E(Instruction ins)
	{
		// Read off the register and op
		const uint8_t reg = (ins >> 8) & 0x0F;
		const uint8_t op  = (ins >> 0) & 0xFF;
		
		uint8_t& val = mRegisters[reg];
		
		switch (op)
		{
			case 0x9E:
			{
				if (val >= mKeyboard.size())
				{
					OnError("Invalid key code requested");
				}
				
				if (mKeyboard[val])
				{
					// Skip the next instruction
					mPC += sizeof(Instruction);
				}
			}
			break;
			
			case 0xA1:
			{
				if (val >= mKeyboard.size())
				{
					OnError("Invalid key code requested");
				}
				
				if (!mKeyboard[val])
				{
					// Skip the next instruction
					mPC += sizeof(Instruction);
				}
			}
			break;
			
			
			default:
				Unhandled(ins);
				break;
		}
	}
	
	void CHIP8::Handle_F(Instruction ins)
	{
		// Read off the register and op
		const uint8_t reg = (ins >> 8) & 0x0F;
		const uint8_t op  = (ins >> 0) & 0xFF;
		
		uint8_t& val = mRegisters[reg];
		
		switch (op)
		{
			case 0x07:
			{
				val = mDelayTimer;
			}
			break;
			
			case 0x0A:
			{
				// Save the register we want the key press to be saved to.
				// This will be handled on the next call to Step().
				mKeyboardRegister = reg;
			}
			break;
			
			case 0x15:
			{
				mDelayTimer = val;
			}
			break;
			
			case 0x18:
			{
				mSoundTimer = val;
			}
			break;
			
			
			case 0x1E:
			{
				if (mI + val > mRAM.size())
				{
					OnError("Moving I to outside of RAM");
				}
				mI += val;
			}
			break;
			
			
			case 0x29:
			{
				if (val >= 16)
				{
					OnError("Unknown key");
				}
				
				// Each sprite is 5 lines long
				mI = kCharacterSpritesStart + val * 5;
			}
			break;
			
			
			case 0x33:
			{
				if (mI + 3 > mRAM.size())
				{
					OnError("Storing to I outside of RAM");
				}
				auto * ptr = reinterpret_cast<uint8_t*>(&mRAM[mI]);
				
				ptr[0] = (val / 100) % 10;
				ptr[1] = (val /  10) % 10;
				ptr[2] = (val /   1) % 10;
			}
			break;
			
			
			case 0x55:
			{
				if (mI + reg > mRAM.size())
				{
					OnError("Copying to I outside of RAM");
				}
				std::memcpy(&mRAM[mI], &mRegisters[0], reg + 1);
			}
			break;
			
			case 0x65:
			{
				if (mI + reg > mRAM.size())
				{
					OnError("Copying from I outside of RAM");
				}
				std::memcpy(&mRegisters[0], &mRAM[mI], reg + 1);
			}
			break;
			
			
			default:
				Unhandled(ins);
				break;
		}
	}
}
