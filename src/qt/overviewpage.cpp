// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2018 The Catocoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "spork.h"
#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "init.h"
#include "obfuscation.h"
#include "obfuscationconfig.h"
#include "optionsmodel.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QTimer>

#define DECORATION_SIZE 48
#define ICON_OFFSET 16
#define NUM_ITEMS 5

extern CWallet* pwalletMain;

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate() : QAbstractItemDelegate(), unit(BitcoinUnits::CATO)
    {
    }

    inline void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        mainRect.moveLeft(ICON_OFFSET);
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2 * ypad) / 2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top() + ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top() + ypad + halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = COLOR_BLACK;
        if (value.canConvert<QBrush>()) {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft | Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool()) {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top() + ypad + halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if (amount < 0) {
            foreground = COLOR_NEGATIVE;
        } else if (!confirmed) {
            foreground = COLOR_UNCONFIRMED;
        } else {
            foreground = COLOR_BLACK;
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if (!confirmed) {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight | Qt::AlignVCenter, amountText);

        painter->setPen(COLOR_BLACK);
        painter->drawText(amountRect, Qt::AlignLeft | Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget* parent) : QWidget(parent),
                                              ui(new Ui::OverviewPage),
                                              clientModel(0),
                                              walletModel(0),
                                              currentBalance(-1),
                                              currentUnconfirmedBalance(-1),
                                              currentImmatureBalance(-1),
                                              currentZerocoinBalance(-1),
                                              currentUnconfirmedZerocoinBalance(-1),
                                              currentimmatureZerocoinBalance(-1),
                                              currentWatchOnlyBalance(-1),
                                              currentWatchUnconfBalance(-1),
                                              currentWatchImmatureBalance(-1),
                                              txdelegate(new TxViewDelegate()),
                                              filter(0)
{
    nDisplayUnit = 0; // just make sure it's not unitialized
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));


    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex& index)
{
    if (filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::getPercentage(CAmount nUnlockedBalance, CAmount nZerocoinBalance, QString& sCATOPercentage, QString& szCATOPercentage)
{
    int nPrecision = 2;
    double dzPercentage = 0.0;

    if (nZerocoinBalance <= 0){
        dzPercentage = 0.0;
    }
    else{
        if (nUnlockedBalance <= 0){
            dzPercentage = 100.0;
        }
        else{
            dzPercentage = 100.0 * (double)(nZerocoinBalance / (double)(nZerocoinBalance + nUnlockedBalance));
        }
    }

    double dPercentage = 100.0 - dzPercentage;
    
    szCATOPercentage = "(" + QLocale(QLocale::system()).toString(dzPercentage, 'f', nPrecision) + " %)";
    sCATOPercentage = "(" + QLocale(QLocale::system()).toString(dPercentage, 'f', nPrecision) + " %)";
    
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, 
                              const CAmount& zerocoinBalance, const CAmount& unconfirmedZerocoinBalance, const CAmount& immatureZerocoinBalance,
                              const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentZerocoinBalance = zerocoinBalance;
    currentUnconfirmedZerocoinBalance = unconfirmedZerocoinBalance;
    currentimmatureZerocoinBalance = immatureZerocoinBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;

    // CATO labels
    ui->labelBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, balance - immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, zerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal1->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, balance + unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal2->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, GetSporkValue(SPORK_17_CURRENT_MN_COLLATERAL) * 100000000, false, BitcoinUnits::separatorAlways));
    ui->labelTotal3->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, GetSporkValue(SPORK_59_CURRENT_MN_COLLATERAL) * 100000000, false, BitcoinUnits::separatorAlways));
    ui->labelTotal4->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, GetSporkValue(SPORK_60_CURRENT_MN_COLLATERAL) * 100000000, false, BitcoinUnits::separatorAlways));
unsigned int count_tier_1 = 0;
unsigned int count_tier_2 = 0;
unsigned int count_tier_3 = 0;
//do calculation shit here
CTransaction wtx; 
uint256 hashBlock;
    std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
