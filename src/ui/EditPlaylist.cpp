/****************************************************************************
* EditPlaylist.cpp: Playlist editor
*****************************************************************************
* Copyright (C) 2008-2010 Tadej Novak
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* This file may be used under the terms of the
* GNU General Public License version 3.0 as published by the
* Free Software Foundation and appearing in the file LICENSE.GPL
* included in the packaging of this file.
*****************************************************************************/

#include "EditPlaylist.h"
#include "ui_EditPlaylist.h"

#include <QtGui/QCloseEvent>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

#include "core/Settings.h"
#include "plugins/PluginsLoader.h"

EditPlaylist::EditPlaylist(const QString &playlist, QWidget *parent) :
		QMainWindow(parent),
		ui(new Ui::EditPlaylist),
		_closeEnabled(false),
		_playlist(playlist),
		_channelIcon(QIcon(":/icons/images/video.png")),
		_print(0)
{
	ui->setupUi(this);

	createSettings();
	createConnections();

	ui->editWidget->setEnabled(false);
	ui->playlist->disableCategories();
	ui->playlist->open(_playlist);
	ui->editName->setText(ui->playlist->name());
	ui->number->display(ui->playlist->treeWidget()->topLevelItemCount());

	PluginsLoader *loader = new PluginsLoader();
	for(int i=0; i < loader->epgPlugin().size(); i++)
		ui->epgCombo->addItem(loader->epgName()[i]);
	delete loader;
	for(int i=0; i < ui->epgCombo->count(); i++) {
		if(ui->epgCombo->itemText(i) == ui->playlist->epgPlugin()) {
			ui->epgCombo->setCurrentIndex(i);
			break;
		}
	}
}

EditPlaylist::~EditPlaylist()
{
	delete ui;
	delete _print;
}

void EditPlaylist::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;
		default:
			break;
	}
}

void EditPlaylist::closeEvent(QCloseEvent *event)
{
	event->ignore();
	exit();
}

void EditPlaylist::createSettings()
{
	Settings *settings = new Settings(this);
	ui->toolBar->setToolButtonStyle(Qt::ToolButtonStyle(settings->toolbarLook()));
	delete settings;
}

void EditPlaylist::createConnections()
{
	connect(ui->actionDelete, SIGNAL(triggered()), this, SLOT(deleteItem()));
	connect(ui->actionAdd, SIGNAL(triggered()), this, SLOT(addItem()));
	connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(exit()));
	connect(ui->actionImport, SIGNAL(triggered()), this, SLOT(import()));
	connect(ui->actionPrint, SIGNAL(triggered()), this, SLOT(print()));

	connect(ui->buttonApplyNum, SIGNAL(clicked()), this, SLOT(editChannelNumber()));
	connect(ui->editChannelName, SIGNAL(textChanged(QString)), this, SLOT(editChannelName(QString)));
	connect(ui->editUrl, SIGNAL(textChanged(QString)), this, SLOT(editChannelUrl(QString)));
	connect(ui->editCategories, SIGNAL(textChanged(QString)), this, SLOT(editChannelCategories(QString)));
	connect(ui->editLanguage, SIGNAL(textChanged(QString)), this, SLOT(editChannelLanguage(QString)));
	connect(ui->editEpg, SIGNAL(textChanged(QString)), this, SLOT(editChannelEpg(QString)));
	connect(ui->editLogo, SIGNAL(textChanged(QString)), this, SLOT(editChannelLogo(QString)));

	connect(ui->actionUp, SIGNAL(triggered()), this, SLOT(moveUp()));
	connect(ui->actionDown, SIGNAL(triggered()), this, SLOT(moveDown()));

	connect(ui->playlist->treeWidget(), SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(editItem(QTreeWidgetItem*)));
}

void EditPlaylist::deleteItem()
{
	ui->editNumber->setText("");
	ui->editChannelName->setText("");
	ui->editUrl->setText("");
	ui->editCategories->setText("");
	ui->editLanguage->setText("");
	ui->editEpg->setText("");

	ui->editWidget->setEnabled(false);

	ui->playlist->deleteItem();

	ui->number->display(ui->playlist->treeWidget()->topLevelItemCount());
}

void EditPlaylist::addItem()
{
	editItem(ui->playlist->createItem());
	ui->number->display(ui->playlist->treeWidget()->topLevelItemCount());
}

