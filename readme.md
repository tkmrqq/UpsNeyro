\## Build Dependencies



\### Windows

1\. vcpkg: https://github.com/microsoft/vcpkg

2\. `vcpkg install ffmpeg:x64-windows`

3\. При запуске cmake: `-DCMAKE\_TOOLCHAIN\_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`



\### Linux

`sudo pacman -S ffmpeg` или `sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev`



