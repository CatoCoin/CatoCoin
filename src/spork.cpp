// Copyright (c) 2014-2016 The Dash developers
// Copyright (c) 2016-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "spork.h"
#include "base58.h"
#include "key.h"
#include "main.h"
#include "masternode-budget.h"
#include "net.h"
#include "protocol.h"
#include "sync.h"
#include "sporkdb.h"
#include "util.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

class CSporkMessage;
class CSporkManager;

CSporkManager sporkManager;

std::map<uint256, CSporkMessage> mapSporks;
std::map<int, CSporkMessage> mapSporksActive;

// Catocoin: on startup load spork values from previous session if they exist in the sporkDB
void LoadSporksFromDB()
{
    for (int i = SPORK_START; i <= SPORK_END; ++i) {
        // Since not all spork IDs are in use, we have to exclude undefined IDs
        std::string strSpork = sporkManager.GetSporkNameByID(i);
        if (strSpork == "Unknown") continue;

        // attempt to read spork from sporkDB
        CSporkMessage spork;
        if (!pSporkDB->ReadSpork(i, spork)) {
            LogPrintf("%s : no previous value for %s found in database\n", __func__, strSpork);
            continue;
        }

        // add spork to memory
        mapSporks[spork.GetHash()] = spork;
        mapSporksActive[spork.nSporkID] = spork;
        std::time_t result = spork.nValue;
        // If SPORK Value is greater than 1,000,000 assume it's actually a Date and then convert to a more readable format
        if (spork.nValue > 1000000) {
            LogPrintf("%s : loaded spork %s with value %d : %s", __func__,
                      sporkManager.GetSporkNameByID(spork.nSporkID), spork.nValue,
                      std::ctime(&result));
        } else {
            LogPrintf("%s : loaded spork %s with value %d\n", __func__,
                      sporkManager.GetSporkNameByID(spork.nSporkID), spork.nValue);
        }
    }
}