//std::vector<CMasternode> final_list;
BOOST_FOREACH(CMasternode &mn, vMasternodes){ 

if(GetTransaction(mn.vin.prevout.hash, wtx, hashBlock, true)) {
for (int i = 0; i< wtx.vout.size(); i++) {
//obj.push_back(Pair("Tx Outpoint", wtx2.vout[i].nValue));
//obj.push_back(Pair("CScript Dest", wtx2.vout[i].scriptPubKey.ToString()));
if (wtx.vout[i].scriptPubKey.ToString() 
== GetScriptForDestination(mn.pubKeyCollateralAddress.GetID()).ToString() 
&& wtx.vout[i].nValue/100000000 >= 25000) {
unsigned int collat_required;

CTransaction wtx2;
uint256 hashBlock2;
if(GetTransaction(mn.vin.prevout.hash, wtx2, hashBlock2, true)) {
//hashblock2 now has the block hash
BlockMap::iterator iter = mapBlockIndex.find(hashBlock2);
if (iter != mapBlockIndex.end()) {
unsigned int txnheight = iter->second->nHeight;
//block height of txn
if (txnheight <= GetSporkValue(SPORK_37_LAST_25000_COLLAT_BLOCK)){
collat_required = 25000;
} else if (txnheight <= GetSporkValue(SPORK_38_LAST_26250_COLLAT_BLOCK)) {
collat_required = 26250;
} else if (txnheight <= GetSporkValue(SPORK_39_LAST_27575_COLLAT_BLOCK)){
collat_required = 27575;
} else if (txnheight <= GetSporkValue(SPORK_40_LAST_28950_COLLAT_BLOCK)){
collat_required = 28950;
} else if (txnheight <= GetSporkValue(SPORK_41_LAST_30400_COLLAT_BLOCK)){
collat_required = 30400;
} else if (txnheight <= GetSporkValue(SPORK_42_LAST_31900_COLLAT_BLOCK)){
collat_required = 31900;
} else if (txnheight <= GetSporkValue(SPORK_43_LAST_33500_COLLAT_BLOCK)){
collat_required = 33500;
} else if (txnheight <= GetSporkValue(SPORK_44_LAST_35175_COLLAT_BLOCK)){
collat_required = 35175;
} else if (txnheight <= GetSporkValue(SPORK_45_LAST_36925_COLLAT_BLOCK)){
collat_required = 36925;
} else if (txnheight <= GetSporkValue(SPORK_46_LAST_38775_COLLAT_BLOCK)){
collat_required = 38775;
} else if (txnheight <= GetSporkValue(SPORK_47_LAST_40725_COLLAT_BLOCK)){
collat_required = 40725;
} else if (txnheight <= GetSporkValue(SPORK_48_LAST_42750_COLLAT_BLOCK)){
collat_required = 42750;
} else if (txnheight <= GetSporkValue(SPORK_49_LAST_44900_COLLAT_BLOCK)){
collat_required = 44900;
} else if (txnheight <= GetSporkValue(SPORK_50_LAST_47150_COLLAT_BLOCK)){
collat_required = 47150;
} else if (txnheight <= GetSporkValue(SPORK_51_LAST_49500_COLLAT_BLOCK)){
collat_required = 49500;
} else if (txnheight <= GetSporkValue(SPORK_52_LAST_51975_COLLAT_BLOCK)){
collat_required = 51975;
} else if (txnheight <= GetSporkValue(SPORK_53_LAST_54575_COLLAT_BLOCK)){
collat_required = 54575;
} else if (txnheight <= GetSporkValue(SPORK_54_LAST_57304_COLLAT_BLOCK)){
collat_required = 57304;
} else if (txnheight <= GetSporkValue(SPORK_55_LAST_60169_COLLAT_BLOCK)){

collat_required = 60169;
} else if (txnheight <= GetSporkValue(SPORK_56_LAST_63175_COLLAT_BLOCK)){

collat_required = 63175;
} else if (txnheight <= GetSporkValue(SPORK_57_LAST_66325_COLLAT_BLOCK)){

collat_required = 66325;

} else if (txnheight <= GetSporkValue(SPORK_58_LAST_69650_COLLAT_BLOCK)){

collat_required = 69650;
}

} else {
collat_required = 69650;
}
} else {
collat_required = 69650;
}



if (wtx.vout[i].nValue/100000000 >= collat_required) {
count_tier_1 += 1;
}
}// inner if

if (wtx.vout[i].scriptPubKey.ToString() 
== GetScriptForDestination(mn.pubKeyCollateralAddress.GetID()).ToString() 
&& wtx.vout[i].nValue/100000000 >= 60000) {
unsigned int collat_required2;

CTransaction wtx3;
uint256 hashBlock3;
if(GetTransaction(mn.vin.prevout.hash, wtx3, hashBlock3, true)) {
//hashBlock3 now has the block hash
BlockMap::iterator iter = mapBlockIndex.find(hashBlock3);
if (iter != mapBlockIndex.end()) {
unsigned int txnheight2 = iter->second->nHeight;
//block height of txn
if (txnheight2 <= GetSporkValue(SPORK_37_LAST_25000_COLLAT_BLOCK)){
collat_required2 = 60000;
} else if (txnheight2 <= GetSporkValue(SPORK_38_LAST_26250_COLLAT_BLOCK)) {
collat_required2 = 63000;
} else if (txnheight2 <= GetSporkValue(SPORK_39_LAST_27575_COLLAT_BLOCK)){
collat_required2 = 66150;
} else if (txnheight2 <= GetSporkValue(SPORK_40_LAST_28950_COLLAT_BLOCK)){
collat_required2 = 69450;
} else if (txnheight2 <= GetSporkValue(SPORK_41_LAST_30400_COLLAT_BLOCK)){
collat_required2 = 72900;
} else if (txnheight2 <= GetSporkValue(SPORK_42_LAST_31900_COLLAT_BLOCK)){
collat_required2 = 76575;
} else if (txnheight2 <= GetSporkValue(SPORK_43_LAST_33500_COLLAT_BLOCK)){
collat_required2 = 80400;
} else if (txnheight2 <= GetSporkValue(SPORK_44_LAST_35175_COLLAT_BLOCK)){
collat_required2 = 84425;
} else if (txnheight2 <= GetSporkValue(SPORK_45_LAST_36925_COLLAT_BLOCK)){
collat_required2 = 88650;
} else if (txnheight2 <= GetSporkValue(SPORK_46_LAST_38775_COLLAT_BLOCK)){
collat_required2 = 93083;
} else if (txnheight2 <= GetSporkValue(SPORK_47_LAST_40725_COLLAT_BLOCK)){
collat_required2 = 97725;
} else if (txnheight2 <= GetSporkValue(SPORK_48_LAST_42750_COLLAT_BLOCK)){
collat_required2 = 102611;
} else if (txnheight2 <= GetSporkValue(SPORK_49_LAST_44900_COLLAT_BLOCK)){
collat_required2 = 107750;
} else if (txnheight2 <= GetSporkValue(SPORK_50_LAST_47150_COLLAT_BLOCK)){
collat_required2 = 113125;
} else if (txnheight2 <= GetSporkValue(SPORK_51_LAST_49500_COLLAT_BLOCK)){
collat_required2 = 118800;
} else if (txnheight2 <= GetSporkValue(SPORK_52_LAST_51975_COLLAT_BLOCK)){
collat_required2 = 124725;
} else if (txnheight2 <= GetSporkValue(SPORK_53_LAST_54575_COLLAT_BLOCK)){
collat_required2 = 130975;
} else if (txnheight2 <= GetSporkValue(SPORK_54_LAST_57304_COLLAT_BLOCK)){
collat_required2 = 137500;
} else if (txnheight2 <= GetSporkValue(SPORK_55_LAST_60169_COLLAT_BLOCK)){

collat_required2 = 144400;
} else if (txnheight2 <= GetSporkValue(SPORK_56_LAST_63175_COLLAT_BLOCK)){

collat_required2 = 151600;
} else if (txnheight2 <= GetSporkValue(SPORK_57_LAST_66325_COLLAT_BLOCK)){

collat_required2 = 159180;

} else if (txnheight2 <= GetSporkValue(SPORK_58_LAST_69650_COLLAT_BLOCK)){

collat_required2 = 167150;
}

} else {
collat_required2 = 167150;
}
} else {
collat_required2 = 167150;
}



if (wtx3.vout[i].nValue/100000000 >= collat_required2) {

count_tier_2 += 1;
}
}// inner if

if (wtx.vout[i].scriptPubKey.ToString() 
== GetScriptForDestination(mn.pubKeyCollateralAddress.GetID()).ToString() 
&& wtx.vout[i].nValue/100000000 >= 100000) {

unsigned int collat_required4;

CTransaction wtx4;
uint256 hashBlock4;
if(GetTransaction(mn.vin.prevout.hash, wtx4, hashBlock4, true)) {
//hashBlock4 now has the block hash
BlockMap::iterator iter = mapBlockIndex.find(hashBlock4);
if (iter != mapBlockIndex.end()) {
unsigned int txnheight4 = iter->second->nHeight;
//block height of txn
if (txnheight4 <= GetSporkValue(SPORK_37_LAST_25000_COLLAT_BLOCK)){
collat_required4 = 100000;
} else if (txnheight4 <= GetSporkValue(SPORK_38_LAST_26250_COLLAT_BLOCK)) {
collat_required4 = 105000;
} else if (txnheight4 <= GetSporkValue(SPORK_39_LAST_27575_COLLAT_BLOCK)){
collat_required4 = 110250;
} else if (txnheight4 <= GetSporkValue(SPORK_40_LAST_28950_COLLAT_BLOCK)){
collat_required4 = 115750;
} else if (txnheight4 <= GetSporkValue(SPORK_41_LAST_30400_COLLAT_BLOCK)){
collat_required4 = 121550;
} else if (txnheight4 <= GetSporkValue(SPORK_42_LAST_31900_COLLAT_BLOCK)){
collat_required4 = 127625;
} else if (txnheight4 <= GetSporkValue(SPORK_43_LAST_33500_COLLAT_BLOCK)){
collat_required4 = 134000;
} else if (txnheight4 <= GetSporkValue(SPORK_44_LAST_35175_COLLAT_BLOCK)){
collat_required4 = 140700;
} else if (txnheight4 <= GetSporkValue(SPORK_45_LAST_36925_COLLAT_BLOCK)){
collat_required4 = 147750;
} else if (txnheight4 <= GetSporkValue(SPORK_46_LAST_38775_COLLAT_BLOCK)){
collat_required4 = 155150;
} else if (txnheight4 <= GetSporkValue(SPORK_47_LAST_40725_COLLAT_BLOCK)){
collat_required4 = 162900;
} else if (txnheight4 <= GetSporkValue(SPORK_48_LAST_42750_COLLAT_BLOCK)){
collat_required4 = 171000;
} else if (txnheight4 <= GetSporkValue(SPORK_49_LAST_44900_COLLAT_BLOCK)){
collat_required4 = 179600;
} else if (txnheight4 <= GetSporkValue(SPORK_50_LAST_47150_COLLAT_BLOCK)){
collat_required4 = 188580;
} else if (txnheight4 <= GetSporkValue(SPORK_51_LAST_49500_COLLAT_BLOCK)){
collat_required4 = 198000;
} else if (txnheight4 <= GetSporkValue(SPORK_52_LAST_51975_COLLAT_BLOCK)){
collat_required4 = 207900;
} else if (txnheight4 <= GetSporkValue(SPORK_53_LAST_54575_COLLAT_BLOCK)){
collat_required4 = 218275;
} else if (txnheight4 <= GetSporkValue(SPORK_54_LAST_57304_COLLAT_BLOCK)){
collat_required4 = 229200;
} else if (txnheight4 <= GetSporkValue(SPORK_55_LAST_60169_COLLAT_BLOCK)){

collat_required4 = 240650;
} else if (txnheight4 <= GetSporkValue(SPORK_56_LAST_63175_COLLAT_BLOCK)){

collat_required4 = 252683;
} else if (txnheight4 <= GetSporkValue(SPORK_57_LAST_66325_COLLAT_BLOCK)){

collat_required4 = 265317;

} else if (txnheight4 <= GetSporkValue(SPORK_58_LAST_69650_COLLAT_BLOCK)){

collat_required4 = 278600;
}

} else {
collat_required4 = 278600;
}
} else {
collat_required4 = 278600;
}



if (wtx4.vout[i].nValue/100000000 >= collat_required4) {


count_tier_3 += 1;
}
}// inner if

}//for loop
}//gettransaction if
       // obj.push_back(Pair("Collat amnts", wtx2.ToString()));
}//fboost_foreach

