//
// Created by kj16609 on 4/4/23.
//

// You may need to build the project (run Qt uic code generator) to get "ui_RecordEditor.h" resolved

#include "RecordEditor.hpp"

#include <moc_RecordEditor.cpp>

#include <QDragEnterEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>

#include "core/database/record/RecordBanner.hpp"
#include "core/database/record/RecordPreviews.hpp"
#include "ui/dialog/ProgressBarDialog.hpp"
#include "ui/models/FilepathModel.hpp"
#include "ui_RecordEditor.h"

RecordEditor::RecordEditor( const RecordID record, QWidget* parent ) :
  QDialog( parent ),
  m_record( record ),
  m_banner_path( m_record->banners().getBannerPath( Normal ) ),
  m_preview_paths( m_record->previews().getPreviewPaths() ),
  m_versions( m_record->getVersions() ),
  ui( new Ui::RecordEditor )
{
	ui->setupUi( this );
}

void RecordEditor::loadRecordInfo()
{
	ui->titleLineEdit->setText( m_record->get< RecordColumns::Title >() );
	ui->creatorLineEdit->setText( m_record->get< RecordColumns::Creator >() );
	ui->engineLineEdit->setText( m_record->get< RecordColumns::Engine >() );
	//ui->gameText->setText( m_record->getDescription() );
}

void RecordEditor::loadBanners()
{
	ui->bannerPreview
		->setPixmap( m_record->banners()
	                     .getBanner( ui->bannerPreview->size() - QSize( 25, 40 ), KEEP_ASPECT_RATIO, Normal ) );
}

void RecordEditor::loadPreviews()
{
	ui->previewList->setPaths( m_record->previews().getPreviewPaths() );
}

void RecordEditor::loadTags()
{
	const auto user_tags { m_record->getUserTags() };
	QStringList user_tags_list;
	for ( const auto& tag : user_tags ) user_tags_list.emplace_back( tag );

	ui->userTagsList->clear();
	ui->userTagsList->addItems( std::move( user_tags_list ) );

	const auto all_tags { m_record->getAllTags() };
	QStringList all_tags_list;
	for ( const auto& tag : all_tags ) all_tags_list.emplace_back( tag );

	ui->processedTagsList->clear();
	ui->processedTagsList->addItems( std::move( all_tags_list ) );
}

RecordEditor::~RecordEditor()
{
	delete ui;
}

void RecordEditor::resizeEvent( QResizeEvent* event )
{
	loadBanners();

	QDialog::resizeEvent( event );
}

void RecordEditor::on_splitter_splitterMoved( [[maybe_unused]] int pos, [[maybe_unused]] int index )
{
	loadBanners();
}

void RecordEditor::showEvent( QShowEvent* event )
{
	QDialog::showEvent( event );
	loadRecordInfo();
	loadBanners();
	loadPreviews();
	loadTags();
	loadVersions();
}

void RecordEditor::on_btnSetBanner_pressed()
{
	QFileDialog file_dialog { this, "Select Banner" };

	file_dialog.setFileMode( QFileDialog::ExistingFile );
	file_dialog.setNameFilter( "Images (*.png *.jpg *.jpeg *.bmp *.webp)" );
	file_dialog.setViewMode( QFileDialog::Detail );

	if ( file_dialog.exec() )
		m_record->banners()
			.setBanner( std::filesystem::path( file_dialog.selectedFiles().first().toStdString() ), Normal );
}

void RecordEditor::on_btnAddPreviews_pressed()
{
	QFileDialog file_dialog { this, "Select Previews" };

	file_dialog.setFileMode( QFileDialog::ExistingFiles );
	file_dialog.setNameFilter( "Images (*.png *.jpg *.jpeg *.bmp *.webp)" );
	file_dialog.setViewMode( QFileDialog::Detail );

	if ( file_dialog.exec() )
	{
		for ( const auto& path : file_dialog.selectedFiles() )
			m_record->previews().addPreview( { path.toStdString() } );
	}
}

void RecordEditor::on_btnRemovePreviews_pressed()
{
	if ( QMessageBox::question(
			 this, "Are you sure?", "Are you sure you want to remove the selected previews? This can't be reverted" )
	     == QMessageBox::Yes )
	{
		//Remove selected
		for ( const auto& preview : ui->previewList->selectedItems() ) m_record->previews().removePreview( preview );
	}
	else
		return;
}