void ProcessSpork(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if (fLiteMode) return; //disable all obfuscation/masternode related functionality

    if (strCommand == "spork") {
        //LogPrintf("ProcessSpork::spork\n");
        CDataStream vMsg(vRecv);
        CSporkMessage spork;
        vRecv >> spork;

        if (chainActive.Tip() == NULL) return;

        // Ignore spork messages about unknown/deleted sporks
        std::string strSpork = sporkManager.GetSporkNameByID(spork.nSporkID);
        if (strSpork == "Unknown") return;

        uint256 hash = spork.GetHash();
        if (mapSporksActive.count(spork.nSporkID)) {
            if (mapSporksActive[spork.nSporkID].nTimeSigned >= spork.nTimeSigned) {
                if (fDebug) LogPrintf("spork - seen %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
                return;
            } else {
                if (fDebug) LogPrintf("spork - got updated spork %s block %d \n", hash.ToString(), chainActive.Tip()->nHeight);
            }
        }

        LogPrintf("spork - new %s ID %d Time %d bestHeight %d\n", hash.ToString(), spork.nSporkID, spork.nValue, chainActive.Tip()->nHeight);

        if (!sporkManager.CheckSignature(spork)) {
            LogPrintf("spork - invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        mapSporks[hash] = spork;
        mapSporksActive[spork.nSporkID] = spork;
        sporkManager.Relay(spork);

        // Catocoin: add to spork database.
        pSporkDB->WriteSpork(spork.nSporkID, spork);
    }
    if (strCommand == "getsporks") {
        std::map<int, CSporkMessage>::iterator it = mapSporksActive.begin();

        while (it != mapSporksActive.end()) {
            pfrom->PushMessage("spork", it->second);
            it++;
        }
    }
}


// grab the value of the spork on the network, or the default
int64_t GetSporkValue(int nSporkID)
{
    int64_t r = -1;

    if (mapSporksActive.count(nSporkID)) {
        r = mapSporksActive[nSporkID].nValue;
    } else {
        if (nSporkID == SPORK_2_SWIFTTX) r = SPORK_2_SWIFTTX_DEFAULT;
        if (nSporkID == SPORK_3_SWIFTTX_BLOCK_FILTERING) r = SPORK_3_SWIFTTX_BLOCK_FILTERING_DEFAULT;
        if (nSporkID == SPORK_5_MAX_VALUE) r = SPORK_5_MAX_VALUE_DEFAULT;
        if (nSporkID == SPORK_7_MASTERNODE_SCANNING) r = SPORK_7_MASTERNODE_SCANNING_DEFAULT;
        if (nSporkID == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT) r = SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT_DEFAULT;
        if (nSporkID == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT) r = SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT_DEFAULT;
        if (nSporkID == SPORK_10_MASTERNODE_PAY_UPDATED_NODES) r = SPORK_10_MASTERNODE_PAY_UPDATED_NODES_DEFAULT;
        if (nSporkID == SPORK_11_LOCK_INVALID_UTXO) r = SPORK_11_LOCK_INVALID_UTXO_DEFAULT;
        if (nSporkID == SPORK_13_ENABLE_SUPERBLOCKS) r = SPORK_13_ENABLE_SUPERBLOCKS_DEFAULT;
        if (nSporkID == SPORK_14_NEW_PROTOCOL_ENFORCEMENT) r = SPORK_14_NEW_PROTOCOL_ENFORCEMENT_DEFAULT;
        if (nSporkID == SPORK_15_NEW_PROTOCOL_ENFORCEMENT_2) r = SPORK_15_NEW_PROTOCOL_ENFORCEMENT_2_DEFAULT;
        if (nSporkID == SPORK_16_ZEROCOIN_MAINTENANCE_MODE) r = SPORK_16_ZEROCOIN_MAINTENANCE_MODE_DEFAULT;
	if (nSporkID == SPORK_17_CURRENT_MN_COLLATERAL) r = SPORK_17_CURRENT_MN_COLLATERAL_DEFAULT;
	if (nSporkID == SPORK_18_LAST_2000_COLLAT_BLOCK) r = SPORK_18_LAST_2000_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_19_LAST_2400_COLLAT_BLOCK) r = SPORK_19_LAST_2400_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_20_LAST_2550_COLLAT_BLOCK) r = SPORK_20_LAST_2550_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_21_LAST_2750_COLLAT_BLOCK) r = SPORK_21_LAST_2750_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_22_LAST_2950_COLLAT_BLOCK) r = SPORK_22_LAST_2950_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_23_LAST_3150_COLLAT_BLOCK) r = SPORK_23_LAST_3150_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_24_LAST_3350_COLLAT_BLOCK) r = SPORK_24_LAST_3350_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_25_LAST_3600_COLLAT_BLOCK) r = SPORK_25_LAST_3600_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_26_LAST_3850_COLLAT_BLOCK) r = SPORK_26_LAST_3850_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_27_LAST_4150_COLLAT_BLOCK) r = SPORK_27_LAST_4150_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_28_LAST_4400_COLLAT_BLOCK) r = SPORK_28_LAST_4400_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_29_LAST_4750_COLLAT_BLOCK) r = SPORK_29_LAST_4750_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_30_LAST_5050_COLLAT_BLOCK) r = SPORK_30_LAST_5050_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_31_LAST_5400_COLLAT_BLOCK) r = SPORK_31_LAST_5400_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_32_LAST_5800_COLLAT_BLOCK) r = SPORK_32_LAST_5800_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_33_LAST_6200_COLLAT_BLOCK) r = SPORK_33_LAST_6200_COLLAT_BLOCK_DEFAULT;
	if (nSporkID == SPORK_34_LAST_6600_COLLAT_BLOCK) r = SPORK_34_LAST_6600_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_35_MOVE_REWARDS) r = SPORK_35_MOVE_REWARDS_DEFAULT;
	if (nSporkID == SPORK_36_LAST_2200_COLLAT_BLOCK) r = SPORK_36_LAST_2200_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_37_LAST_25000_COLLAT_BLOCK) r = SPORK_37_LAST_25000_COLLAT_BLOCK_DEFAULT;
