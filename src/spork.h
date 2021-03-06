// Copyright (c) 2014-2016 The Dash developers
// Copyright (c) 2016-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SPORK_H
#define SPORK_H

#include "base58.h"
#include "key.h"
#include "main.h"
#include "net.h"
#include "sync.h"
#include "util.h"

#include "obfuscation.h"
#include "protocol.h"
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

/*
    Don't ever reuse these IDs for other sporks
    - This would result in old clients getting confused about which spork is for what

    Sporks 11,12, and 16 to be removed with 1st zerocoin release
*/
#define SPORK_START 10001
#define SPORK_END 10059

#define SPORK_2_SWIFTTX 10001
#define SPORK_3_SWIFTTX_BLOCK_FILTERING 10002
#define SPORK_5_MAX_VALUE 10004
#define SPORK_7_MASTERNODE_SCANNING 10006
#define SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT 10007
#define SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT 10008
#define SPORK_10_MASTERNODE_PAY_UPDATED_NODES 10009
#define SPORK_11_LOCK_INVALID_UTXO 10010
//#define SPORK_12_RECONSIDER_BLOCKS 10011
#define SPORK_13_ENABLE_SUPERBLOCKS 10012
#define SPORK_14_NEW_PROTOCOL_ENFORCEMENT 10013
#define SPORK_15_NEW_PROTOCOL_ENFORCEMENT_2 10014
#define SPORK_16_ZEROCOIN_MAINTENANCE_MODE 10015
#define SPORK_17_CURRENT_MN_COLLATERAL 10016
#define SPORK_18_LAST_2000_COLLAT_BLOCK 10017
#define SPORK_19_LAST_2400_COLLAT_BLOCK 10018
#define SPORK_20_LAST_2550_COLLAT_BLOCK 10019
#define SPORK_21_LAST_2750_COLLAT_BLOCK 10020
#define SPORK_22_LAST_2950_COLLAT_BLOCK 10021
#define SPORK_23_LAST_3150_COLLAT_BLOCK 10022
#define SPORK_24_LAST_3350_COLLAT_BLOCK 10023
#define SPORK_25_LAST_3600_COLLAT_BLOCK 10024
#define SPORK_26_LAST_3850_COLLAT_BLOCK 10025
#define SPORK_27_LAST_4150_COLLAT_BLOCK 10026
#define SPORK_28_LAST_4400_COLLAT_BLOCK 10027
#define SPORK_29_LAST_4750_COLLAT_BLOCK 10028
#define SPORK_30_LAST_5050_COLLAT_BLOCK 10029
#define SPORK_31_LAST_5400_COLLAT_BLOCK 10030
#define SPORK_32_LAST_5800_COLLAT_BLOCK 10031
#define SPORK_33_LAST_6200_COLLAT_BLOCK 10032
#define SPORK_34_LAST_6600_COLLAT_BLOCK 10033
#define SPORK_35_MOVE_REWARDS 10034
#define SPORK_36_LAST_2200_COLLAT_BLOCK 10035
#define SPORK_37_LAST_25000_COLLAT_BLOCK 10036
//#define SPORK_36_LAST_2200_COLLAT_BLOCK 9999999 SPORK_36_LAST_2200_COLLAT_BLOCK;
  //      if (nSporkID == SPORK_36_LAST_2200_COLLAT_BLOCK 9999999 SPORK_36_LAST_2200_COLLAT_BLOCK;
