# RemoteJoyLite SDL

Crossplatform RemoteJoyLite client using SDL3

## Controls
You can control your psp with gamepad

Keyboard 1/2/3 - switch window size (x1/x2/x3)

## Building

Requirements:
- SDL3 >= 3.2
- cmake >= 3.20

```
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build
```