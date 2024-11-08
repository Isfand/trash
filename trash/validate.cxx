#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <print>
#include <algorithm>

#include "validate.hxx"
#include "common/database.hxx"
#include "validate_args.hxx"
#include "common/globals.hxx"
#include "common/utils.hxx"

namespace trash{

Validate::Validate(ValidateOptions& vOpt) : m_vOpt{vOpt}  
{
#ifndef NDEBUG
	std::println("verboseOption is:     {}", m_vOpt.verboseOption);
	std::println("forceOption is:       {}", m_vOpt.forceOption);
	std::println("silentOption is:      {}", m_vOpt.silentOption);
	std::println("fileOption is:        {}", m_vOpt.fileOption);
	std::println("dataOption is:        {}", m_vOpt.dataOption);
	std::println("wipeOption is:        {}", m_vOpt.wipeOption);
	std::println("fillDirSizeOption is: {}", m_vOpt.fillDirSizeOption);
#endif
	if(m_vOpt.fileOption)Validate::file();
	if(m_vOpt.dataOption)Validate::data();
	if(m_vOpt.wipeOption)Validate::wipe();
	if(m_vOpt.fillDirSizeOption)Validate::fillDirectorySize();
}

void Validate::allFile()
{
	file();
	data();
	wipe();
}

void Validate::file()
{
	// check if the trashFile is referenced inside data/ with any record.
	// Move to wipe/
	// Finally delete it.

	std::vector<std::string> danglingFiles{};

	for (const auto& entry : std::filesystem::directory_iterator(singleton->getWorkingTrashFileDir()))
	{    
		std::string trashFile = entry.path().filename().string();
		std::string dbTrashFileRecord = m_db.selectData(std::format("SELECT file FROM trash WHERE file='{}';", trashFile));

		if(trashFile != dbTrashFileRecord)
			danglingFiles.push_back(trashFile);
	}

	if(danglingFiles.size() == 0)
	{
		if(m_vOpt.verboseOption)
			std::println("no dangling file(s) found inside file/");
	}
	else if(danglingFiles.size() > 0)
	{
		bool confirmFlag{};
		
		if(m_vOpt.verboseOption)
		{
			std::println("dangling file(s) found in file/:");
			for(const auto& file : danglingFiles)
				std::println("{}", file);
		}
		if(!confirmFlag && !m_vOpt.forceOption && !m_vOpt.silentOption)
		{
			std::string confirm;
			std::println("permanently remove dangling files? [Y/n]"); 
			do std::getline(std::cin, confirm); 
			while(confirm != "Y" && confirm != "n");
			confirm == "Y" ? confirmFlag=true : confirmFlag=false;
		}
		if(confirmFlag || m_vOpt.forceOption || m_vOpt.silentOption)
		{
			for(const auto& file : danglingFiles)
			{
				//WARNING: If a file with the same name exists in wipe/ it will be overwritten. file/ only contains unique names but files in wipe/ are not checked.
				std::filesystem::rename(singleton->getWorkingTrashFileDir() + file, singleton->getWorkingTrashWipeDir() + file);          
				std::filesystem::remove_all(singleton->getWorkingTrashWipeDir() + file);
			}
		}  
	}

}

void Validate::data()
{
	// Should check for:
	// records points to a trashFile in /file
	// Then delete

	std::vector<std::string> danglingRecords { Database().selectDataB("SELECT file from trash;") };

	std::vector<std::string> trashFiles{};
	for (const auto& entry : std::filesystem::directory_iterator(singleton->getWorkingTrashFileDir()))
		trashFiles.push_back(entry.path().filename().string());

	//pop back dbTrashFiles with whatever is inside trashFiles.
	for (const auto& element : trashFiles)
		danglingRecords.erase(std::remove(danglingRecords.begin(), danglingRecords.end(), element), danglingRecords.end());

	if(danglingRecords.size() == 0)
	{
		if(m_vOpt.verboseOption)
			std::println("no dangling record(s) found inside data/");    
	}
	else if(danglingRecords.size() > 0)
	{
		bool confirmFlag{};
		
		if(m_vOpt.verboseOption)
		{
			std::println("Dangling record(s) found:");
			for (const auto& danglingRecord : danglingRecords)
				std::println("{}", danglingRecord);
		}
		if(!confirmFlag && !m_vOpt.forceOption && !m_vOpt.silentOption)
		{
			std::string confirm;
			std::println("permanently remove dangling record(s)? [Y/n]"); 
			do std::getline(std::cin, confirm); 
			while(confirm != "Y" && confirm != "n");
			confirm == "Y" ? confirmFlag=true : confirmFlag=false;
		}
		if(confirmFlag || m_vOpt.forceOption || m_vOpt.silentOption)
		{
			for (const auto& danglingRecord : danglingRecords)
				Database().executeSQL(std::format("DELETE FROM trash WHERE file='{0}';", danglingRecord));
		}
	}

}

void Validate::wipe()
{
	//Check for existence of files first. Display them. Then delete...
	std::vector<std::filesystem::path> danglingFiles{};

	for (const auto& entry : std::filesystem::directory_iterator(singleton->getWorkingTrashWipeDir()))
		danglingFiles.push_back(entry);

	if (danglingFiles.size() == 0)
	{
		if(m_vOpt.verboseOption)
			std::println("no dangling file(s) found inside wipe/");    
	}
	else if(danglingFiles.size() > 0)
	{   
		bool confirmFlag{};
		
		if(m_vOpt.verboseOption)
		{
			std::println("dangling file(s) found in wipe/:");
			for(const auto& entry : danglingFiles)
				std::println("{}", entry.filename().string());
		}
		if(!confirmFlag && !m_vOpt.forceOption && !m_vOpt.silentOption)
		{
			std::string confirm;
			std::println("permanently remove dangling files? [Y/n]"); 
			do std::getline(std::cin, confirm); 
			while(confirm != "Y" && confirm != "n");
			confirm == "Y" ? confirmFlag=true : confirmFlag=false;
		}
		if(confirmFlag || m_vOpt.forceOption || m_vOpt.silentOption)
		{
			for (const auto& entry : std::filesystem::directory_iterator(singleton->getWorkingTrashWipeDir()))
				std::filesystem::remove_all(entry.path());
		} 
	}

} 

void Validate::fillDirectorySize()
{
	//Store NULL directories into vector
	//Optional: Check directory is in-fact a directory again. If not then remove from vector.
	//iterate directory and save all files excluding directories into a new vector list like in List::size().
	//Remove dupe ino,dev from new vector.
	//Stat all files and append them onto a long long
	//Update the directory size in the database using executeSQL().

	std::vector<std::string> nullDirectoriesQuery { Database().selectDataB("SELECT file from trash WHERE filetype='directory' AND size='NULL';") };
	
	//Append file/ path to each filename
	std::transform(nullDirectoriesQuery.begin(), 
				   nullDirectoriesQuery.end(), 
				   nullDirectoriesQuery.begin(), 
				   [](std::string& f) 
				   { return singleton->getWorkingTrashFileDir() + f; });

	for(auto& directoryPathString : nullDirectoriesQuery)
	{
		//Check for read perms on directory, which is needed in order to iterate through it.
		if (!canReadDirChk(std::filesystem::directory_entry(directoryPathString)) &&
			Verity(std::filesystem::directory_entry(directoryPathString)).type == 
			std::filesystem::file_type::directory) 
		{
			if(!m_vOpt.silentOption)
				std::println("cannot save directory size, process execution user {} is missing read permissions for directory {}", 
				singleton->getWorkingUsername(), directoryPathString);
			continue;
		}

		auto directory_entry = std::filesystem::directory_entry(directoryPathString);
		unsigned long long size { directorySize(directory_entry) };
		Database().executeSQL(std::format("UPDATE trash SET size='{}' WHERE file='{}';", size, directory_entry.path().filename().string()));
	}
}

}//namespace trash
