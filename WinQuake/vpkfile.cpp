#include "quakedef.h"
#include "vpkfile.h"

cxxifstream* loaded_vpks[512][256] = {};
cxxstring* loaded_vpk_names[512][256] = {};

static byte ReadVPKChar(cxxifstream* file)
{
	byte c = file->get();

	return c;
}

static void ReadVPKString(cxxifstream* file, char* out)
{
	int pos = 0;
	while (1)
	{
		out[pos] = ReadVPKChar(file);

		if (!out[pos])
			break;

		pos++;
	}
}

static VPKHeader_v2* GetVPKHeader(cxxifstream* file)
{
	VPKHeader_v2* header = nullptr;

	char* str = new char[sizeof(VPKHeader_v2)];

	file->read(str, sizeof(VPKHeader_v2));
	file->seekg(0, file->beg);

	header = (VPKHeader_v2*)str;

	return header;
}

static int OpenAllVPKDependencies(cxxstring filename)
{
	int high = 0;

	if (filename.find("_dir") == cxxstring::npos)
	{
		Sys_Error("OpenAllVPKDependencies: no _dir in filename parameter!\n");
	}

	char append[512] = {};

	int vpk_assignment = 0;
	static int j = 0;

	loaded_vpks[j][0] = new cxxifstream;
	loaded_vpks[j][0]->close();
	int size = g_Common->COM_FOpenFile_IFStream(filename.c_str(), loaded_vpks[j][0], nullptr);

	Con_PrintColor(TEXT_COLOR_GREEN, "Added VPK file %s/%s (%d bytes)\n", g_Common->com_gamedir, filename.c_str(), size);

	loaded_vpk_names[j][0] = new cxxstring(filename);

	for (int i = 1; i < SUB_VPKS_SIZE; i++)
	{
		cxxstring fname;
		fname.assign(filename);
		fname.erase(fname.find("_dir"));

		snprintf(append, sizeof(append), "%s_%03d.vpk", fname.c_str(), i - 1);

		loaded_vpks[j][i] = new cxxifstream;
		loaded_vpks[j][i]->close();
		g_Common->COM_FOpenFile_IFStream(append, loaded_vpks[j][i], nullptr);

		if (!loaded_vpks[j][i]->is_open())
			break;

		Con_PrintColor(TEXT_COLOR_GREEN, "Added sub VPK file %s/%s\n", g_Common->com_gamedir, append);

		loaded_vpk_names[j][i] = new cxxstring(append);

		high++;
	}
	j++;

	return high;
}

static char fileread_data[512] = {};

const VPKDirectoryEntry* FindVPKFile(cxxifstream* file, const char* filename)
{
	char data[sizeof(VPKHeader_v2)] = {};
	file->clear();
	file->read(data, sizeof(data));

	memset(fileread_data, 0, sizeof(fileread_data));

	while (1)
	{
		file->clear();
		char c1[512] = {};

		ReadVPKString(file, c1);

		if (!c1[0])
			break;

		while (1)
		{
			file->clear();
			char c2[512] = {};

			ReadVPKString(file, c2);

			if (!c2[0])
				break;

			while (1)
			{
				file->clear();
				char c3[512] = {};
				ReadVPKString(file, c3);

				if (!c3[0])
					break;

				cxxstring c4(c2);
				c4.append("/");
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

void LoadAllVPKs()
{
	char combined[256] = {};

	snprintf(combined, sizeof(combined), "%s/vpks", g_Common->com_gamedir);

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
			if (str.find("vpks") == cxxstring::npos)
				continue;

			str.erase(0, str.find("vpks"));

			if (str.find("\\") != cxxstring::npos)
				str.replace(str.find("\\"), 1, "/");

			int num = OpenAllVPKDependencies(str);
		}
	}
}

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

		for (int j = 0; j < SUB_VPKS_SIZE; j++)
		{
			if (!loaded_vpks[i][j])
				break;

			loaded_vpks[i][j]->close();
		}

		loaded_vpks[i][0]->close();
	}
}
