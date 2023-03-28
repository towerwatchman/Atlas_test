//
// Created by kj16609 on 1/12/23.
//

#ifndef HYDRUS95_CONFIG_HPP
#define HYDRUS95_CONFIG_HPP

#include <filesystem>

#include <QSettings>
#include <QVariant>

#include "h95/logging.hpp"

/**
 *
 * @page H95Settings Settings list
 *
 * @warning THESE SHOULD NEVER BE MODIFIED MANUALLY IN `config.ini` UNLESS TOLD TOO. These are simply to provide some reference to what they are when developing new modules
 *
 * |Group 		| Key				| Value 	| Default	|
 * |------------|-------------------|-----------|-----------|
 * |			| first_launch 		| boolean 	|  true 	|
 * |			| version_number 	| int 		| 100 		|
 * | main_view	| item_width 		| int 		| 400 		|
 * | main_view	| item_height 		| int 		| 300		|
 * | main_view	| banner_width 		| int 		| 400		|
 * | main_view	| banner_height 	| int 		| 280		|
 * | main_view	| padding			| int		| 4			|
 * | paths 		| data 				| string 	| ./data/   |
 * | paths		| games				| string	| ./data/games	 |
 * | paths		| images			| string	| ./data/images |
 */

//TODO: Add cache
inline QSettings getSettingsObject()
{
	return { "./data/config.ini", QSettings::IniFormat };
}

/**
 * @throws std::runtime_error when setting T not default constructable and no setting given
 * @tparam T type for setting
 * @param setting_name
 * @return
 */
//! Returns T for the given setting_name
template < typename T >
inline T getSettings( const QString setting_name )
{
	QSettings settings { getSettingsObject() };
	const auto variant { settings.value( setting_name ) };
	if ( variant.template canConvert< T >() )
		return variant.template value< T >();
	else
	{
		spdlog::warn( "Setting for {} was not populated!", setting_name );

		if constexpr ( std::is_default_constructible_v< T > )
			return {};
		else
			throw std::runtime_error(
				"T was not default constructable! Throwing instead! For given setting:" + setting_name.toStdString() );
	}
}

template < typename T >
void setSettings( const QString name, const T& value )
{
	getSettingsObject().setValue( name, value );
}

#define KEY_VALUE( group, name ) QString( #group ) + "/" + #name

#define SETTINGS_D( group, name, type, default_value )                                                                 \
  namespace config::group::name                                                                                        \
  {                                                                                                                    \
	inline type get()                                                                                                  \
	{                                                                                                                  \
	  return getSettingsObject().value( KEY_VALUE( group, name ) ).value< type >();                                    \
	}                                                                                                                  \
                                                                                                                       \
	inline void set( const type& val )                                                                                 \
	{                                                                                                                  \
	  getSettingsObject().setValue( KEY_VALUE( group, name ), val );                                                   \
	}                                                                                                                  \
                                                                                                                       \
	inline void setDefault()                                                                                           \
	{                                                                                                                  \
	  set( default_value );                                                                                            \
	}                                                                                                                  \
  }

#define SETTINGS_PATH( group, name, default_path )                                                                     \
  SETTINGS_D( group, name, QString, default_path )                                                                     \
  namespace config::group::name                                                                                        \
  {                                                                                                                    \
	inline std::filesystem::path getPath()                                                                             \
	{                                                                                                                  \
	  return { group::name::get().toStdString() };                                                                     \
	}                                                                                                                  \
                                                                                                                       \
	inline void setPath( const std::filesystem::path path )                                                            \
	{                                                                                                                  \
	  group::name::set( QString::fromStdString( path.string() ) );                                                     \
	}                                                                                                                  \
  }

SETTINGS_PATH( paths, database, "./data/hydrus95.db" )
SETTINGS_PATH( paths, images, "./data/images" )
SETTINGS_PATH( paths, games, "./data/games" )
SETTINGS_PATH( paths, theme, "./data/themes/default.qss" );

SETTINGS_D( importer, pathparse, QString, "{creator}/{title}/{version}" )
SETTINGS_D( importer, skipFilesize, bool, false )
SETTINGS_D( importer, searchGameInfo, bool, true )
SETTINGS_D( importer, downloadBanner, bool, false )
SETTINGS_D( importer, downloadVNDB, bool, false )
SETTINGS_D( importer, moveImported, bool, true )

SETTINGS_D( db, first_start, bool, true )
SETTINGS_D( logging, level, int, 2 )

#endif //HYDRUS95_CONFIG_HPP