//        if (nSporkID == SPORK_36_LAST_2200_COLLAT_BLOCK) r = SPORK_36_LAST_2200_COLLAT_BLOCK_DEFAULT;
  //      if (nSporkID == SPORK_36_LAST_2200_COLLAT_BLOCK) r = SPORK_36_LAST_2200_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_38_LAST_26250_COLLAT_BLOCK) r = SPORK_38_LAST_26250_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_39_LAST_27575_COLLAT_BLOCK) r = SPORK_39_LAST_27575_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_40_LAST_28950_COLLAT_BLOCK) r = SPORK_40_LAST_28950_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_41_LAST_30400_COLLAT_BLOCK) r = SPORK_41_LAST_30400_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_42_LAST_31900_COLLAT_BLOCK) r = SPORK_42_LAST_31900_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_43_LAST_33500_COLLAT_BLOCK) r = SPORK_43_LAST_33500_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_44_LAST_35175_COLLAT_BLOCK) r = SPORK_44_LAST_35175_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_45_LAST_36925_COLLAT_BLOCK) r = SPORK_45_LAST_36925_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_46_LAST_38775_COLLAT_BLOCK) r = SPORK_46_LAST_38775_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_47_LAST_40725_COLLAT_BLOCK) r = SPORK_47_LAST_40725_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_48_LAST_42750_COLLAT_BLOCK) r = SPORK_48_LAST_42750_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_49_LAST_44900_COLLAT_BLOCK) r = SPORK_49_LAST_44900_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_50_LAST_47150_COLLAT_BLOCK) r = SPORK_50_LAST_47150_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_51_LAST_49500_COLLAT_BLOCK) r = SPORK_51_LAST_49500_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_52_LAST_51975_COLLAT_BLOCK) r = SPORK_52_LAST_51975_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_53_LAST_54575_COLLAT_BLOCK) r = SPORK_53_LAST_54575_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_54_LAST_57304_COLLAT_BLOCK) r = SPORK_54_LAST_57304_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_55_LAST_60169_COLLAT_BLOCK) r = SPORK_55_LAST_60169_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_56_LAST_63175_COLLAT_BLOCK) r = SPORK_56_LAST_63175_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_57_LAST_66325_COLLAT_BLOCK) r = SPORK_57_LAST_66325_COLLAT_BLOCK_DEFAULT;
        if (nSporkID == SPORK_58_LAST_69650_COLLAT_BLOCK) r = SPORK_58_LAST_69650_COLLAT_BLOCK_DEFAULT;

        if (nSporkID == SPORK_59_CURRENT_MN_COLLATERAL) r = SPORK_59_CURRENT_MN_COLLATERAL_DEFAULT;
        if (nSporkID == SPORK_60_CURRENT_MN_COLLATERAL) r = SPORK_60_CURRENT_MN_COLLATERAL_DEFAULT;



        if (r == -1) LogPrintf("GetSpork::Unknown Spork %d\n", nSporkID);
    }

    return r;
}

// grab the spork value, and see if it's off
bool IsSporkActive(int nSporkID)
{
    int64_t r = GetSporkValue(nSporkID);
    if (r == -1) return false;
    return r < GetTime();
}


void ReprocessBlocks(int nBlocks)
{
    std::map<uint256, int64_t>::iterator it = mapRejectedBlocks.begin();
    while (it != mapRejectedBlocks.end()) {
        //use a window twice as large as is usual for the nBlocks we want to reset
        if ((*it).second > GetTime() - (nBlocks * 60 * 5)) {
            BlockMap::iterator mi = mapBlockIndex.find((*it).first);
            if (mi != mapBlockIndex.end() && (*mi).second) {
                LOCK(cs_main);

                CBlockIndex* pindex = (*mi).second;
                LogPrintf("ReprocessBlocks - %s\n", (*it).first.ToString());

                CValidationState state;
                ReconsiderBlock(state, pindex);
            }
        }
        ++it;
    }

    CValidationState state;
    {
        LOCK(cs_main);
        DisconnectBlocksAndReprocess(nBlocks);
    }

    if (state.IsValid()) {
        ActivateBestChain(state);
    }
}

