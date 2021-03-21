/******************************************************************************
    Copyright (C) 2019-2020 by Dillon Pentz <dillon@vodbox.io>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "importers.hpp"

std::vector<std::unique_ptr<Importer>> importers;

void ImportersInit()
{
	importers.clear();
	importers.push_back(std::make_unique<StudioImporter>());
	importers.push_back(std::make_unique<ClassicImporter>());
	importers.push_back(std::make_unique<SLImporter>());
	importers.push_back(std::make_unique<XSplitImporter>());
}

int ImportSCFromProg(const std::string &path, std::string &name,
		     const std::string &program, json11::Json &res)
{
	if (!os_file_exists(path.c_str())) {
		return IMPORTER_FILE_NOT_FOUND;
	}

	for (size_t i = 0; i < importers.size(); i++) {
		if (program == importers[i]->Prog()) {
			return importers[i]->ImportScenes(path, name, res);
		}
	}

	return IMPORTER_UNKNOWN_ERROR;
}

int ImportSC(const std::string &path, std::string &name, json11::Json &res)
{
	if (!os_file_exists(path.c_str())) {
		return IMPORTER_FILE_NOT_FOUND;
	}

	std::string prog = DetectProgram(path);

	if (prog == "Null") {
		return IMPORTER_FILE_NOT_RECOGNISED;
	}

	return ImportSCFromProg(path, name, prog, res);
}

std::string DetectProgram(const std::string &path)
{
	if (!os_file_exists(path.c_str())) {
		return "Null";
	}

	for (size_t i = 0; i < importers.size(); i++) {
		if (importers[i]->Check(path)) {
			return importers[i]->Prog();
		}
	}

	return "Null";
}

std::string GetSCName(const std::string &path, const std::string &prog)
{
	for (size_t i = 0; i < importers.size(); i++) {
		if (importers[i]->Prog() == prog) {
			return importers[i]->Name(path);
		}
	}

	return "Null";
}

OBSImporterFiles ImportersFindFiles()
{
	OBSImporterFiles f;

	for (size_t i = 0; i < importers.size(); i++) {
		OBSImporterFiles f2 = importers[i]->FindFiles();
		if (f2.size() != 0) {
			f.insert(f.end(), f2.begin(), f2.end());
		}
	}

	return f;
}
