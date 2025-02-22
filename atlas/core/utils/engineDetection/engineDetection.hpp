//
// Created by kj16609 on 4/1/23.
//

#ifndef ATLAS_ENGINEDETECTION_HPP
#define ATLAS_ENGINEDETECTION_HPP

#include <filesystem>
#include <vector>

#include <QString>

#include "core/utils/FileScanner.hpp"

enum Engine : int
{
	ENGINES_BEGIN = 0,
	RenPy, // #22
	Unity, // #23
	Unreal,
	RPGM, // #24
	WolfRPG,
	VisualNovelMaker,
	TyanoBuilder,
	Java,
	Flash,
	RAGS,
	KiriKiri,
	NScripter,
	NVList,
	Sukai2,
	HTML,
	ENGINES_END,
	UNKNOWN
};

//! Function to be specialized for each Engine to return true if the engine is valid.
template < Engine engine >
bool isEngineT( FileScanner& scanner );

//! String name of the engine.
template < Engine engine >
QString engineNameT();

//std::vector<std::filesystem::path> createFileList(const std::filesystem::path& path);

//! Returns an engine type of ENGINES_END if no engine is determined
Engine determineEngine( FileScanner& scanner );

//! Returns a string name of the engine
QString engineName( const Engine engine );

std::vector< std::filesystem::path > detectExecutables( FileScanner& scanner );

std::vector< std::filesystem::path >
	scoreExecutables( std::vector< std::filesystem::path > paths, const Engine engine = UNKNOWN );

#endif //ATLAS_ENGINEDETECTION_HPP