bool CSporkManager::CheckSignature(CSporkMessage& spork)
{
    //note: need to investigate why this is failing
    std::string strMessage = boost::lexical_cast<std::string>(spork.nSporkID) + boost::lexical_cast<std::string>(spork.nValue) + boost::lexical_cast<std::string>(spork.nTimeSigned);

//    string strSporkKey = "04D5B4C055667586CC8CEBFE6DAA0CEF55B50F1D27DD9F2018596EDD9F9C2B7301925BA63E7A29D8B34F8B2870C5364F2DEBABB85E15DABC06596B6EBF47665C68";
string strSporkKey = "02d0373ad80b108f0697b961bbccf85a4ed55bdb6cba961cf90fcac264ea289e77";
    //CPubKey pubkeynew(ParseHex(Params().SporkKey()));
    CPubKey pubkeynew(ParseHex(strSporkKey));
    //cout << "CheckSignature: " << pubkeynew.GetHex() << "\n";
    std::string errorMessage = "";
    if (obfuScationSigner.VerifyMessage(pubkeynew, spork.vchSig, strMessage, errorMessage)) {
        return true;
    }

    return false;
}

bool CSporkManager::Sign(CSporkMessage& spork)
{
    std::string strMessage = boost::lexical_cast<std::string>(spork.nSporkID) + boost::lexical_cast<std::string>(spork.nValue) + boost::lexical_cast<std::string>(spork.nTimeSigned);

    CKey key2;
    CPubKey pubkey2;
    std::string errorMessage = "";

    if (!obfuScationSigner.SetKey(strMasterPrivKey, errorMessage, key2, pubkey2)) {
        LogPrintf("CMasternodePayments::Sign - ERROR: Invalid masternodeprivkey: '%s'\n", errorMessage);
        return false;
    }

    if (!obfuScationSigner.SignMessage(strMessage, errorMessage, spork.vchSig, key2)) {
        LogPrintf("CMasternodePayments::Sign - Sign message failed");
        return false;
    }

    if (!obfuScationSigner.VerifyMessage(pubkey2, spork.vchSig, strMessage, errorMessage)) {
        LogPrintf("CMasternodePayments::Sign - Verify message failed");
        return false;
    }

    return true;
}

bool CSporkManager::UpdateSpork(int nSporkID, int64_t nValue)
{
    CSporkMessage msg;
    msg.nSporkID = nSporkID;
    msg.nValue = nValue;
    msg.nTimeSigned = GetTime();

    if (Sign(msg)) {
        Relay(msg);
        mapSporks[msg.GetHash()] = msg;
        mapSporksActive[nSporkID] = msg;
        return true;
    }

    return false;
}

void CSporkManager::Relay(CSporkMessage& msg)
{
    CInv inv(MSG_SPORK, msg.GetHash());
    RelayInv(inv);
}

bool CSporkManager::SetPrivKey(std::string strPrivKey)
{
    CSporkMessage msg;

    // Test signing successful, proceed
    strMasterPrivKey = strPrivKey;

    Sign(msg);

    if (CheckSignature(msg)) {
        LogPrintf("CSporkManager::SetPrivKey - Successfully initialized as spork signer\n");
        return true;
    } else {
        return false;
    }
}

