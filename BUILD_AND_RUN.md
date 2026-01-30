# Build and run TPT (The Powder Toy)

This project uses **Meson** and **Ninja**. Two ways to build:

---

## Option A: Prebuilt libraries (easiest)

Meson will download SDL2, LuaJIT, bzip2, etc. from the official tpt-libs release. You only need:

- **meson** and **ninja**
- **C++ compiler** (gcc or clang)
- **Python 3** (for Meson)

### 1. Install build tools (Manjaro/Arch)

```bash
sudo pacman -S meson ninja base-devel
```

### 2. Configure and build

```bash
cd /path/to/universal-physics-mod
meson setup build -Dstatic=prebuilt -Dbuildtype=release
ninja -C build
```

To **reconfigure** an existing build dir (e.g. switch to release):

```bash
meson setup build -Dstatic=prebuilt -Dbuildtype=release --reconfigure
ninja -C build
```

**Important:** Use `-Dbuildtype=release` for normal play. The default is `debug` (no optimization) and the Rusanov air solver runs ~10–50× slower in debug, giving low FPS.

### 3. Run

```bash
./build/powder
```

---

## Option B: System libraries

Use your distro’s SDL2, LuaJIT, fftw, etc. Slightly more work but no download of prebuilt libs.

### 1. Install dependencies

**Manjaro / Arch:**

```bash
sudo pacman -S meson ninja base-devel sdl2 luajit fftw jsoncpp libpng curl bzip2
```

(On Arch-based distros there is no separate `-dev` package; headers come with the main package.)

### 2. Configure

On **Manjaro/Arch**, bzip2 is in `/usr/lib`, not `/usr/lib/x86_64-linux-gnu`, so override the lib dir:

```bash
meson setup build \
  -Dworkaround_elusive_bzip2_lib_dir=/usr/lib
```

If you’re on **Debian/Ubuntu**, the default paths are fine:

```bash
sudo apt install libluajit-5.1-dev libcurl4-openssl-dev libfftw3-dev libsdl2-dev libbz2-dev libjsoncpp-dev libpng-dev
meson setup build
```

### 3. Build and run

```bash
ninja -C build
./build/powder
```

---

## Useful options

- **Debug build:**  
  `meson setup build -Dbuildtype=debug`  
  (or add `-Dbuildtype=debug` to your `meson setup` line.)

- **Build without HTTP (no libcurl):**  
  `-Dhttp=false`

- **Build without Lua:**  
  `-Dlua=none`

- **Change executable name** (e.g. for a mod):  
  `-Dapp_exe=mygame`

Reconfigure later with:

```bash
meson configure build -Doption=value
```

Then run `ninja -C build` again.