void RecordEditor::on_btnDeleteVersion_pressed()
{
	if ( ui->versionList->count() == 0 ) return;

	const auto selected_list { ui->versionList->selectedItems() };
	if ( selected_list.size() == 0 ) return;
	const auto selected { selected_list.at( 0 ) };

	const auto version { m_record->getVersion( selected->text() ) };

	if ( !version.has_value() ) return;

	if (
		QMessageBox::question(
			this,
			"Are you sure?",
			QString(
				"Are you absolutely sure you want to delete %1. This can NOT be undone and you will lose ALL files associated with this version!" )
				.arg( version->getVersionName() ) )
		== QMessageBox::Yes )
	{
		m_record->removeVersion( *version );

		for ( const auto& file : std::filesystem::recursive_directory_iterator( version->getPath() ) )
		{
			try
			{
				if ( file.is_regular_file() ) std::filesystem::remove( file );
			}
			catch ( std::filesystem::filesystem_error& e )
			{
				spdlog::error( "Failed to delete {} due to {}", file.path(), e.what() );
			}
		}

		try
		{
			std::filesystem::remove_all( version->getPath() );
		}
		catch ( std::filesystem::filesystem_error& e )
		{
			spdlog::error( "Failed to delete all due to {}", e.what() );
		}

		loadVersions();
	}
	else
		return;
}

void RecordEditor::on_btnAddVersion_pressed()
{
	const auto path { QFileDialog::getExistingDirectory( this, "Select Version Directory" ) };

	if ( path.isEmpty() ) return;

	const auto version_name { QInputDialog::getText( this, "Version Name", "Version Name" ) };

	if ( version_name.isEmpty() ) return;

	if ( m_record->getVersion( version_name ).has_value() )
	{
		QMessageBox::critical( this, "Invalid Version Name", "A version with that name already exists!" );
		return;
	}

	const QString executable { QFileDialog::getOpenFileName( this, "Select Executable" ) };

	if ( executable.isEmpty() ) return;

	if ( !executable.startsWith( path ) )
	{
		QMessageBox::critical( this, "Invalid Executable", "The executable must be in the version directory!" );
		return;
	}

	const std::filesystem::path source { path.toStdString() };

	const std::filesystem::path relative { std::filesystem::relative( executable.toStdString(), source ) };

	const bool should_move {
		QMessageBox::question( this, "Move Files?", "Would you like to move the files to the atlas import location?" )
		== QMessageBox::Yes
	};

	const auto title { m_record->get< RecordColumns::Title >() };
	const auto creator { m_record->get< RecordColumns::Creator >() };

	//Calculate filesize
	std::size_t size { 0 };

	ProgressBarDialog* dialog { new ProgressBarDialog( this ) };
	dialog->show();
	dialog->showSubProgress( true );
	dialog->setText( "Importing game" );
	dialog->setMax( 3 );

	dialog->setSubMax( 0 );
	dialog->setSubText( "Scanning files..." );
	std::vector< std::filesystem::path > files;
	for ( auto file : std::filesystem::recursive_directory_iterator( source ) )
		if ( file.is_regular_file() ) files.push_back( std::filesystem::relative( file, source ) );

	dialog->setValue( 1 );
	dialog->setSubMax( static_cast< int >( files.size() ) );

	int counter { 0 };

	if ( should_move )
	{
		dialog->setSubText( "Copying files..." );
		const std::filesystem::path dest_root { config::paths::games::getPath() };
		const std::filesystem::path dest_path { dest_root / creator.toStdString() / title.toStdString()
			                                    / version_name.toStdString() };

		for ( auto file : files )
		{
			const auto source_path { source / file };
			const auto dest { dest_path / file };

			size += std::filesystem::file_size( source_path );

			if ( !std::filesystem::exists( dest.parent_path() ) )
				std::filesystem::create_directories( dest.parent_path() );

			std::filesystem::copy_file( source_path, dest );

			dialog->setValue( ++counter );
		}

		m_record->addVersion( version_name, dest_path, relative, size, false );
	}
	else
	{
		dialog->setSubText( "Calculating size..." );
		for ( auto file : files )
		{
			size += std::filesystem::file_size( source / file );
			dialog->setValue( ++counter );
		}

		m_record->addVersion( version_name, source, relative, size, true );
	}

	delete dialog;

	QMessageBox::information( this, "Import complete", "Import complete!" );
}

