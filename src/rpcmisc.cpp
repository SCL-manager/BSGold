// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "init.h"
#include "main.h"
#include "net.h"
#include "netbase.h"
#include "rpcserver.h"
#include "util.h"
#include "stealth.h"
#include "spork.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
//#include "walletdb.h"
#endif

#include <stdint.h>
//#include "standard.h"

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

Value getinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getinfo\n"
            "Returns an object containing various state info.");

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    Object obj, diff;
    obj.push_back(json_spirit::Pair("version",       FormatFullVersion()));
    obj.push_back(json_spirit::Pair("protocolversion",(int)PROTOCOL_VERSION));
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        obj.push_back(json_spirit::Pair("walletversion", pwalletMain->GetVersion()));
        obj.push_back(json_spirit::Pair("balance",       ValueFromAmount(pwalletMain->GetBalance())));
        if(!fLiteMode)
            obj.push_back(json_spirit::Pair("darksend_balance", ValueFromAmount(pwalletMain->GetAnonymizedBalance())));
        obj.push_back(json_spirit::Pair("newmint",       ValueFromAmount(pwalletMain->GetNewMint())));
        obj.push_back(json_spirit::Pair("lastreward",   ValueFromAmount(pindexBest->nLastReward)));
        obj.push_back(json_spirit::Pair("stake",         ValueFromAmount(pwalletMain->GetStake())));
    }
#endif

#ifndef LOWMEM
    obj.push_back(json_spirit::Pair("moneysupply",   ValueFromAmount(pindexBest->nMoneySupply)));
#endif
    obj.push_back(json_spirit::Pair("blocks",        (int)nBestHeight));
    obj.push_back(json_spirit::Pair("timeoffset",    (int64_t)GetTimeOffset()));
    obj.push_back(json_spirit::Pair("connections",   (int)vNodes.size()));
    obj.push_back(json_spirit::Pair("proxy",         (proxy.IsValid() ? proxy.proxy.ToStringIPPort() : string())));
    //obj.push_back(Pair("proxy",         (proxy.first.IsValid() ? proxy.first.ToStringIPPort() : string())));
    obj.push_back(json_spirit::Pair("ip",            GetLocalAddress(NULL).ToStringIP()));

    diff.push_back(json_spirit::Pair("proof-of-work",  GetDifficulty()));
    diff.push_back(json_spirit::Pair("proof-of-stake", GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    obj.push_back(json_spirit::Pair("difficulty",    GetDifficulty(GetLastBlockIndex(pindexBest, true))));

    obj.push_back(json_spirit::Pair("testnet",       TestNet()));
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        obj.push_back(json_spirit::Pair("keypoololdest", (int64_t)pwalletMain->GetOldestKeyPoolTime()));
        obj.push_back(json_spirit::Pair("keypoolsize",   (int)pwalletMain->GetKeyPoolSize()));
    }
    obj.push_back(json_spirit::Pair("paytxfee",      ValueFromAmount(nTransactionFee)));
    obj.push_back(json_spirit::Pair("mininput",      ValueFromAmount(nMinimumInputValue)));
    if (pwalletMain && pwalletMain->IsCrypted())
        obj.push_back(json_spirit::Pair("unlocked_until", (int64_t)nWalletUnlockTime));
#endif
    obj.push_back(json_spirit::Pair("errors",        GetWarnings("statusbar")));
    return obj;
}

#ifdef ENABLE_WALLET
class DescribeAddressVisitor : public boost::static_visitor<Object>
{
private:
    isminetype mine;

public:
    DescribeAddressVisitor(isminetype mineIn) : mine(mineIn) {}

    Object operator()(const CNoDestination &dest) const { return Object(); }

    Object operator()(const CKeyID &keyID) const {
        Object obj;
        CPubKey vchPubKey;
        obj.push_back(json_spirit::Pair("isscript", false));
        if (mine == ISMINE_SPENDABLE) {
            pwalletMain->GetPubKey(keyID, vchPubKey);
            obj.push_back(json_spirit::Pair("pubkey", HexStr(vchPubKey)));
            obj.push_back(json_spirit::Pair("iscompressed", vchPubKey.IsCompressed()));
        }
        return obj;
    }

