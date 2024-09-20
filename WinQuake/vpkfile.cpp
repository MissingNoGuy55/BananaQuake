#include "quakedef.h"
#include "vpkfile.h"

//============================================================================
// Missi: VPK LIBRARY
// 
// Rudimentary library to load and scan VPKs for filenames, offsets, etc.
// 
// FUNCTIONS:
// 
// ReadVPKChar: scans a character from an std::ifstream file stream
// ReadVPKString: scans a string from an std::ifstream file stream
// GetVPKHeader: allocates a VPKHeader_v2 structure for integrity checking
// PopulateVPKData: allocates a vpk_t structure for use in Quake's search path struct
// OpenAllVPKDependencies: opens all sub-VPKs of a multi-chunk VPK or just a standalone VPK in the case of non-chunked
// FindVPKFile: searches the specified VPKs for the specified filename and returns a VPKDirectoryEntry struct containing the info
// FindVPKFileAmongstLoadedVPKs: searches all loaded VPKs for a file. VERY SLOW; use sparingly
// 
// FindVPKIndexForFileAmongstLoadedVPKs: searches for the specified filename amongst all loaded VPKs, but only returns the archive
// index. This is a deprecated function, do not use it.
// 
// CloseAllVPKs: closes all file handles and frees memory from search paths if any have VPKs in them
// 
// WARNING: PopulateVPKData allocates a vpk_t structure for use in COM_AddGameDirectory,
// but the members "entries" and "filenames" are malloc'd. It is the users' responsibility
// to free this memory when they are done with it. (9/14/2024)
// 
//============================================================================

cxxifstream* loaded_vpks[512][256];
cxxstring* loaded_vpk_names[512][256];

static constexpr int VPK_VERSION = 2;

static byte ReadVPKChar(cxxifstream* file)
{
	byte c = '\0';

	// Missi: NOTE: we want to use read here, because the root directory of VPKs is specified as
	// " ", and ifstream::get will skip whitespace and not pick up any files in the root dir
	// of the VPK (9/14/2024)
	file->read((char*)&c, 1);

	return c;
}

static void ReadVPKString(cxxifstream* file, char* out)
{
	int pos = 0;
	while (1)
	{
		out[pos] = ReadVPKChar(file);

		if (!out[pos++])
			break;
	}
}

static char vpk_header[sizeof(VPKHeader_v2)];

VPKHeader_v2* GetVPKHeader(cxxifstream* file)
{
	memset(vpk_header, 0, sizeof(VPKHeader_v2));

	VPKHeader_v2* header = nullptr;

	file->read(vpk_header, sizeof(VPKHeader_v2));
	file->seekg(0, file->beg);

	header = (VPKHeader_v2*)vpk_header;

	return header;
}

static char fileread_data[512];
static CQVector<VPKDirectoryEntry*> fileread_entries = {};
static CQVector<const char*> fileread_names = {};

vpk_t* PopulateVPKData(cxxifstream* file)
{
	char data[sizeof(VPKHeader_v2)];
	file->clear();
	file->read(data, sizeof(data));

	VPKHeader_v2* header = (VPKHeader_v2*)data;
	vpk_t* vpk_data = nullptr;

	int numextensions = 0;
	int numentries = 0;
	int numdirs = 0;

	VPKDirectoryEntry* entry = new VPKDirectoryEntry;
	memset(entry, 0, sizeof(*entry));
	memset(fileread_data, 0, sizeof(fileread_data));

	while (1)
	{
		file->clear();
		char c1[512];

		ReadVPKString(file, c1);

		if (!c1[0])
			break;

		numextensions++;

		while (1)
		{
			file->clear();
			char c2[512];

			ReadVPKString(file, c2);

			if (c2[0] != ' ' && !c2[0])
				break;

			numdirs++;

			while (1)
			{
				file->clear();
				char c3[512];
				ReadVPKString(file, c3);

				if (!c3[0])
					break;

				cxxstring c4(c2);

				if (c4[0] != ' ')
				{
					c4.append("/");
					c4.append(c3);
					c4.append(".");
					c4.append(c1);
				}
				else
				{
					c4.erase(c4.find(" "), 1);
					c4.append(c3);
					c4.append(".");
					c4.append(c1);
				}

				file->clear();
				file->read(fileread_data, sizeof(VPKDirectoryEntry) - sizeof(unsigned short));

				memcpy(entry, fileread_data, sizeof(VPKDirectoryEntry));

				char** filename = new char*;
				*filename = new char[c4.length() + 2];

				memset(*filename, 0, c4.length() + 2);

				memcpy(*filename, c4.c_str(), c4.length() + 2);

				fileread_entries.AddToEnd(&entry);
				fileread_names.AddToEnd(filename);

				numentries++;
			}
		}
	}

	vpk_data = new vpk_t;

	vpk_data->entries = static_cast<VPKDirectoryEntry**>(malloc(fileread_entries.GetSize()));

	if (!vpk_data->entries)
	{
		Sys_Error("PopulateVPKData: failed to allocate memory for VPK entries\n");
		return nullptr;
	}

	vpk_data->filenames = static_cast<const char**>(malloc(fileread_names.GetSize()));

	if (!vpk_data->filenames)
	{
		Sys_Error("PopulateVPKData: failed to allocate memory for VPK file names\n");
		return nullptr;
	}

	vpk_data->numdirs = numdirs;
	vpk_data->numextensions = numextensions;
	vpk_data->numfiles = numentries;

	memcpy(vpk_data->entries, fileread_entries.GetBase(), fileread_entries.GetSize());
	memcpy(vpk_data->filenames, fileread_names.GetBase(), fileread_names.GetSize());

	memcpy(&vpk_data->header, header, sizeof(VPKHeader_v2));

	fileread_names.Clear();
	fileread_entries.Clear();

	file->seekg(0, cxxifstream::beg);
	file->clear();
	return vpk_data;
}