void RecordEditor::switchTabs( const int index )
{
	ui->tabWidget->setCurrentIndex( index );
}

void RecordEditor::on_btnChangeTitle_pressed()
{
	if ( const auto output = QInputDialog::
	         getText( this, "Change Title", "New Title", QLineEdit::Normal, m_record->get< RecordColumns::Title >() );
	     output != m_record->get< RecordColumns::Title >() && !output.isEmpty() )
	{
		Transaction trans {};
		std::size_t count { 0 };
		trans << "SELECT COUNT(*) FROM records WHERE title = ? AND creator = ?;" << output.toStdString()
			  << m_record->get< RecordColumns::Creator >().toStdString()
			  << m_record->get< RecordColumns::Engine >().toStdString()
			>> count;

		if ( count != 0 )
		{
			QMessageBox::warning( this, "Duplicate!", "A title with that name already exists on the same creator!" );
			return;
		}

		m_record->set< RecordColumns::Title >( output );
	}
	loadRecordInfo();
}

void RecordEditor::on_btnChangeCreator_pressed()
{
	if ( const auto output = QInputDialog::getText(
			 this, "Change Creator", "New Creator", QLineEdit::Normal, m_record->get< RecordColumns::Creator >() );
	     output != m_record->get< RecordColumns::Creator >() && !output.isEmpty() )
	{
		Transaction trans {};
		std::size_t count { 0 };
		trans << "SELECT COUNT(*) FROM records WHERE title = ? AND creator = ? AND engine = ?;" << output.toStdString()
			  << m_record->get< RecordColumns::Creator >().toStdString()
			  << m_record->get< RecordColumns::Engine >().toStdString()
			>> count;

		if ( count != 0 )
		{
			QMessageBox::warning(
				this,
				"Duplicate!",
				"The creator already has a game with this name and engine! Merging is not implemented yet" );
			return;
		}

		m_record->set< RecordColumns::Creator >( output );
	}
	loadRecordInfo();
}

void RecordEditor::on_btnChangeEngine_pressed()
{
	if ( const QString output = QInputDialog::getText(
			 this, "Change Engine", "New Engine", QLineEdit::Normal, m_record->get< RecordColumns::Engine >() );
	     output != m_record->get< RecordColumns::Engine >() && !output.isEmpty() )
	{
		Transaction trans {};
		std::size_t count { 0 };
		trans << "SELECT COUNT(*) FROM records WHERE title = ? AND creator = ? AND engine = ?;" << output.toStdString()
			  << m_record->get< RecordColumns::Creator >().toStdString()
			  << m_record->get< RecordColumns::Engine >().toStdString()
			>> count;

		if ( count != 0 )
		{
			QMessageBox::
				warning( this, "Duplicate!", "There is already a game with the same creator, name and engine!" );
			return;
		}
		m_record->set< RecordColumns::Engine >( output );
	}
	loadRecordInfo();
}

void RecordEditor::on_btnApplyDesc_pressed()
{
	m_record->setDesc( ui->gameText->toPlainText() );
}

void RecordEditor::on_tagEdit_returnPressed()
{
	const auto text { ui->tagEdit->text() };

	//If tag is already added then remove it instead
	if ( const auto items = ui->userTagsList->findItems( text, Qt::MatchExactly ); items.size() > 0 )
		m_record->removeUserTag( text );
	else
		m_record->addUserTag( text );

	ui->tagEdit->clear();

	loadTags();
}

void RecordEditor::on_versionList_currentRowChanged( [[maybe_unused]] int idx )
{
	if ( ui->versionList->count() == 0 || idx < 0 )
	{
		ui->activeVersion->setVersion( std::nullopt );
		return;
	}
	else
	{
		const auto item { ui->versionList->item( idx ) };

		if ( item == nullptr ) return;

		const auto version { m_record->getVersion( item->text() ) };

		ui->activeVersion->setVersion( version );
	}
}

void RecordEditor::loadVersions()
{
	ui->versionList->clear();

	QStringList list;

	for ( const auto& version : m_record->getVersions() ) list.emplace_back( version.getVersionName() );

	ui->versionList->addItems( std::move( list ) );
}

void RecordEditor::on_previewList_reordered()
{
	m_record->previews().reorderPreviews( ui->previewList->pathmodel()->getFilepaths() );
}
