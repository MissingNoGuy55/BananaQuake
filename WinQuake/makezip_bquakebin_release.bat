7z\7za a x64.7z ./release_gl_64/*.exe ./release_gl_64/*.dll -r
7z\7za a x86.7z ./release_gl/*.exe ./release_gl/*.dll -r
7z\7za a x86.7z ./release/*.exe ./release/*.dll -r

7z\7za a BananaQuake_bin_release.7z ./x86.7z
7z\7za a BananaQuake_bin_release.7z ./x64.7z