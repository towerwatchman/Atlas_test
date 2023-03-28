#include "batchimportdialog.h"

#include <QFileDialog>
#include <QFutureWatcher>
#include <QMimeDatabase>
#include <QtConcurrentRun>

#include <tracy/Tracy.hpp>

#include "h95/config.hpp"
#include "h95/regex.hpp"
#include "h95/utils.hpp"
#include "ui/delegates/BatchImportDelegate.hpp"
#include "ui/models/BatchImportModel.hpp"
#include "ui_batchimportdialog.h"

batchImportDialog::batchImportDialog( QWidget* parent ) : QDialog( parent ), ui( new Ui::batchImportDialog )
{
	ui->setupUi( this );
	ui->progressBar->hide();

	ui->twGames->setModel( new BatchImportModel() );
	ui->twGames->setItemDelegate( new BatchImportDelegate() );


	connect( this, &batchImportDialog::startProcessingDirectory, &processor, &ImportProcessor::processDirectory );
	connect( &processor, &ImportProcessor::finishedDirectory, this, &batchImportDialog::processFinishedDirectory );
	connect( &processor, &ImportProcessor::finishedProcessing, this, &batchImportDialog::finishedProcessing );

	connect(
		this,
		&batchImportDialog::addToModel,
		dynamic_cast< BatchImportModel* >( ui->twGames->model() ),
		&BatchImportModel::addGame );


	loadConfig();
}

void batchImportDialog::loadConfig()
{
	ui->tbFormat->setText( config::importer::pathparse::get() );

	ui->cbCheckLocal->setChecked(config::importer::searchGameInfo::get());
	ui->cbSkipFilesize->setChecked(config::importer::skipFilesize::get());
	ui->cbDownloadBanners->setChecked(config::importer::downloadBanner::get());
	ui->cbDownloadVNDB->setChecked(config::importer::downloadVNDB::get());
	ui->cbMoveImported->setChecked(config::importer::moveImported::get());
}

void batchImportDialog::saveConfig()
{
	config::importer::pathparse::set( ui->tbFormat->text() );

	config::importer::searchGameInfo::set(ui->cbCheckLocal->isChecked());
	config::importer::skipFilesize::set(ui->cbSkipFilesize->isChecked());
	config::importer::downloadBanner::set(ui->cbDownloadBanners->isChecked());
	config::importer::downloadVNDB::set(ui->cbDownloadVNDB->isChecked());
	config::importer::moveImported::set(ui->cbMoveImported->isChecked());
}


batchImportDialog::~batchImportDialog()
{

	saveConfig();

	delete ui;
}

void batchImportDialog::on_btnSetFolder_pressed()
{
	auto directory { QFileDialog::getExistingDirectory( this, "Select folder to add" ) };

	if ( directory.isEmpty() || !QFile::exists( directory ) )
		return;
	else
		ui->tbPath->setText( std::move( directory ) );
}

void batchImportDialog::processFiles()
{
	ZoneScoped;
	/*
	 * base: /run/media/kj16609/HDDWIN/NSFWSorted/
	 * regex: {path}/{creator}/{title}/{version}
	 *
	 */

	auto stripEndSlash = []( const std::filesystem::path path ) -> std::filesystem::path
	{
		ZoneScoped;
		if ( const auto str = path.string(); str.ends_with( '/' ) || str.ends_with( '\\' ) )
			return { str.substr( 0, str.size() - 1 ) };
		else
			return path;
	};

	const std::filesystem::path base {
		stripEndSlash( std::filesystem::path( ui->tbPath->text().toStdString() ).make_preferred() )
	};

	const std::filesystem::path search {
		stripEndSlash( std::filesystem::path( ui->tbFormat->text().toStdString() ).make_preferred() )
	};

	const QString cleaned_regex { regexify( escapeStr( QString::fromStdString( ( base / search ).string() ) ) ) };

	spdlog::info( "Scanning {} for games", base );

	emit startProcessingDirectory( cleaned_regex, base, ui->cbMoveImported->isChecked(), ui->cbSkipFilesize->isChecked() );

	ui->twGames->resizeColumnsToContents();
}

void batchImportDialog::on_btnNext_pressed()
{
	//Verify that the path is set
	const auto& path { ui->tbPath->text() };
	if ( path.isEmpty() || !QFile::exists( path ) )
	{
		ui->statusLabel->setText( "Path not set or invalid. Please check" );
		return;
	}

	if ( ui->tbFormat->text().isEmpty() )
	{
		ui->statusLabel->setText( "Autofill not set. Please fill out the required entries" );
		return;
	}

	if ( !ui->tbFormat->text().contains( "{title}" ) )
	{
		ui->statusLabel->setText( "Autofill missing \"{title}\" which is required" );
		return;
	}

	ui->swImportGames->setCurrentIndex( 1 );
	ui->btnBack->setEnabled( true );
	ui->btnNext->setEnabled( false );

	processFiles();
}

void batchImportDialog::on_btnBack_pressed()
{
	//Clear the model
	dynamic_cast< BatchImportModel* >( ui->twGames->model() )->clearData();
}

void batchImportDialog::modelChanged(
	[[maybe_unused]] const QModelIndex& topLeft,
	[[maybe_unused]] const QModelIndex& bottomRight,
	[[maybe_unused]] const QList< int >& roles )
{
	ui->twGames->resizeColumnsToContents();
}

void batchImportDialog::processFinishedDirectory( const GameImportData game_data )
{
	emit addToModel( game_data );
	ui->twGames->resizeColumnsToContents();
	ui->statusLabel->setText( QString( "Processed %1 games" ).arg( ui->twGames->model()->rowCount() ) );
}

void batchImportDialog::finishedProcessing()
{
	ui->statusLabel->setText( "Finished processing all games" );
}