int CSporkManager::GetSporkIDByName(std::string strName)
{
    if (strName == "SPORK_2_SWIFTTX") return SPORK_2_SWIFTTX;
    if (strName == "SPORK_3_SWIFTTX_BLOCK_FILTERING") return SPORK_3_SWIFTTX_BLOCK_FILTERING;
    if (strName == "SPORK_5_MAX_VALUE") return SPORK_5_MAX_VALUE;
    if (strName == "SPORK_7_MASTERNODE_SCANNING") return SPORK_7_MASTERNODE_SCANNING;
    if (strName == "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT") return SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT;
    if (strName == "SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT") return SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT;
    if (strName == "SPORK_10_MASTERNODE_PAY_UPDATED_NODES") return SPORK_10_MASTERNODE_PAY_UPDATED_NODES;
    if (strName == "SPORK_11_LOCK_INVALID_UTXO") return SPORK_11_LOCK_INVALID_UTXO;
    if (strName == "SPORK_13_ENABLE_SUPERBLOCKS") return SPORK_13_ENABLE_SUPERBLOCKS;
    if (strName == "SPORK_14_NEW_PROTOCOL_ENFORCEMENT") return SPORK_14_NEW_PROTOCOL_ENFORCEMENT;
    if (strName == "SPORK_15_NEW_PROTOCOL_ENFORCEMENT_2") return SPORK_15_NEW_PROTOCOL_ENFORCEMENT_2;
    if (strName == "SPORK_16_ZEROCOIN_MAINTENANCE_MODE") return SPORK_16_ZEROCOIN_MAINTENANCE_MODE;
        if (strName == "SPORK_17_CURRENT_MN_COLLATERAL") return SPORK_17_CURRENT_MN_COLLATERAL;
        if (strName == "SPORK_18_LAST_2000_COLLAT_BLOCK") return SPORK_18_LAST_2000_COLLAT_BLOCK;
        if (strName == "SPORK_19_LAST_2400_COLLAT_BLOCK") return SPORK_19_LAST_2400_COLLAT_BLOCK;
        if (strName == "SPORK_20_LAST_2550_COLLAT_BLOCK") return SPORK_20_LAST_2550_COLLAT_BLOCK;
        if (strName == "SPORK_21_LAST_2750_COLLAT_BLOCK") return SPORK_21_LAST_2750_COLLAT_BLOCK;
        if (strName == "SPORK_22_LAST_2950_COLLAT_BLOCK") return SPORK_22_LAST_2950_COLLAT_BLOCK;
        if (strName == "SPORK_23_LAST_3150_COLLAT_BLOCK") return SPORK_23_LAST_3150_COLLAT_BLOCK;
        if (strName == "SPORK_24_LAST_3350_COLLAT_BLOCK") return SPORK_24_LAST_3350_COLLAT_BLOCK;
        if (strName == "SPORK_25_LAST_3600_COLLAT_BLOCK") return SPORK_25_LAST_3600_COLLAT_BLOCK;
        if (strName == "SPORK_26_LAST_3850_COLLAT_BLOCK") return SPORK_26_LAST_3850_COLLAT_BLOCK;
        if (strName == "SPORK_27_LAST_4150_COLLAT_BLOCK") return SPORK_27_LAST_4150_COLLAT_BLOCK;
        if (strName == "SPORK_28_LAST_4400_COLLAT_BLOCK") return SPORK_28_LAST_4400_COLLAT_BLOCK;
        if (strName == "SPORK_29_LAST_4750_COLLAT_BLOCK") return SPORK_29_LAST_4750_COLLAT_BLOCK;
        if (strName == "SPORK_30_LAST_5050_COLLAT_BLOCK") return SPORK_30_LAST_5050_COLLAT_BLOCK;
        if (strName == "SPORK_31_LAST_5400_COLLAT_BLOCK") return SPORK_31_LAST_5400_COLLAT_BLOCK;
        if (strName == "SPORK_32_LAST_5800_COLLAT_BLOCK") return SPORK_32_LAST_5800_COLLAT_BLOCK;
        if (strName == "SPORK_33_LAST_6200_COLLAT_BLOCK") return SPORK_33_LAST_6200_COLLAT_BLOCK;
        if (strName == "SPORK_34_LAST_6600_COLLAT_BLOCK") return SPORK_34_LAST_6600_COLLAT_BLOCK;
        if (strName == "SPORK_35_MOVE_REWARDS") return SPORK_35_MOVE_REWARDS;
        if (strName == "SPORK_36_LAST_2200_COLLAT_BLOCK")  return SPORK_36_LAST_2200_COLLAT_BLOCK;
if (strName == "SPORK_37_LAST_25000_COLLAT_BLOCK") return SPORK_37_LAST_25000_COLLAT_BLOCK;
if (strName == "SPORK_38_LAST_26250_COLLAT_BLOCK") return SPORK_38_LAST_26250_COLLAT_BLOCK;
        if (strName == "SPORK_39_LAST_27575_COLLAT_BLOCK") return SPORK_39_LAST_27575_COLLAT_BLOCK;
        if (strName == "SPORK_40_LAST_28950_COLLAT_BLOCK") return SPORK_40_LAST_28950_COLLAT_BLOCK;
        if (strName == "SPORK_41_LAST_30400_COLLAT_BLOCK") return SPORK_41_LAST_30400_COLLAT_BLOCK;
        if (strName == "SPORK_42_LAST_31900_COLLAT_BLOCK") return SPORK_42_LAST_31900_COLLAT_BLOCK;
        if (strName == "SPORK_43_LAST_33500_COLLAT_BLOCK") return SPORK_43_LAST_33500_COLLAT_BLOCK;
        if (strName == "SPORK_44_LAST_35175_COLLAT_BLOCK") return SPORK_44_LAST_35175_COLLAT_BLOCK;
        if (strName == "SPORK_45_LAST_36925_COLLAT_BLOCK") return SPORK_45_LAST_36925_COLLAT_BLOCK;
        if (strName == "SPORK_46_LAST_38775_COLLAT_BLOCK") return SPORK_46_LAST_38775_COLLAT_BLOCK;
        if (strName == "SPORK_47_LAST_40725_COLLAT_BLOCK") return SPORK_47_LAST_40725_COLLAT_BLOCK;
        if (strName == "SPORK_48_LAST_42750_COLLAT_BLOCK") return SPORK_48_LAST_42750_COLLAT_BLOCK;
        if (strName == "SPORK_49_LAST_44900_COLLAT_BLOCK") return SPORK_49_LAST_44900_COLLAT_BLOCK;
        if (strName == "SPORK_50_LAST_47150_COLLAT_BLOCK") return SPORK_50_LAST_47150_COLLAT_BLOCK;
        if (strName == "SPORK_51_LAST_49500_COLLAT_BLOCK") return SPORK_51_LAST_49500_COLLAT_BLOCK;
        if (strName == "SPORK_52_LAST_51975_COLLAT_BLOCK") return SPORK_52_LAST_51975_COLLAT_BLOCK;
        if (strName == "SPORK_53_LAST_54575_COLLAT_BLOCK") return SPORK_53_LAST_54575_COLLAT_BLOCK;
        if (strName == "SPORK_54_LAST_57304_COLLAT_BLOCK") return SPORK_54_LAST_57304_COLLAT_BLOCK;
        if (strName == "SPORK_55_LAST_60169_COLLAT_BLOCK") return SPORK_55_LAST_60169_COLLAT_BLOCK;
        if (strName == "SPORK_56_LAST_63175_COLLAT_BLOCK") return SPORK_56_LAST_63175_COLLAT_BLOCK;
        if (strName == "SPORK_57_LAST_66325_COLLAT_BLOCK") return SPORK_57_LAST_66325_COLLAT_BLOCK;
        if (strName == "SPORK_58_LAST_69650_COLLAT_BLOCK") return SPORK_58_LAST_69650_COLLAT_BLOCK;
        if (strName == "SPORK_59_CURRENT_MN_COLLATERAL") return SPORK_59_CURRENT_MN_COLLATERAL;
        if (strName == "SPORK_60_CURRENT_MN_COLLATERAL") return SPORK_60_CURRENT_MN_COLLATERAL;

    return -1;
}

