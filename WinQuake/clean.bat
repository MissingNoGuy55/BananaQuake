rmdir /s /q debug
rmdir /s /q release
rmdir /s /q debugi386
rmdir /s /q debug_gl
rmdir /s /q debug_gl_64
rmdir /s /q release_gl
rmdir /s /q release_gl_64
rmdir /s /q x64

rmdir /s /q gas2masm\debug
rmdir /s /q gas2masm\release
rmdir /s /q gas2masm\x64

del gas2masm\gas2masm.opt
del gas2masm\gas2masm.plg
del gas2masm\gas2masm.ncb
del gas2masm\gas2masm.stt

del WinQuake.opt
del WinQuake.plg
del WinQuake.ncb
del WinQuake.stt

del /s *.ipch
del /s *.obj
pause
