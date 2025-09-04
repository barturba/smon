# smon

Minimalist system monitor for macOS Apple Silicon (ARM64). Real-time CPU, memory, network, and process monitoring with ncurses UI.

## Features

- Live CPU usage (total + per-core)
- Memory and swap usage
- Network throughput
- Top processes by CPU
- Load average and uptime
- Keyboard navigation (q to quit, arrows to scroll)

## Requirements

- macOS 11.0+ on Apple Silicon
- ncurses (`brew install ncurses`)

## Build

```bash
git clone https://github.com/barturba/smon.git
cd smon
make
```

## Usage

```bash
./build/smon
```

## License

MIT
