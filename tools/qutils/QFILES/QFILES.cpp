
#include "COMMON/CMDLIB.H"

#define	MAX_SOUNDS		1024
#define	MAX_MODELS		1024
#define	MAX_FILES		1024

#define	MAX_DATA_PATH	64


char		precache_sounds[MAX_SOUNDS][MAX_DATA_PATH];
int			precache_sounds_block[MAX_SOUNDS];
int			numsounds;

char		precache_models[MAX_MODELS][MAX_DATA_PATH];
int			precache_models_block[MAX_SOUNDS];
extern int	nummodels;

char		precache_files[MAX_FILES][MAX_DATA_PATH];
int			precache_files_block[MAX_SOUNDS];
int			numfiles;


typedef struct
{
	char	name[56];
	int		filepos, filelen;
} packfile_t;

typedef struct
{
	char	id[4];
	int		dirofs;
	int		dirlen;
} packheader_t;

packfile_t	pfiles[4096], *pf;
FILE		*packhandle;
int			packbytes;


/*
===========
PackFile

Copy a file into the pak file
===========
*/
void PackFile (char *src, char *name)
{
	FILE	*in;
	int		remaining, count;
	char	buf[4096];
	
	if ( (byte *)pf - (byte *)pfiles > sizeof(pfiles) )
		Error ("Too many files in pak file");
	
	in = SafeOpenRead (src);
	remaining = filelength (in);

	pf->filepos = LittleLong (ftell (packhandle));
	pf->filelen = LittleLong (remaining);
	strcpy (pf->name, name);
	printf ("%64s : %7i\n", pf->name, remaining);

	packbytes += remaining;
	
	while (remaining)
	{
		if (remaining < sizeof(buf))
			count = remaining;
		else
			count = sizeof(buf);
		SafeRead (in, buf, count);
		SafeWrite (packhandle, buf, count);
		remaining -= count;
	}

	fclose (in);
	pf++;
}


/*
===========
CopyQFiles
===========
*/
void CopyQFiles(const char* gamedir, const char* pakfile)
{
	int		i, p;
	char	srcfile[1024];
	char	destfile[1024];
	char	name[1024];
	packheader_t	header;
	int		dirlen;
	unsigned short		crc;

	// create a pak file
	pf = pfiles;

	snprintf (destfile, sizeof(destfile), "%s", pakfile);
	packhandle = SafeOpenWrite (destfile);
	SafeWrite (packhandle, &header, sizeof(header));	

	for (i=0 ; i<numsounds ; i++)
	{
		snprintf (name, sizeof(name), "sound/%s", precache_sounds[i]);
		snprintf (srcfile, sizeof(srcfile), "%s/sound/%s", gamedir, precache_sounds[i]);
		PackFile (srcfile, name);
	}
	for (i=0 ; i<nummodels ; i++)
	{
		sprintf (srcfile,"%s/%s",gamedir, precache_models[i]);
		PackFile (srcfile, precache_models[i]);
	}
	for (i=0 ; i<numfiles ; i++)
	{
		sprintf (srcfile,"%s/%s",gamedir, precache_files[i]);
		PackFile (srcfile, precache_files[i]);
	}
	
	header.id[0] = 'P';
	header.id[1] = 'A';
	header.id[2] = 'C';
	header.id[3] = 'K';
	dirlen = (byte *)pf - (byte *)pfiles;
	header.dirofs = LittleLong(ftell (packhandle));
	header.dirlen = LittleLong(dirlen);
	
	SafeWrite (packhandle, pfiles, dirlen);

	fseek (packhandle, 0, SEEK_SET);
	SafeWrite (packhandle, &header, sizeof(header));
	fclose (packhandle);	

// do a crc of the file
	CRC_Init (&crc);
	for (i=0 ; i<dirlen ; i++)
		CRC_ProcessByte (&crc, ((byte *)pfiles)[i]);

	i = pf - pfiles;
	printf ("%i files packed in %i bytes (%i crc)\n",i, packbytes, crc);
}


/*
=================
BspModels

Runs qbsp and light on all of the models with a .bsp extension
=================
*/
void BspModels (void)
{
	int		p;
	int		i;
	char	*m;
	char	cmd[1024];
	char	name[256];

	for (i=0 ; i<nummodels ; i++)
	{
		m = precache_models[i];
		if (strcmp(m+strlen(m)-4, ".bsp"))
			continue;
		strcpy (name, m);
		name[strlen(m)-4] = 0;

		sprintf (cmd, "qbsp %s%s",gamedir, name);
		system (cmd);
		sprintf (cmd, "light -extra %s%s", gamedir, name);
		system (cmd);
	}
}

/*
=============
ReadFiles
=============
*/
int ReadFiles (void)
{
	FILE	*f;
	int		i;

	char wd[1024] = {};
	char filepath[1024] = {};

	int p = CheckParm("-pak");

	if (p != 0)
	{
		char* path = myargv[p+1];
		int len = strlen(path) - 1;

		if (path[len] == '\\' || path[len] == '/')
			path[len] = '\0';

		snprintf(filepath, sizeof(filepath), "%s/files.dat", myargv[p+1]);
	}

	f = (p > 0) ? SafeOpenRead(filepath) : SafeOpenRead ("files.dat");

	fscanf (f, "%i\n", &numsounds);
	for (i=0 ; i<numsounds ; i++)
		fscanf (f, "%i %s\n", &precache_sounds_block[i],
			precache_sounds[i]);

	fscanf (f, "%i\n", &nummodels);
	for (i=0 ; i<nummodels ; i++)
		fscanf (f, "%i %s\n", &precache_models_block[i],
			precache_models[i]);

	fscanf (f, "%i\n", &numfiles);
	for (i=0 ; i<numfiles ; i++)
		fscanf (f, "%i %s\n", &precache_files_block[i],
			precache_files[i]);

	fclose (f);

	printf ("%3i sounds\n", numsounds);
	printf ("%3i models\n", nummodels);
	printf ("%3i files\n", numfiles);

	return (numsounds + nummodels + numfiles);
}


/*
=============
main
=============
*/
int main (int argc, char **argv)
{
	myargc = argc;
	myargv = argv;

	if (argc == 1)
	{
		printf ("qfiles -pak <0 / 1> : build a .pak file\n");
		printf ("qfiles -bspmodels : regenerates all brush models\n");
		exit (1);
	}

	SetQdirFromPath ("");

	ReadFiles ();

	const char* gamepath = myargv[CheckParm("-pak")+1];
	const char* outpath = myargv[CheckParm("-pak")+2];

	if (!strcmp (argv[1], "-pak"))
	{
		CopyQFiles(gamepath, outpath);
	}
	else if (!strcmp (argv[1], "-bspmodels"))
	{
		BspModels ();
	}
	else
		Error ("unknown command: %s", argv[1]);

	return 0;
}