std::string CSporkManager::GetSporkNameByID(int id)
{
    if (id == SPORK_2_SWIFTTX) return "SPORK_2_SWIFTTX";
    if (id == SPORK_3_SWIFTTX_BLOCK_FILTERING) return "SPORK_3_SWIFTTX_BLOCK_FILTERING";
    if (id == SPORK_5_MAX_VALUE) return "SPORK_5_MAX_VALUE";
    if (id == SPORK_7_MASTERNODE_SCANNING) return "SPORK_7_MASTERNODE_SCANNING";
    if (id == SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT) return "SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT";
    if (id == SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT) return "SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT";
    if (id == SPORK_10_MASTERNODE_PAY_UPDATED_NODES) return "SPORK_10_MASTERNODE_PAY_UPDATED_NODES";
    if (id == SPORK_11_LOCK_INVALID_UTXO) return "SPORK_11_LOCK_INVALID_UTXO";
    if (id == SPORK_13_ENABLE_SUPERBLOCKS) return "SPORK_13_ENABLE_SUPERBLOCKS";
    if (id == SPORK_14_NEW_PROTOCOL_ENFORCEMENT) return "SPORK_14_NEW_PROTOCOL_ENFORCEMENT";
    if (id == SPORK_15_NEW_PROTOCOL_ENFORCEMENT_2) return "SPORK_15_NEW_PROTOCOL_ENFORCEMENT_2";
    if (id == SPORK_16_ZEROCOIN_MAINTENANCE_MODE) return "SPORK_16_ZEROCOIN_MAINTENANCE_MODE";
        if (id == SPORK_17_CURRENT_MN_COLLATERAL) return "SPORK_17_CURRENT_MN_COLLATERAL";
        if (id == SPORK_18_LAST_2000_COLLAT_BLOCK) return "SPORK_18_LAST_2000_COLLAT_BLOCK";
        if (id == SPORK_19_LAST_2400_COLLAT_BLOCK) return "SPORK_19_LAST_2400_COLLAT_BLOCK";
        if (id == SPORK_20_LAST_2550_COLLAT_BLOCK) return "SPORK_20_LAST_2550_COLLAT_BLOCK";
        if (id == SPORK_21_LAST_2750_COLLAT_BLOCK) return "SPORK_21_LAST_2750_COLLAT_BLOCK";
        if (id == SPORK_22_LAST_2950_COLLAT_BLOCK) return "SPORK_22_LAST_2950_COLLAT_BLOCK";
        if (id == SPORK_23_LAST_3150_COLLAT_BLOCK) return "SPORK_23_LAST_3150_COLLAT_BLOCK";
        if (id == SPORK_24_LAST_3350_COLLAT_BLOCK) return "SPORK_24_LAST_3350_COLLAT_BLOCK";
        if (id == SPORK_25_LAST_3600_COLLAT_BLOCK) return "SPORK_25_LAST_3600_COLLAT_BLOCK";
        if (id == SPORK_26_LAST_3850_COLLAT_BLOCK) return "SPORK_26_LAST_3850_COLLAT_BLOCK";
        if (id == SPORK_27_LAST_4150_COLLAT_BLOCK) return "SPORK_27_LAST_4150_COLLAT_BLOCK";
        if (id == SPORK_28_LAST_4400_COLLAT_BLOCK) return "SPORK_28_LAST_4400_COLLAT_BLOCK";
        if (id == SPORK_29_LAST_4750_COLLAT_BLOCK) return "SPORK_29_LAST_4750_COLLAT_BLOCK";
        if (id == SPORK_30_LAST_5050_COLLAT_BLOCK) return "SPORK_30_LAST_5050_COLLAT_BLOCK";
        if (id == SPORK_31_LAST_5400_COLLAT_BLOCK) return "SPORK_31_LAST_5400_COLLAT_BLOCK";
        if (id == SPORK_32_LAST_5800_COLLAT_BLOCK) return "SPORK_32_LAST_5800_COLLAT_BLOCK";
        if (id == SPORK_33_LAST_6200_COLLAT_BLOCK) return "SPORK_33_LAST_6200_COLLAT_BLOCK";
        if (id == SPORK_34_LAST_6600_COLLAT_BLOCK) return "SPORK_34_LAST_6600_COLLAT_BLOCK";
        if (id == SPORK_35_MOVE_REWARDS) return "SPORK_35_MOVE_REWARDS";
        if (id == SPORK_36_LAST_2200_COLLAT_BLOCK) return "SPORK_36_LAST_2200_COLLAT_BLOCK";


if (id == SPORK_37_LAST_25000_COLLAT_BLOCK) return "SPORK_37_LAST_25000_COLLAT_BLOCK";
if (id == SPORK_38_LAST_26250_COLLAT_BLOCK) return "SPORK_38_LAST_26250_COLLAT_BLOCK";
        if (id == SPORK_39_LAST_27575_COLLAT_BLOCK) return "SPORK_39_LAST_27575_COLLAT_BLOCK";
        if (id == SPORK_40_LAST_28950_COLLAT_BLOCK) return "SPORK_40_LAST_28950_COLLAT_BLOCK";
        if (id == SPORK_41_LAST_30400_COLLAT_BLOCK) return "SPORK_41_LAST_30400_COLLAT_BLOCK";
        if (id == SPORK_42_LAST_31900_COLLAT_BLOCK) return "SPORK_42_LAST_31900_COLLAT_BLOCK";
        if (id == SPORK_43_LAST_33500_COLLAT_BLOCK) return "SPORK_43_LAST_33500_COLLAT_BLOCK";
        if (id == SPORK_44_LAST_35175_COLLAT_BLOCK) return "SPORK_44_LAST_35175_COLLAT_BLOCK";
        if (id == SPORK_45_LAST_36925_COLLAT_BLOCK) return "SPORK_45_LAST_36925_COLLAT_BLOCK";
        if (id == SPORK_46_LAST_38775_COLLAT_BLOCK) return "SPORK_46_LAST_38775_COLLAT_BLOCK";
        if (id == SPORK_47_LAST_40725_COLLAT_BLOCK) return "SPORK_47_LAST_40725_COLLAT_BLOCK";
        if (id == SPORK_48_LAST_42750_COLLAT_BLOCK) return "SPORK_48_LAST_42750_COLLAT_BLOCK";
        if (id == SPORK_49_LAST_44900_COLLAT_BLOCK) return "SPORK_49_LAST_44900_COLLAT_BLOCK";
        if (id == SPORK_50_LAST_47150_COLLAT_BLOCK) return "SPORK_50_LAST_47150_COLLAT_BLOCK";
        if (id == SPORK_51_LAST_49500_COLLAT_BLOCK) return "SPORK_51_LAST_49500_COLLAT_BLOCK";
        if (id == SPORK_52_LAST_51975_COLLAT_BLOCK) return "SPORK_52_LAST_51975_COLLAT_BLOCK";
        if (id == SPORK_53_LAST_54575_COLLAT_BLOCK) return "SPORK_53_LAST_54575_COLLAT_BLOCK";
        if (id == SPORK_54_LAST_57304_COLLAT_BLOCK) return "SPORK_54_LAST_57304_COLLAT_BLOCK";
        if (id == SPORK_55_LAST_60169_COLLAT_BLOCK) return "SPORK_55_LAST_60169_COLLAT_BLOCK";
        if (id == SPORK_56_LAST_63175_COLLAT_BLOCK) return "SPORK_56_LAST_63175_COLLAT_BLOCK";
        if (id == SPORK_57_LAST_66325_COLLAT_BLOCK) return "SPORK_57_LAST_66325_COLLAT_BLOCK";
        if (id == SPORK_58_LAST_69650_COLLAT_BLOCK) return "SPORK_58_LAST_69650_COLLAT_BLOCK";
        if (id == SPORK_59_CURRENT_MN_COLLATERAL) return "SPORK_59_CURRENT_MN_COLLATERAL";
        if (id == SPORK_60_CURRENT_MN_COLLATERAL) return "SPORK_60_CURRENT_MN_COLLATERAL";


    return "Unknown";
}      //  if (strName == "SPORK_17_CURRENT_MN_COLLATERAL") return SPORK_17_CURRENT_MN_COLLATERAL;

