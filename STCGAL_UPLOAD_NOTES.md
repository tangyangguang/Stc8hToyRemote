# STCGAL Upload Notes

This project uses `stcgal` with STC8H UART ISP. A setup that looks reasonable can still be unstable, especially on macOS with USB-UART adapters.

## Known-good default

Current project defaults:

- protocol: `stc8g`
- upload port: `/dev/cu.usbserial-110`
- download baud: `19200`
- no default trim (`-t` not passed)
- no forced handshake baud (`-l` not passed)
- upload runner retries and falls back across multiple baud/handshake combinations

These values are set in:

- `controller/platformio.ini`
- `receiver/platformio.ini`
- `controller/upload_stcgal.py`
- `receiver/upload_stcgal.py`
- `tools/upload_stcgal_runner.py`

## What failed in practice

### 1. Wrong protocol family

Using PlatformIO's default uploader behavior for STC8H can fail early with protocol framing errors. This project must use `stcgal` with protocol `stc8g`.

### 2. Forcing RC trim made uploads less stable

Passing `-t 11059` / `-t 11059.2` looked attractive because the target runs around 11.0592 MHz, but this pushed failures into the `Target frequency` stage on this hardware.

`stcgal` documents that RC trimming uses the UART clock as reference, so UART timing quality directly affects trim reliability. On this board/adapter combination, removing trim was more stable than forcing it.

### 3. Lower handshake baud was not a universal improvement

Trying `-l 1200` did not help here. It changed the failure point but did not improve reliability. For this setup, leaving handshake baud at the tool default worked better.

### 4. Slower download baud was not always better

`9600` was not the most reliable setting here. `19200` matched `stcgal`'s conservative default guidance and worked better on this setup.

### 5. High download baud can work once retries exist

After adding a retry/fallback upload runner, this setup could also complete uploads at `38400` and `115200`.

That does **not** mean high baud is intrinsically stable on this board. The more accurate conclusion is:

- single-shot `stcgal` uploads were fragile on this setup
- automatic retry plus baud fallback made the workflow robust enough that even faster baud settings could succeed

So the important lesson is not "always use 115200". The important lesson is "do not rely on one upload attempt with one timing configuration".

## Practical rule for this repo

If `stcgal` starts failing with messages like:

- `Protocol error: incorrect frame start`
- `Target frequency: Disconnected!`
- `Finishing write: Disconnected!`

then do **not** change multiple timing variables at once. Prefer this order:

1. keep protocol at `stc8g`
2. keep baud at `19200`
3. remove forced trim
4. remove forced handshake baud
5. use the retry/fallback runner
6. only then experiment with `-l` or higher `-b`

Current retry/fallback strategy is:

1. same configured baud, retry twice
2. same baud with `-l 1200`
3. `9600`
4. `9600` with `-l 1200`
5. `4800` with `-l 1200`

This matches `stcgal` FAQ guidance better than hard-coding one supposedly magic baud rate.

## Encoder debugging lesson learned at the same time

The controller encoder issue was not a pin mapping problem. Two code-side causes mattered:

1. encoder scan was only happening once every 50 ms, which missed most quadrature transitions
2. after restoring 1 ms scanning, `DRV_EC11_SMALL_STEPS_PER_DETENT=2` over-counted; `4` matched the actual full detent sequence better

So for this controller:

- fast input polling matters more than it first appeared
- `steps_per_detent` cannot be judged correctly while scan frequency is too low