    Object operator()(const CScriptID &scriptID) const {
        Object obj;
        obj.push_back(json_spirit::Pair("isscript", true));
        if (mine != ISMINE_NO) {
            CScript subscript;
            pwalletMain->GetCScript(scriptID, subscript);
            std::vector<CTxDestination> addresses;
          txnouttype whichType;
            int nRequired;
            ExtractDestinations(subscript, whichType, addresses, nRequired);
            obj.push_back(json_spirit::Pair("script", GetTxnOutputType(whichType)));
            obj.push_back(json_spirit::Pair("hex", HexStr(subscript.begin(), subscript.end())));
            Array a;
            BOOST_FOREACH(const CTxDestination& addr, addresses)
                a.push_back(CSocietyGcoinAddress(addr).ToString());
            obj.push_back(json_spirit::Pair("addresses", a));
            if (whichType == TX_MULTISIG)
                obj.push_back(json_spirit::Pair("sigsrequired", nRequired));
        }
        return obj;
    }

    Object operator()(const CStealthAddress &stxAddr) const {
        Object obj;
        obj.push_back(json_spirit::Pair("todo", true));
        return obj;
    }
};
#endif

Value validateaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "validateaddress <SocietyGaddress>\n"
            "Return information about <SocietyGaddress>.");

    CSocietyGcoinAddress address(params[0].get_str());
    bool isValid = address.IsValid();

    Object ret;
    ret.push_back(json_spirit::Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(json_spirit::Pair("address", currentAddress));
#ifdef ENABLE_WALLET
        isminetype mine = pwalletMain ? IsMine(*pwalletMain, dest) : ISMINE_NO;
        ret.push_back(json_spirit::Pair("ismine", (mine & ISMINE_SPENDABLE) ? true : false));
        if (mine != ISMINE_NO) {
            ret.push_back(json_spirit::Pair("iswatchonly", (mine & ISMINE_WATCH_ONLY) ? true: false));
            Object detail = boost::apply_visitor(DescribeAddressVisitor(mine), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if (pwalletMain && pwalletMain->mapAddressBook.count(dest))
            ret.push_back(json_spirit::Pair("account", pwalletMain->mapAddressBook[dest]));
#endif
    }
    return ret;
}

Value validatepubkey(const Array& params, bool fHelp)
{
    if (fHelp || !params.size() || params.size() > 2)
        throw runtime_error(
            "validatepubkey <SocietyGpubkey>\n"
            "Return information about <SocietyGpubkey>.");

    std::vector<unsigned char> vchPubKey = ParseHex(params[0].get_str());
    CPubKey pubKey(vchPubKey);

    bool isValid = pubKey.IsValid();
    bool isCompressed = pubKey.IsCompressed();
    CKeyID keyID = pubKey.GetID();

    CSocietyGcoinAddress address;
    address.Set(keyID);

    Object ret;
    ret.push_back(json_spirit::Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(json_spirit::Pair("address", currentAddress));
        ret.push_back(json_spirit::Pair("iscompressed", isCompressed));
#ifdef ENABLE_WALLET
        isminetype mine = pwalletMain ? IsMine(*pwalletMain, dest) : ISMINE_NO;
        ret.push_back(json_spirit::Pair("ismine", (mine & ISMINE_SPENDABLE) ? true : false));
        if (mine != ISMINE_NO) {
            ret.push_back(json_spirit::Pair("iswatchonly", (mine & ISMINE_WATCH_ONLY) ? true: false));
        	Object detail = boost::apply_visitor(DescribeAddressVisitor(mine), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if (pwalletMain && pwalletMain->mapAddressBook.count(dest))
            ret.push_back(json_spirit::Pair("account", pwalletMain->mapAddressBook[dest]));
#endif
    }
    return ret;
}

Value verifymessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "verifymessage <SocietyGaddress> <signature> <message>\n"
            "Verify a signed message");

    string strAddress  = params[0].get_str();
    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CSocietyGcoinAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    return (pubkey.GetID() == keyID);
}

/*
    Used for updating/reading spork settings on the network
*/
Value spork(const Array& params, bool fHelp)
{
    if(params.size() == 1 && params[0].get_str() == "show"){
        std::map<int, CSporkMessage>::iterator it = mapSporksActive.begin();

        Object ret;
        while(it != mapSporksActive.end()) {
            ret.push_back(json_spirit::Pair(sporkManager.GetSporkNameByID(it->second.nSporkID), it->second.nValue));
            it++;
        }
        return ret;
    } else if (params.size() == 2){
        int nSporkID = sporkManager.GetSporkIDByName(params[0].get_str());
        if(nSporkID == -1){
            return "Invalid spork name";
        }

        // SPORK VALUE
        int64_t nValue = params[1].get_int();

        //broadcast new spork
        if(sporkManager.UpdateSpork(nSporkID, nValue)){
            return "success";
        } else {
            return "failure";
        }

    }

    throw runtime_error(
        "spork <name> [<value>]\n"
        "<name> is the corresponding spork name, or 'show' to show all current spork settings"
        "<value> is a epoch datetime to enable or disable spork"
        + HelpRequiringPassphrase());
}

