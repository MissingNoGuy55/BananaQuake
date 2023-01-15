7z\7za a x64.7z ./gl_release_64/*.exe ./gl_release_64/*.dll -r
7z\7za a x86.7z ./gl_release/*.exe ./gl_release/*.dll -r
7z\7za a x86.7z ./release/*.exe ./release/*.dll -r

7z\7za a BananaQuake_bin.7z ./x86.7z
7z\7za a BananaQuake_bin.7z ./x64.7z