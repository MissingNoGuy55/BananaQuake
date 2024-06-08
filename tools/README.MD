# QBSP, VIS, LIGHT, AND QCC RECOMPILED

These are the original qbsp, vis, light, and qcc programs recompiled for x64 with some slight adjustments for ease of use.

## USAGE NOTES

QBSP is extremely finicky. It expects a directory named "Quake" where the program is ran. I added functionality for a "-proj" command line option which can override where it looks. This option was referenced in the original tools but was absent in code. I did not add functionality for the Valve map format. Maybe in the future I will, but for now you will just have to make do, or use another toolset.

QCC has two different versions, the one on id's GitHub with John Carmack's readme dated 1999, and a slightly older version that I assume was pushed directly from an ancient source. I chose to prefer the older one because while the 1999 version does have QuakeC sources and .pak functionality, it is quite buggy and I could never get it to produce a genuine progs.dat that the game would accept.

## OTHER NOTES

These programs compared to modern tools are pretty much very primitive, buggy, and have pretty strict limits set on them. You're better off using something like EricW's tools for mapping, and something else for QC compiling.