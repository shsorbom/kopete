//=====QT Stuff here=====//
#include <QtCore>
#include <QPushButton>
#include <QDebug>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QVariantList>
#include <QListView>
#include <QTableView>
#include <QPushButton>
#include <QItemSelectionModel>
#include <QStringList>
#include <QStandardItemModel>
#include <QList>
//=======================//


//=====Kopete Stuff here=====//
#include <kopeteaccountmanager.h>
#include <kopeteaccount.h>
#include <kopeteprotocol.h>
//===========================//

#include <qca2/QtCrypto/QtCrypto>

#include "gnupgpreferences.h"

#include <kpluginfactory.h>

Q_DECLARE_METATYPE(QCA::KeyStoreEntry)

K_PLUGIN_FACTORY (GnupgPreferencesFactory, registerPlugin<GnupgPreferences>();)
K_EXPORT_PLUGIN(GnupgPreferencesFactory ("kcm_kopete_gnupg"))

GnupgPreferences::GnupgPreferences(QWidget* parent, const QVariantList& args)
    : KCModule ( GnupgPreferencesFactory::componentData(), parent, args )
{
    QCA::Initializer init;
    setButtons( Help | Apply | Default );
    QVBoxLayout *globalLayout = new QVBoxLayout(this);
    QHBoxLayout *introLayout = new QHBoxLayout(this);
    QHBoxLayout *addbuttonLayout = new QHBoxLayout(this);
    QHBoxLayout *removebuttonLayout = new QHBoxLayout(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QHBoxLayout *accountsLayout = new QHBoxLayout(this);
    QHBoxLayout *keysLayout = new QHBoxLayout(this);
    QVBoxLayout *resultsLayout = new QVBoxLayout(this);
    resultsLayout->setAlignment(Qt::AlignCenter);
    accountsList = new QListView(this);
    keysList = new QListView(this);
    resultsTable = new QTableView(this);
    resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    addCombination = new QPushButton("Add Pair",this);
    removeCombination = new QPushButton("Remove Pair",this);
    connect(addCombination,SIGNAL(clicked()),this,SLOT(addPair()));
    connect(removeCombination,SIGNAL(clicked()),this,SLOT(remPair()));
    QLabel *resultsInfo = new QLabel("Account-Key PGP pair",this);
    QLabel *intro = new QLabel("This is the GnuPG plugin.<br>Please select your private key below:",this);
    QList<Kopete::Account*> accountList = Kopete::AccountManager::self()->accounts();
    accountsModel = new QStandardItemModel(this);
    resultsModel = new QStandardItemModel(this);
    QStringList headersList;
    headersList << "Account" << "Private Key";
    resultsModel->setHorizontalHeaderLabels(headersList);
    resultsTable->setModel(resultsModel);
    resultsTable->resizeColumnsToContents();
    accountsList->setModel(accountsModel);
    if(accountList.length()==0)
    {
        QStandardItem *accountItem = new QStandardItem();
        accountItem->setData("<no account>",Qt::DisplayRole);
        accountItem->setEditable(false);
        accountsModel->appendRow(accountItem);
    }
    else
    {
        foreach( Kopete::Account *account, accountList )
        {
            QStandardItem *accountItem = new QStandardItem();
            accountItem->setData(account->accountLabel(),Qt::DisplayRole);
            accountItem->setEditable(false);
            accountsModel->appendRow(accountItem);
        }
    }

    QCA::KeyStoreManager::start();
    QCA::KeyStoreManager sman(this);
    sman.waitForBusyFinished();
    QCA::KeyStore pgpks(QString("qca-gnupg"), &sman);
    if(accountList.length()==0 || pgpks.entryList().length() == 0)
    {
      addCombination->setEnabled(false);
    }
    if(resultsModel->rowCount() == 0)
    {
      removeCombination->setEnabled(false);
    }
    keysModel = new QStandardItemModel(this);
    keysList->setModel(keysModel);
    if(pgpks.entryList().length() == 0 )
    {
        QStandardItem *keyItem = new QStandardItem();
        keyItem->setData("<no pgp keys>",Qt::DisplayRole);
        keyItem->setEditable(false);
        keysModel->appendRow(keyItem);
    }
    else
    {
        foreach(const QCA::KeyStoreEntry kse, pgpks.entryList())
        {
            QString text = kse.name()+" "+kse.id();
            QVariant v;
            v.setValue(kse);
            if(!kse.pgpSecretKey().isNull())
            {
                QStandardItem *keyItem = new QStandardItem();
                keyItem->setData(text,Qt::DisplayRole);
                keyItem->setEditable(false);
                keysModel->appendRow(keyItem);
            }
        }
    }
    accountsLayout->addWidget(accountsList);
    keysLayout->addWidget(keysList);
    introLayout->addWidget(intro);
    addbuttonLayout->addWidget(addCombination);
    removebuttonLayout->addWidget(removeCombination);
    resultsLayout->addWidget(resultsInfo);
    resultsLayout->addWidget(resultsTable);
    mainLayout->addLayout(accountsLayout);
    mainLayout->addLayout(keysLayout);
    globalLayout->addLayout(introLayout);
    globalLayout->addLayout(mainLayout);
    globalLayout->addLayout(addbuttonLayout);
    globalLayout->addLayout(resultsLayout);
    globalLayout->addLayout(removebuttonLayout);
    load();
}

void GnupgPreferences::defaults()
{
    KCModule::defaults();
}

void GnupgPreferences::load()
{
    KCModule::load();
}

void GnupgPreferences::save()
{
    KCModule::save();
}

void GnupgPreferences::addPair()
{
  QString account = accountsList->currentIndex().data().toString();
  QString key = keysList->currentIndex().data().toString(); 
  QStandardItem *pairItem = new QStandardItem();
  QStandardItem *pairItem2 = new QStandardItem();
  pairItem2->setData(key,Qt::DisplayRole);
  pairItem2->setEditable(false);
  pairItem->setData(account,Qt::DisplayRole);
  pairItem->setEditable(false);
  QList<QStandardItem *> myList;
  myList << pairItem << pairItem2;
  resultsModel->appendRow(myList);
  if(!removeCombination->isEnabled())
  {
    removeCombination->setEnabled(true);
  }
  int index = accountsList->currentIndex().row();
  accountsModel->removeRow(index);
}

void GnupgPreferences::remPair()
{
  int index = resultsTable->currentIndex().row();
  QString temp = resultsTable->currentIndex().data(0).toString();
  resultsModel->removeRow(index);
  qDebug() << "Removed INDEX: " << index << endl;
  QStandardItem *accountItem = new QStandardItem();
  accountItem->setData(temp,Qt::DisplayRole);
  accountItem->setEditable(false);
  accountsModel->appendRow(accountItem);
  if(resultsModel->rowCount() == 0)
  {
    removeCombination->setEnabled(false);
  }
}

GnupgPreferences::~GnupgPreferences()
{

}