int OpenAllVPKDependencies(cxxstring filename)
{
	int high = 0;
	char append[512];
	int vpk_assignment = 0;
	static int j = 0;

	loaded_vpks[j][0] = new cxxifstream;
	loaded_vpks[j][0]->close();
	loaded_vpks[j][0]->open(filename.c_str(), cxxifstream::binary);

	VPKHeader_v2* header = GetVPKHeader(loaded_vpks[j][0]);

	if (header->Version != VPK_VERSION)
	{
		Sys_Error("VPK version of %s is not version %d!\n", filename.c_str(), VPK_VERSION);
		return -1;
	}

	loaded_vpk_names[j][0] = new cxxstring(filename);

	// Missi: bail out here if there is no "_dir" in the pathname, as it is not a
	// multi-chunk VPK (9/14/2024)
	if (filename.find("_dir") == cxxstring::npos)
		return -1;

	for (int i = 1; i < SUB_VPKS_SIZE; i++)
	{
		cxxstring fname;
		fname.assign(filename);
		fname.erase(fname.find("_dir"));

		snprintf(append, sizeof(append), "%s_%03d.vpk", fname.c_str(), i - 1);

		loaded_vpks[j][i] = new cxxifstream;
		loaded_vpks[j][i]->close();
		loaded_vpks[j][i]->open(append, cxxifstream::binary);

		if (!loaded_vpks[j][i]->is_open())
			break;

		loaded_vpk_names[j][i] = new cxxstring(append);

		high++;
	}
	j++;

	return high;
}

const VPKDirectoryEntry* FindVPKFile(cxxifstream* file, const char* filename)
{
	char data[sizeof(VPKHeader_v2)];
	file->clear();
	file->read(data, sizeof(data));

	memset(fileread_data, 0, sizeof(fileread_data));

	// Missi: this mess is based on pseudocode from https://developer.valvesoftware.com/wiki/VPK_(file_format)#Tree
	// but it works fine. We need to scan VPKs like this anyway because of the format it follows (file extension, file dir, then file name) (9/14/2024)
	while (1)
	{
		file->clear();
		char c1[512];

		ReadVPKString(file, c1);

		if (!c1[0])
			break;

		while (1)
		{
			file->clear();
			char c2[512];

			ReadVPKString(file, c2);

			if (!c2[0])
				break;

			while (1)
			{
				file->clear();
				char c3[512];
				ReadVPKString(file, c3);

				if (!c3[0])
					break;

				cxxstring c4(c2);
				if (c4[0] != ' ')
					c4.append("/");
				else
					c4.erase(c4.find(" "), 1);

				c4.append(c3);
				c4.append(".");
				c4.append(c1);

				file->clear();
				file->read(fileread_data, sizeof(VPKDirectoryEntry) - sizeof(unsigned short));

				const VPKDirectoryEntry* entry = (VPKDirectoryEntry*)fileread_data;

				if (!Q_strcmp(c4.c_str(), filename))
				{
					file->seekg(0, cxxifstream::beg);
					file->clear();
					return entry;
				}
			}
		}
	}

	file->seekg(0, cxxifstream::beg);
	file->clear();
	return nullptr;
}

const VPKDirectoryEntry* FindVPKFileAmongstLoadedVPKs(const char* filename)
{
	const VPKDirectoryEntry* result = nullptr;

	for (int i = 0; i < TOTAL_VPKS_SIZE; i++)
	{
		if (!loaded_vpk_names[i][0])
			break;

		result = FindVPKFile(loaded_vpks[i][0], filename);
			
		if (result)
			return result;
	}

	delete[] result;

	return nullptr;
}

int FindVPKIndexForFileAmongstLoadedVPKs(const char* filename)
{
	const VPKDirectoryEntry* result = nullptr;

	for (int i = 0; i < TOTAL_VPKS_SIZE; i++)
	{
		if (!loaded_vpk_names[i][0])
			break;

		Con_DPrintf("Searching for \"%s\" in \"%s\"\n", filename, loaded_vpk_names[i][0]->c_str());

		result = FindVPKFile(loaded_vpks[i][0], filename);

		if (result)
		{
			return i;
		}
	}

	return -1;
}

/*void LoadAllVPKs()
{
	char combined[256];

	snprintf(combined, sizeof(combined), "%s/vpk", g_Common->com_gamedir);

	cxxpath p = combined;
	
	if (!fs::exists(p) || fs::is_empty(p))
		return;

	for (const auto& directory : fs::directory_iterator(p))
	{
		if (!directory.is_regular_file())
			continue;

		cxxstring str = directory.path().string();

		if (str.empty())
			continue;

		if (str.find("_dir.vpk") != cxxstring::npos)
		{
			if (str.find("vpk") == cxxstring::npos)
				continue;

			str.erase(0, str.find("vpk"));

			if (str.find("\\") != cxxstring::npos)
				str.replace(str.find("\\"), 1, "/");

			int num = OpenAllVPKDependencies(str);
		}
	}
}*/

/*
==================
CloseAllVPKs

Closes all VPK files that are currently loaded. Only ran at host shutdown.
==================
*/
void CloseAllVPKs()
{
	for (int i = 0; i < TOTAL_VPKS_SIZE; i++)
	{
		if (!loaded_vpks[i][0])
			break;

		loaded_vpks[i][0]->close();

		for (int j = 0; j < SUB_VPKS_SIZE; j++)
		{
			if (!loaded_vpks[i][j])
				break;

			loaded_vpks[i][j]->close();
			delete loaded_vpks[i][j];
		}
	}

	g_Common->COM_CloseVPKs();
}
