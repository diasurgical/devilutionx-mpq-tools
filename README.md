# DevilutionX MPQ tools

## `unpack_and_minify_mpq`

Unpacks an MPQ and minifies its assets.

All graphics assets are converted to CLX.

### Usage

Simply drop the binary into the directory with the MPQs and run it.

Alternatively, run `unpack_and_minify_mpq --help` to see the list of options.

If `--mp3` is passed, audio is converted from WAV to MP3. Not implemented yet.

### Install

On Windows, download the latest release from https://github.com/diasurgical/devilutionx-mpq-tools/releases.

On other OSes, build and install from source (see below).

### Build from source

```bash
cmake -S. -Bbuild-rel -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
cmake --build build-rel -j $(getconf _NPROCESSORS_ONLN)
```

To install the built binary:

```bash
sudo cmake --install build-rel
```