int real_tier_2 = count_tier_2-count_tier_3;
int real_tier_1 = count_tier_1 - real_tier_2 - count_tier_3;

QString num5 = QString::number(real_tier_1) + " Nodes";
QString num6 = QString::number(real_tier_2) + " Nodes";
QString num7 = QString::number(count_tier_3) + " Nodes";
    ui->labelTotal5->setText(num5);
    ui->labelTotal6->setText(num6);
    ui->labelTotal7->setText(num7);


 //   ui->labelTotal5->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, count_tier_1 * 100000000, false, BitcoinUnits::separatorAlways));
   // ui->labelTotal6->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, count_tier_2 * 100000000, false, BitcoinUnits::separatorAlways));
    //ui->labelTotal7->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, count_tier_3 * 100000000, false, BitcoinUnits::separatorAlways));
    // Watchonly labels
    ui->labelWatchAvailable->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchPending->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, BitcoinUnits::separatorAlways));

    // zCATO labels
    QString szPercentage = "";
    QString sPercentage = "";
    CAmount nLockedBalance = 0;
    if (pwalletMain) {
        nLockedBalance = pwalletMain->GetLockedCoins();
    }
    ui->labelLockedBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nLockedBalance, false, BitcoinUnits::separatorAlways));

    CAmount nTotalBalance = balance + unconfirmedBalance;
    CAmount nUnlockedBalance = nTotalBalance - nLockedBalance;
    CAmount matureZerocoinBalance = zerocoinBalance - immatureZerocoinBalance;
    getPercentage(nUnlockedBalance, zerocoinBalance, sPercentage, szPercentage);

    ui->labelBalancez->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nTotalBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalancez->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, zerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalanceImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, immatureZerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalanceUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, unconfirmedZerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalanceMature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, matureZerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotalz->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nTotalBalance + zerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelUnLockedBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nUnlockedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelCATOPercent->setText(sPercentage);
    ui->labelzCATOPercent->setText(szPercentage);

    // Adjust bubble-help according to AutoMint settings
    QString automintHelp = tr("Current percentage of zCATO.\nIf AutoMint is enabled this percentage will settle around the configured AutoMint percentage (default = 10%).\n");
    bool fEnableZeromint = GetBoolArg("-enablezeromint", false);
    int nZeromintPercentage = GetArg("-zeromintpercentage", 10);
    if (fEnableZeromint) {
        automintHelp += tr("AutoMint is currently enabled and set to ") + QString::number(nZeromintPercentage) + "%.\n";
        automintHelp += tr("To disable AutoMint delete set 'enablezeromint=1' to 'enablezeromint=0' in catocoin2.conf.");
    }
    else {
        automintHelp += tr("AutoMint is currently disabled.\nTo enable AutoMint add 'enablezeromint=1' in catocoin2.conf");
    }
    ui->labelzCATOPercent->setToolTip(automintHelp);

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance

    static int cachedTxLocks = 0;

    if (cachedTxLocks != nCompleteTXLocks) {
        cachedTxLocks = nCompleteTXLocks;
        ui->listTransactions->update();
    }
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly) {
        ui->labelWatchImmature->hide();
    } else {
        ui->labelBalance->setIndent(20);
        ui->labelUnconfirmed->setIndent(20);
        ui->labelImmature->setIndent(20);
        ui->labelTotal1->setIndent(20);
        ui->labelTotal2->setIndent(20);
        ui->labelTotal3->setIndent(20);

        ui->labelTotal4->setIndent(20);

    }
}

void OverviewPage::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model) {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    if (model && model->getOptionsModel()) {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getZerocoinBalance(), model->getUnconfirmedZerocoinBalance(), model->getImmatureZerocoinBalance(), 
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this, 
                         SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("CATO")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if (walletModel && walletModel->getOptionsModel()) {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if (currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentZerocoinBalance, currentUnconfirmedZerocoinBalance, currentimmatureZerocoinBalance,
                currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = nDisplayUnit;

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString& warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}
