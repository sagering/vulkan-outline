echo off
mkdir build
for %%x in (pre.vert pre.frag post.vert post.frag) do tools\glslangValidator.exe -V res\shaders\%%x -o build\%%x.spv"
pause