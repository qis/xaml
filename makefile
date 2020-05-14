config = debug
target = xaml

all: build/windows/$(config)/rules.ninja
	@cmake --build build/windows/$(config)

run: build/windows/$(config)/rules.ninja
	@cmake --build build/windows/$(config)
	@cmake -E chdir build/windows/$(config) ./$(target)

install: build/windows/release/rules.ninja
	@cmake --build build/windows/release --target install

package: build/windows/release/rules.ninja
	@cmake --build build/windows/release --target package

format:
	@cmake -P res/format.cmake

clean:
	@cmake -E remove_directory build/windows

build/windows/debug/rules.ninja: CMakeLists.txt
	@cmake -GNinja -DCMAKE_BUILD_TYPE=Debug \
	  -DCMAKE_INSTALL_PREFIX="$(MAKEDIR)\build\install" \
	  -DCMAKE_TOOLCHAIN_FILE="$(MAKEDIR)\res\toolchain.cmake" \
	  -B build/windows/debug

build/windows/release/rules.ninja: CMakeLists.txt
	@cmake -GNinja -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_INSTALL_PREFIX="$(MAKEDIR)\build\install" \
	  -DCMAKE_TOOLCHAIN_FILE="$(MAKEDIR)\res\toolchain.cmake" \
	  -B build/windows/release