#define SPORK_38_LAST_26250_COLLAT_BLOCK 10037
// SPORK_38_LAST_26250_COLLAT_BLOCK;
#define SPORK_39_LAST_27575_COLLAT_BLOCK 10038
//SPORK_39_LAST_27575_
#define SPORK_40_LAST_28950_COLLAT_BLOCK 10039
//SPORK_40_LAST_28950_
#define SPORK_41_LAST_30400_COLLAT_BLOCK 10040
//SPORK_41_LAST_30400_
#define SPORK_42_LAST_31900_COLLAT_BLOCK 10041
//SPORK_42_LAST_31900_
#define SPORK_43_LAST_33500_COLLAT_BLOCK 10042
//SPORK_43_LAST_33500_
#define SPORK_44_LAST_35175_COLLAT_BLOCK 10043
//SPORK_44_LAST_35175_
#define SPORK_45_LAST_36925_COLLAT_BLOCK 10044
//SPORK_45_LAST_36925_
#define SPORK_46_LAST_38775_COLLAT_BLOCK 10045
//SPORK_46_LAST_38775_
#define SPORK_47_LAST_40725_COLLAT_BLOCK 10046
//SPORK_47_LAST_40725_
#define SPORK_48_LAST_42750_COLLAT_BLOCK 10047
//SPORK_48_LAST_42750_
#define SPORK_49_LAST_44900_COLLAT_BLOCK 10048
//SPORK_49_LAST_44900_
#define SPORK_50_LAST_47150_COLLAT_BLOCK 10049
//SPORK_50_LAST_47150_
#define SPORK_51_LAST_49500_COLLAT_BLOCK 10050
//SPORK_51_LAST_49500_
#define SPORK_52_LAST_51975_COLLAT_BLOCK 10051
//SPORK_52_LAST_51975_
#define SPORK_53_LAST_54575_COLLAT_BLOCK 10052
//SPORK_53_LAST_54575_
#define SPORK_54_LAST_57304_COLLAT_BLOCK 10053
//SPORK_54_LAST_57304_
#define SPORK_55_LAST_60169_COLLAT_BLOCK 10054
//SPORK_55_LAST_6016
#define SPORK_56_LAST_63175_COLLAT_BLOCK 10055
//SPORK_56_LAST_63175_
#define SPORK_57_LAST_66325_COLLAT_BLOCK 10056
//SPORK_57_LAST_66325_
#define SPORK_58_LAST_69650_COLLAT_BLOCK 10057

#define SPORK_59_CURRENT_MN_COLLATERAL 10058
#define SPORK_60_CURRENT_MN_COLLATERAL 10059
//#define SPORK_58_CURRENT_MN_COLLATERAL 10059


#define SPORK_2_SWIFTTX_DEFAULT 978307200                         //2001-1-1
#define SPORK_3_SWIFTTX_BLOCK_FILTERING_DEFAULT 1424217600        //2015-2-18
#define SPORK_5_MAX_VALUE_DEFAULT 1000                            //1000 CATO
#define SPORK_7_MASTERNODE_SCANNING_DEFAULT 978307200             //2001-1-1
#define SPORK_8_MASTERNODE_PAYMENT_ENFORCEMENT_DEFAULT 4070908800 //OFF
#define SPORK_9_MASTERNODE_BUDGET_ENFORCEMENT_DEFAULT 4070908800  //OFF
#define SPORK_10_MASTERNODE_PAY_UPDATED_NODES_DEFAULT 4070908800  //OFF
#define SPORK_11_LOCK_INVALID_UTXO_DEFAULT 4070908800             //OFF - NOTE: this is block height not time!
#define SPORK_13_ENABLE_SUPERBLOCKS_DEFAULT 4070908800            //OFF
#define SPORK_14_NEW_PROTOCOL_ENFORCEMENT_DEFAULT 4070908800      //OFF
#define SPORK_15_NEW_PROTOCOL_ENFORCEMENT_2_DEFAULT 4070908800    //OFF
#define SPORK_16_ZEROCOIN_MAINTENANCE_MODE_DEFAULT 4070908800     //OFF
#define SPORK_17_CURRENT_MN_COLLATERAL_DEFAULT 2000
#define SPORK_18_LAST_2000_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_19_LAST_2400_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_20_LAST_2550_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_21_LAST_2750_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_22_LAST_2950_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_23_LAST_3150_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_24_LAST_3350_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_25_LAST_3600_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_26_LAST_3850_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_27_LAST_4150_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_28_LAST_4400_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_29_LAST_4750_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_30_LAST_5050_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_31_LAST_5400_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_32_LAST_5800_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_33_LAST_6200_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_34_LAST_6600_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_35_MOVE_REWARDS_DEFAULT 4200
#define SPORK_36_LAST_2200_COLLAT_BLOCK_DEFAULT 959999
#define SPORK_37_LAST_25000_COLLAT_BLOCK_DEFAULT 959999
//#define SPORK_36_LAST_2200_COLLAT_BLOCK_DEFAULT 9999999 SPORK_36_LAST_2200_COLLAT_BLOCK_DEFAULT_DEFAULT;
  //      if (nSporkID == SPORK_36_LAST_2200_COLLAT_BLOCK_DEFAULT 9999999 SPORK_36_LAST_2200_COLLAT_BLOCK_DEFAULT_DEFAULT;
