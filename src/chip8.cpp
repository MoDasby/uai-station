#include <algorithm>
#include <fstream>
#include <iterator>
#include <emulator.hpp>
#include <chip8.hpp>

namespace {
uint32_t parse_hex_color(std::string value) {
	if (!value.empty() && value.front() == '#') {
		value.erase(0, 1);
	}

	if (value.size() == 6) {
		value += "FF";
	}

	return (static_cast<uint32_t>(std::stoul(value.substr(6, 2), nullptr, 16)) << 24) |
		   (static_cast<uint32_t>(std::stoul(value.substr(0, 2), nullptr, 16)) << 16) |
		   (static_cast<uint32_t>(std::stoul(value.substr(2, 2), nullptr, 16)) << 8) |
		   static_cast<uint32_t>(std::stoul(value.substr(4, 2), nullptr, 16));
}
}

Chip8::Chip8(std::map<std::string, std::string>* metadata)
	: IEmulator(metadata) {
	loadFontset();

	std::string primary_color_str = "FFFFFF";
	if (metadata && metadata->count("primary_color")) {
		primary_color_str = (*metadata)["primary_color"];
	}

	PRIMARY_COLOR = parse_hex_color(primary_color_str);

	std::string secondary_color_str = "000000";
	if (metadata && metadata->count("secondary_color")) {
		secondary_color_str = (*metadata)["secondary_color"];
	}

	SECONDARY_COLOR = parse_hex_color(secondary_color_str);
}

bool Chip8::load_rom(const std::filesystem::path& path) {
	std::ifstream file(path, std::ios::binary);

	if (!file) {
		return false;
	}

	std::vector<uint8_t> rom(
		std::istreambuf_iterator<char>(file),
		{}
	);

	if (rom.size() > memory.size() - 0x200) {
		return false;
	}

	return std::copy(
		rom.begin(),
		rom.end(),
		memory.begin() + 0x200
	);
}

void Chip8::reset() {
	*this = Chip8{};
	accumulator_ = 0.0;
}

void Chip8::pause() {
	paused_ = true;
}

void Chip8::resume() {
	paused_ = false;
}

void Chip8::update(double dt) {
	if (paused_) {
		return;
	}

	accumulator_ += dt;

	constexpr double fixedStep = 1.0 / 60.0;

	while (accumulator_ >= fixedStep) {
		cycleFrame();
		accumulator_ -= fixedStep;
	}
}

void Chip8::render_frame(std::span<uint32_t> buffer) {
	for (size_t y = 0; y < HEIGHT; ++y) {
		for (size_t x = 0; x < WIDTH; ++x) {
			size_t offset = y * WIDTH + x;

			buffer[offset] = screen[y][x] ? PRIMARY_COLOR : SECONDARY_COLOR;
		}
	}
}

void Chip8::loadFontset() {
	const std::array<uint8_t, 80> fontset = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	std::copy(fontset.begin(), fontset.end(), memory.begin() + 0x50);
}

void Chip8::updateKeyboard(uint16_t key, bool state) {
	if (key < 16u) {
		keyboard[key] = state;
	}
}

void Chip8::cycleFrame() {
	if (delayTimer > 0) {
		--delayTimer;
	}

	if (soundTimer > 0) {
		--soundTimer;
	}

	constexpr int cyclesPerFrame = 12;

	for (int cycle = 0; cycle < cyclesPerFrame; ++cycle) {
		uint16_t instruction = decode_instruction();

		pc += 2;

		process(instruction);
	}
}

uint16_t Chip8::decode_instruction() {
	return (static_cast<uint16_t>(memory[pc]) << 8) |
			static_cast<uint16_t>(memory[pc + 1]);
}

