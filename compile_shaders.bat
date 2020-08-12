echo off
mkdir build
for %%x in (simple.vert simple.frag outline.vert outline.frag) do tools\glslangValidator.exe -V res\shaders\%%x -o build\%%x.spv"
pause