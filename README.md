
## Build

```shell
cmake -DCMAKE_BUILD_TYPE=Debug -B ./cmake-build-debug/ -G Ninja -S .

cmake --build ./cmake-build-debug/

# build only one target (clox, testRunner)
cmake --build ./cmake-build-debug/ --target clox

# clean
cmake --build ./cmake-build-debug/ --target clean

# run
./cmake-build-debug/clox
```

```shell
cmake -DCMAKE_BUILD_TYPE=Release -B ./cmake-build-release/ -G Ninja -S .

cmake --build ./cmake-build-release/

# run
./cmake-build-release/clox
```