void Chip8::process(uint16_t instruction) {
	if (instruction == 0x00E0) {
		for (auto& row : screen) {
			row.fill(0);
		}

		return;
	}

	uint16_t category = instruction >> 12;

	switch (category) {
	case 0x0: {
		uint16_t op = instruction & 0x00FF;

		switch (op) {
		case 0xEE:
			--sp;
			pc = stack[sp];
			break;
		}

		break;
	}

	case 0x1:
		pc = instruction & 0x0FFF;
		break;

	case 0x2:
		stack[sp] = pc;
		pc = instruction & 0x0FFF;
		++sp;
		break;

	case 0x3: {
		uint16_t index = (instruction & 0x0F00) >> 8;

		if (v[index] == static_cast<uint8_t>(instruction & 0x00FF)) {
			pc += 2;
		}

		break;
	}

	case 0x4: {
		uint16_t index = (instruction & 0x0F00) >> 8;

		if (v[index] != static_cast<uint8_t>(instruction & 0x00FF)) {
			pc += 2;
		}

		break;
	}

	case 0x5: {
		uint16_t indexX = (instruction & 0x0F00) >> 8;
		uint16_t indexY = (instruction & 0x00F0) >> 4;

		if (v[indexX] == v[indexY]) {
			pc += 2;
		}

		break;
	}

	case 0x6: {
		uint16_t index = (instruction & 0x0F00) >> 8;

		v[index] = static_cast<uint8_t>(instruction & 0x00FF);

		break;
	}

	case 0x7: {
		uint16_t index = (instruction & 0x0F00) >> 8;

		v[index] += static_cast<uint8_t>(instruction & 0x00FF);

		break;
	}

	case 0x8: {
		uint16_t op = instruction & 0x000F;

		uint16_t indexX = (instruction & 0x0F00) >> 8;
		uint16_t indexY = (instruction & 0x00F0) >> 4;

		switch (op) {
		case 0x0:
			v[indexX] = v[indexY];
			break;

		case 0x1:
			v[indexX] |= v[indexY];
			break;

		case 0x2:
			v[indexX] &= v[indexY];
			break;

		case 0x3:
			v[indexX] ^= v[indexY];
			break;

		case 0x4: {
			uint16_t sum =
				static_cast<uint16_t>(v[indexX]) +
				static_cast<uint16_t>(v[indexY]);

			v[0xF] = (sum > 255) ? 1 : 0;
			v[indexX] = static_cast<uint8_t>(sum & 0xFF);

			break;
		}

		case 0x5: {
			v[0xF] = (v[indexX] >= v[indexY]) ? 1 : 0;

			v[indexX] -= v[indexY];

			break;
		}

		case 0x6: {
			v[0xF] = v[indexX] & 0x01;

			v[indexX] >>= 1;

			break;
		}

		case 0x7: {
			v[0xF] = (v[indexY] >= v[indexX]) ? 1 : 0;

			v[indexX] = v[indexY] - v[indexX];

			break;
		}

		case 0xE: {
			v[0xF] = (v[indexX] & 0x80) >> 7;

			v[indexX] <<= 1;

			break;
		}
		}

		break;
	}

	case 0x9: {
		uint16_t indexX = (instruction & 0x0F00) >> 8;
		uint16_t indexY = (instruction & 0x00F0) >> 4;

		if (v[indexX] != v[indexY]) {
			pc += 2;
		}

		break;
	}

	case 0xA:
		i = instruction & 0x0FFF;
		break;

	case 0xB: {
		uint16_t nnn = instruction & 0x0FFF;

		pc = nnn + static_cast<uint16_t>(v[0]);

		break;
	}

	case 0xC: {
		uint16_t index = (instruction & 0x0F00) >> 8;
		uint8_t nn = instruction & 0x00FF;

		uint8_t randomNumber =
			static_cast<uint8_t>(randomByte(rng));

		v[index] = randomNumber & nn;

		break;
	}

	case 0xD: {
		uint16_t indexX = (instruction & 0x0F00) >> 8;
		uint16_t indexY = (instruction & 0x00F0) >> 4;

		uint16_t n = instruction & 0x000F;

		uint8_t x = v[indexX] % 64;
		uint8_t y = v[indexY] % 32;

		v[0xF] = 0;

		for (uint16_t line = 0; line < n; ++line) {
			uint8_t sprite = memory[i + line];

			for (uint8_t column = 0; column < 8; ++column) {
				uint8_t pixel =
					(sprite >> (7 - column)) & 1;

				uint16_t finalX =
					static_cast<uint16_t>(x) + column;

				uint16_t finalY =
					static_cast<uint16_t>(y) + line;

				if (finalX > 63 || finalY > 31) {
					continue;
				}

				if (pixel == 1) {
					if (screen[finalY][finalX] == 1) {
						v[0xF] = 1;
					}

					screen[finalY][finalX] ^= 1;
				}
			}
		}

		break;
	}

	case 0xE: {
		uint16_t op = instruction & 0x00FF;
		uint16_t index = (instruction & 0x0F00) >> 8;

		switch (op) {
		case 0x9E:
			if (keyboard[v[index]]) {
				pc += 2;
			}
			break;

		case 0xA1:
			if (!keyboard[v[index]]) {
				pc += 2;
			}
			break;
		}

		break;
	}

	case 0xF: {
		uint16_t index = (instruction & 0x0F00) >> 8;
		uint16_t op = instruction & 0x00FF;

		switch (op) {
		case 0x07:
			v[index] = delayTimer;
			break;

		case 0x0A: {
			int anyPressed = -1;

			for (int key = 0; key < 16; ++key) {
				if (keyboard[key]) {
					anyPressed = key;
					break;
				}
			}

			if (anyPressed < 0) {
				pc -= 2;
			} else {
				v[index] = static_cast<uint8_t>(anyPressed);
			}

			break;
		}

		case 0x15:
			delayTimer = v[index];
			break;

		case 0x18:
			soundTimer = v[index];
			break;

		case 0x1E:
			i += static_cast<uint16_t>(v[index]);
			break;

		case 0x29:
			i = 0x50 + static_cast<uint16_t>(v[index]) * 5;
			break;

		case 0x33: {
			uint8_t num = v[index];

			memory[i] = num / 100;
			memory[i + 1] = (num / 10) % 10;
			memory[i + 2] = num % 10;

			break;
		}

		case 0x55:
			for (uint16_t vi = 0; vi <= index; ++vi) {
				memory[i + vi] = v[vi];
			}
			break;

		case 0x65:
			for (uint16_t vi = 0; vi <= index; ++vi) {
				v[vi] = memory[i + vi];
			}
			break;
		}

		break;
	}
	}
}