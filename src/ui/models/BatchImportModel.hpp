//
// Created by kj16609 on 3/26/23.
//

#ifndef HYDRUS95_BATCHIMPORTMODEL_HPP
#define HYDRUS95_BATCHIMPORTMODEL_HPP

#include <filesystem>

#include <QAbstractTableModel>

#include "h95/threading/ImportPreProcessor.hpp"

enum ImportColumns
{
	FOLDER_PATH,
	TITLE,
	CREATOR,
	ENGINE,
	VERSION,
	SIZE,
	EXECUTABLES
};

class BatchImportModel final : public QAbstractTableModel
{
	std::vector< GameImportData > m_data {};

  public:


	const std::vector< GameImportData >& getData() const { return m_data; }

	int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
	int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
	QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
	QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;
	Qt::ItemFlags flags( const QModelIndex& index ) const override;
	bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole ) override;

	void clearData();


  public slots:
	void addGame( GameImportData data );
	void addGames( std::vector<GameImportData> data );

	friend class BatchImportDelegate;
};

#endif //HYDRUS95_BATCHIMPORTMODEL_HPP