#define SPORK_38_LAST_26250_COLLAT_BLOCK_DEFAULT 9999999
// SPORK_38_LAST_26250_COLLAT_BLOCK_DEFAULT;
#define SPORK_39_LAST_27575_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_39_LAST_27575_
#define SPORK_40_LAST_28950_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_40_LAST_28950_
#define SPORK_41_LAST_30400_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_41_LAST_30400_
#define SPORK_42_LAST_31900_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_42_LAST_31900_
#define SPORK_43_LAST_33500_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_43_LAST_33500_
#define SPORK_44_LAST_35175_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_44_LAST_35175_
#define SPORK_45_LAST_36925_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_45_LAST_36925_
#define SPORK_46_LAST_38775_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_46_LAST_38775_
#define SPORK_47_LAST_40725_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_47_LAST_40725_
#define SPORK_48_LAST_42750_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_48_LAST_42750_
#define SPORK_49_LAST_44900_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_49_LAST_44900_
#define SPORK_50_LAST_47150_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_50_LAST_47150_
#define SPORK_51_LAST_49500_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_51_LAST_49500_
#define SPORK_52_LAST_51975_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_52_LAST_51975_
#define SPORK_53_LAST_54575_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_53_LAST_54575_
#define SPORK_54_LAST_57304_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_54_LAST_57304_
#define SPORK_55_LAST_60169_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_55_LAST_60169_
#define SPORK_56_LAST_63175_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_56_LAST_63175_
#define SPORK_57_LAST_66325_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_57_LAST_66325_
#define SPORK_58_LAST_69650_COLLAT_BLOCK_DEFAULT 9999999 
//SPORK_58_LAST_69650_
#define SPORK_59_CURRENT_MN_COLLATERAL_DEFAULT 9999999
#define SPORK_60_CURRENT_MN_COLLATERAL_DEFAULT 9999999
//#define SPORK_58_LAST_CURRENT_MN_COLLATERAL 9999999

class CSporkMessage;
class CSporkManager;

extern std::map<uint256, CSporkMessage> mapSporks;
extern std::map<int, CSporkMessage> mapSporksActive;
extern CSporkManager sporkManager;

void LoadSporksFromDB();
void ProcessSpork(CNode* pfrom, std::string& strCommand, CDataStream& vRecv);
int64_t GetSporkValue(int nSporkID);
bool IsSporkActive(int nSporkID);
void ReprocessBlocks(int nBlocks);

//
// Spork Class
// Keeps track of all of the network spork settings
//

class CSporkMessage
{
public:
    std::vector<unsigned char> vchSig;
    int nSporkID;
    int64_t nValue;
    int64_t nTimeSigned;

    uint256 GetHash()
    {
        uint256 n = XEVAN(BEGIN(nSporkID), END(nTimeSigned));
        return n;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(nSporkID);
        READWRITE(nValue);
        READWRITE(nTimeSigned);
        READWRITE(vchSig);
    }
};


class CSporkManager
{
private:
    std::vector<unsigned char> vchSig;
    std::string strMasterPrivKey;

public:
    CSporkManager()
    {
    }

    std::string GetSporkNameByID(int id);
    int GetSporkIDByName(std::string strName);
    bool UpdateSpork(int nSporkID, int64_t nValue);
    bool SetPrivKey(std::string strPrivKey);
    bool CheckSignature(CSporkMessage& spork);
    bool Sign(CSporkMessage& spork);
    void Relay(CSporkMessage& msg);
};

#endif