void EditPlaylist::save()
{
	QString fileName =
		QFileDialog::getSaveFileName(this, tr("Save Channel list"),
									QDir::homePath(),
									tr("Tano TV Channel list Files (*.m3u)"));
	if (fileName.isEmpty())
		return;

	ui->playlist->save(ui->editName->text(), ui->epgCombo->currentText(), fileName);

	_closeEnabled = true;
	exit();
}

void EditPlaylist::import()
{
	QString fileName =
			QFileDialog::getOpenFileName(this, tr("Open Channel list File"),
										QDir::homePath(),
										tr("Tano TV Old Channel list Files(*.tano *.xml)"));
	if (fileName.isEmpty())
		return;

	ui->playlist->import(fileName);
	ui->number->display(ui->playlist->treeWidget()->topLevelItemCount());
}

void EditPlaylist::exit()
{
	if(_closeEnabled) {
		hide();
		return;
	}

	int ret;
	ret = QMessageBox::warning(this, tr("Playlist Editor"),
								   tr("Do you want close the editor?\nYou will lose any unsaved settings."),
								   QMessageBox::Save | QMessageBox::Close | QMessageBox::Cancel,
								   QMessageBox::Close);

	switch (ret) {
		case QMessageBox::Save:
			ui->actionSave->trigger();
			break;
		case QMessageBox::Close:
			_closeEnabled = true;
			ui->actionClose->trigger();
			break;
		case QMessageBox::Cancel:
			break;
		default:
			break;
	}
}

void EditPlaylist::print()
{
	if(_print)
		delete _print;

	_print = new Print();
	_print->channelList(ui->editName->text(), ui->playlist);
}

void EditPlaylist::editItem(QTreeWidgetItem *item)
{
	if(!ui->editWidget->isEnabled())
		ui->editWidget->setEnabled(true);

	ui->playlist->treeWidget()->setCurrentItem(item);

	ui->editNumber->setText(ui->playlist->channelRead(item)->numberString());
	ui->editChannelName->setText(ui->playlist->channelRead(item)->name());
	ui->editUrl->setText(ui->playlist->channelRead(item)->url());
	ui->editCategories->setText(ui->playlist->channelRead(item)->categories().join(","));
	ui->editLanguage->setText(ui->playlist->channelRead(item)->language());
	ui->editEpg->setText(ui->playlist->channelRead(item)->epg());
	ui->editLogo->setText(ui->playlist->channelRead(item)->logo());
}

void EditPlaylist::editChannelNumber()
{
	QString text = ui->editNumber->text();
	if(text.toInt() != ui->playlist->channelRead(ui->playlist->treeWidget()->currentItem())->number())
		ui->editNumber->setText(QString().number(ui->playlist->processNum(ui->playlist->treeWidget()->currentItem(), text.toInt())));
	ui->playlist->treeWidget()->sortByColumn(0, Qt::AscendingOrder);
}

void EditPlaylist::editChannelName(const QString &text)
{
	ui->playlist->channelRead(ui->playlist->treeWidget()->currentItem())->setName(text);
	ui->playlist->treeWidget()->currentItem()->setText(1, text);
}

void EditPlaylist::editChannelUrl(const QString &text)
{
	ui->playlist->channelRead(ui->playlist->treeWidget()->currentItem())->setUrl(text);
}

void EditPlaylist::editChannelCategories(const QString &text)
{
	ui->playlist->channelRead(ui->playlist->treeWidget()->currentItem())->setCategories(text.split(","));
}

void EditPlaylist::editChannelLanguage(const QString &text)
{
	ui->playlist->channelRead(ui->playlist->treeWidget()->currentItem())->setLanguage(text);
}

void EditPlaylist::editChannelEpg(const QString &text)
{
	ui->playlist->channelRead(ui->playlist->treeWidget()->currentItem())->setEpg(text);
}

void EditPlaylist::editChannelLogo(const QString &text)
{
	ui->playlist->channelRead(ui->playlist->treeWidget()->currentItem())->setLogo(text);
}

void EditPlaylist::moveUp()
{
	ui->playlist->moveUp(ui->playlist->treeWidget()->currentItem());
	ui->editNumber->setText(ui->playlist->channelRead(ui->playlist->treeWidget()->currentItem())->numberString());
}

void EditPlaylist::moveDown()
{
	ui->playlist->moveDown(ui->playlist->treeWidget()->currentItem());
	ui->editNumber->setText(ui->playlist->channelRead(ui->playlist->treeWidget()->currentItem())->numberString());
}
