// Copyright (c) 2009-2012 The Darkcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "protocol.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include <boost/lexical_cast.hpp>
#include "clientversion.h"

//
// Bootup the masternode, look for a 150 000 SocietyG input and register on the network
//
void CActiveMasternode::ManageStatus()
{
    std::string errorMessage;

/*    if (fDebug) */
        LogPrintf("\n\n CActiveMasternode::ManageStatus() - Begin \n"); 

    /* -------------------------------------------------------
       -- RGP, fMasterNode is set in the societyG.conf file --
       ------------------------------------------------------- */
    if(!fMasterNode) 
    {
        LogPrintf("RGP CActiveMasternode::ManageStatus, fMasterNode is FALSE \n");
        return;
    }

    /* -------------------------------------------------------------------------------------------------------
       -- RGP, On determination that this is a MasterNode, status is always set to MASTERNODE_NOT_PROCESSED --
       ------------------------------------------------------------------------------------------------------- */

    //need correct adjusted time to send ping
    bool fIsInitialDownload = IsInitialBlockDownload();
    if(fIsInitialDownload)
    {
        status = MASTERNODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveMasternode::ManageStatus() - Sync in progress. Must wait until sync is complete to start masternode.\n");
        return;
    }

LogPrintf("RGP ManageStatus RGP 001 \n");
status = MASTERNODE_REMOTELY_ENABLED;

    if(status == MASTERNODE_INPUT_TOO_NEW || status == MASTERNODE_NOT_CAPABLE || status == MASTERNODE_SYNC_IN_PROCESS)
    {
        
        LogPrintf("RGP ManageStatus RGP DEBUG MASTERNODE_NOT_PROCESSED status is %d \n", status);
        switch( status )
        {
           case MASTERNODE_INPUT_TOO_NEW   : LogPrintf("RGP ManageStatus MASTERNODE_INPUT_TOO_NEW\n");
                                             break;
           case MASTERNODE_NOT_CAPABLE     : LogPrintf("RGP ManageStatus MASTERNODE_NOT_CAPABLE\n");
                                             break;
           case MASTERNODE_SYNC_IN_PROCESS : LogPrintf("RGP ManageStatus MASTERNODE_SYNC_IN_PROCESS\n");
                                             break;
        }
        status = MASTERNODE_NOT_PROCESSED;
    }

    if( status == MASTERNODE_NOT_PROCESSED )
    {
LogPrintf("RGP CActiveMasternode Masternode processing \n");
        if(strMasterNodeAddr.empty()) 
        {
LogPrintf("RGP CActiveMasternode Masternode processing, missing IP Address \n");
            if(!GetLocal(service)) 
            {
                LogPrintf(" RGP DEBUG %s not cable on \n", strMasterNodeAddr.c_str()  );

                notCapableReason = "Can't detect external address %s . Please use the masternodeaddr configuration option.";
                status = MASTERNODE_NOT_CAPABLE;
                LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
                return;
            }
        }
        else
        {
        	LogPrintf("CActiveMasternode::ManageStatus() service is TRUE \n");
        	service = CService(strMasterNodeAddr, true);
        }

        LogPrintf("CActiveMasternode::ManageStatus() - Checking inbound connection to '%s'\n", service.ToString().c_str());


        /* SocietyG NOTE: There is no logical reason to restrict this to a specific port.  Its a peer, what difference does it make. */
        if ( service.ToString().c_str() == strMasterNodeAddr )
        {
           LogPrintf("ignoring our own IP address %s \n", strMasterNodeAddr.c_str() );
           
        }
        else
        {
        
           LogPrintf("RGP Debug ActiveMasternode trying to ConnectNode %s \n", service.ToString().c_str() );

           if(!ConnectNode( (CAddress)service, service.ToString().c_str()))
           {
                notCapableReason = "Could not connect to " + service.ToString();
                status = MASTERNODE_NOT_CAPABLE;
                LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
                return;
           }
           
        }
        
LogPrintf("RGP Debug ActiveMasternode before locked wallet check \n");
        if(pwalletMain->IsLocked()){
            notCapableReason = "Wallet is locked.";
            status = MASTERNODE_NOT_CAPABLE;
            LogPrintf("CActiveMasternode::ManageStatus() - not capable: %s\n", notCapableReason.c_str());
            return;
        }


LogPrintf("RGP Debug ActiveMasternode before Remote enabled check \n");
        if (status != MASTERNODE_REMOTELY_ENABLED)
        {

LogPrintf("RGP Debug ActiveMasternode before Remote enabled check 001 \n");
            // Set defaults
            status = MASTERNODE_NOT_CAPABLE;
            notCapableReason = "Unknown. Check debug.log for more information.\n";

            // Choose coins to use
            CPubKey pubKeyCollateralAddress;
            CKey keyCollateralAddress;            

LogPrintf("RGP Debug ActiveMasternode before Remote enabled check 002 \n");

            if( GetMasterNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress) )
            {
LogPrintf("RGP Debug ActiveMasternode before Remote enabled check 003 \n");
                if(GetInputAge(vin) < MASTERNODE_MIN_CONFIRMATIONS)
                {
LogPrintf("RGP Debug ActiveMasternode before Remote enabled check 004 \n");

                    notCapableReason = "Input must have least " + boost::lexical_cast<string>(MASTERNODE_MIN_CONFIRMATIONS) +
                                       " confirmations - " + boost::lexical_cast<string>(GetInputAge(vin)) + " confirmations";
                    LogPrintf("CActiveMasternode::ManageStatus() - %s\n", notCapableReason.c_str());
                    status = MASTERNODE_INPUT_TOO_NEW;

LogPrintf("RGP Debug ActiveMasternode before Remote enabled check 004 \n");

                    return;
                }

                LogPrintf("CActiveMasternode::ManageStatus() - Is capable master node!\n");

                status = MASTERNODE_IS_CAPABLE;
                notCapableReason = "";
LogPrintf("RGP Debug ActiveMasternode before Remote enabled check 005 \n");
                pwalletMain->LockCoin(vin.prevout);

                // send to all nodes
                CPubKey pubKeyMasternode;
                CKey keyMasternode;

                LogPrintf("*** RGPC ActiveMasternode::ManageStatus %s \n", strMasterNodePrivKey.c_str() );

                if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
                {

LogPrintf("RGP Debug ActiveMasternode before Remote enabled check 006 \n");

                    LogPrintf("ActiveMasternode::Dseep() - Error upon calling SetKey [ManageStatus] : %s \n", errorMessage.c_str() );
                    return;
                }

                /* rewards are not supported in societyG.conf */
                CScript rewardAddress = CScript();
                int rewardPercentage = 0;

                if(!Register(vin, service, keyCollateralAddress, pubKeyCollateralAddress, keyMasternode, pubKeyMasternode, rewardAddress, rewardPercentage, errorMessage ) )
                {
                   LogPrintf("CActiveMasternode::ManageStatus() - Error on Register: %s\n", errorMessage.c_str());
                }
LogPrintf("RGP Debug ActiveMasternode before Remote enabled check 004 \n");

                return;

             }
             else
             {

LogPrintf("RGP Debug ActiveMasternode before Remote enabled check 005 \n");

                    notCapableReason = "Could not find suitable coins!";
                    //LogPrintf("CActiveMasternode::ManageStatus() - Could not find suitable coins!\n");

             }

        }
        else
LogPrintf("Masternode is remotely connected \n");

    }

    //send to all peers
    if(!Dseep(errorMessage))
    {
        LogPrintf("CActiveMasternode::ManageStatus() - Error on Ping: %s\n", errorMessage.c_str());
    }
    MilliSleep(5); /* RGP Optimise */
}

// Send stop dseep to network for remote masternode
bool CActiveMasternode::StopMasterNode(std::string strService, std::string strKeyMasternode, std::string& errorMessage) {
	CTxIn vin;
    CKey keyMasternode;
    CPubKey pubKeyMasternode;

     //LogPrintf("*** RGP CActiveMasternode::StopMasternode Start 1 \n");

    if(!darkSendSigner.SetKey(strKeyMasternode, errorMessage, keyMasternode, pubKeyMasternode))
    {
        //LogPrintf("CActiveMasternode::StopMasterNode() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    if (GetMasterNodeVin(vin, pubKeyMasternode, keyMasternode)){
        // LogPrintf("MasternodeStop::VinFound: %s\n", vin.ToString());
    }
    MilliSleep(5); /* RGP Optimise */
    
    return StopMasterNode(vin, CService(strService, true), keyMasternode, pubKeyMasternode, errorMessage);
}

// Send stop dseep to network for main masternode
bool CActiveMasternode::StopMasterNode(std::string& errorMessage)
{
        //LogPrintf("*** RGP CActiveMasternode::StopMasternode Start 2 \n");

        if(status != MASTERNODE_IS_CAPABLE && status != MASTERNODE_REMOTELY_ENABLED) {
		errorMessage = "masternode is not in a running status";
        // LogPrintf("CActiveMasternode::StopMasterNode() - Error: %s\n", errorMessage.c_str());
		return false;
	}

	status = MASTERNODE_STOPPED;

    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
    {
        // LogPrintf("Register::ManageStatus() - Error upon calling SetKey: %s\n", errorMessage.c_str());
    	return false;
    }
    MilliSleep(5); /* RGP Optimise */

    return StopMasterNode(vin, service, keyMasternode, pubKeyMasternode, errorMessage);
}

// Send stop dseep to network for any masternode
bool CActiveMasternode::StopMasterNode(CTxIn vin, CService service, CKey keyMasternode, CPubKey pubKeyMasternode, std::string& errorMessage)
{
        //LogPrintf("*** RGP CActiveMasternode::StopMasternode Start 3 \n");

        pwalletMain->UnlockCoin(vin.prevout);
	return Dseep(vin, service, keyMasternode, pubKeyMasternode, errorMessage, true);
}

bool CActiveMasternode::Dseep(std::string& errorMessage) 
{
	if(status != MASTERNODE_IS_CAPABLE && status != MASTERNODE_REMOTELY_ENABLED) 
    {
		errorMessage = "masternode is not in a running status";
        LogPrintf("CActiveMasternode::Dseep() - Error: %s\n", errorMessage.c_str());

/* let's try to force this */
return true;
		return false;
	}

LogPrintf("RGP Debug Dseep() preparing for darksendsigner key %s \n", strMasterNodePrivKey.c_str() );

    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    if(!darkSendSigner.SetKey(strMasterNodePrivKey, errorMessage, keyMasternode, pubKeyMasternode))
    {
        LogPrintf("CActiveMasternode::Dseep() - Error upon calling SetKey [stopmasternode]: %s\n", errorMessage.c_str());
    	return false;
    }

	return Dseep(vin, service, keyMasternode, pubKeyMasternode, errorMessage, false);
}

bool CActiveMasternode::Dseep(CTxIn vin, CService service, CKey keyMasternode, CPubKey pubKeyMasternode, std::string &retErrorMessage, bool stop)
{
    std::string errorMessage;
    std::vector<unsigned char> vchMasterNodeSignature;
    std::string strMasterNodeSignMessage;
    int64_t masterNodeSignatureTime = GetAdjustedTime();

    std::string strMessage = service.ToString() + boost::lexical_cast<std::string>(masterNodeSignatureTime) + boost::lexical_cast<std::string>(stop);


    LogPrintf("*** RGP CActiveMasternode::dsEEP Start 2 \n");

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchMasterNodeSignature, keyMasternode)) {
    	retErrorMessage = "sign message failed: " + errorMessage;
        LogPrintf("CActiveMasternode::Dseep() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchMasterNodeSignature, strMessage, errorMessage)) {
    	retErrorMessage = "Verify message failed: " + errorMessage;
        LogPrintf("CActiveMasternode::Dseep() - Error: %s\n", retErrorMessage.c_str());
        return false;
    }

    // Update Last Seen timestamp in masternode list
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn != NULL)
    {
        if(stop)
            mnodeman.Remove(pmn->vin);
        else
            pmn->UpdateLastSeen();
    } else {
    	// Seems like we are trying to send a ping while the masternode is not registered in the network
    	retErrorMessage = "Darksend Masternode List doesn't include our masternode, Shutting down masternode pinging service! " + vin.ToString();
        LogPrintf("CActiveMasternode::Dseep() - Error: %s\n", retErrorMessage.c_str());
        status = MASTERNODE_NOT_CAPABLE;
        notCapableReason = retErrorMessage;
        return false;
    }

    //send to all peers
    //LogPrintf("CActiveMasternode::Dseep() - RelayMasternodeEntryPing vin = %s\n", vin.ToString().c_str());
    mnodeman.RelayMasternodeEntryPing(vin, vchMasterNodeSignature, masterNodeSignatureTime, stop);

    MilliSleep(5); /* RGP Optimise */
    
    return true;
}

bool CActiveMasternode::Register(std::string strService, std::string strKeyMasternode, std::string txHash, std::string strOutputIndex, std::string strRewardAddress, std::string strRewardPercentage, std::string& errorMessage) {
    CTxIn vin;
    CPubKey pubKeyCollateralAddress;
    CKey keyCollateralAddress;
    CPubKey pubKeyMasternode;
    CKey keyMasternode;
    CScript rewardAddress = CScript();
    int rewardPercentage = 0;


    LogPrintf("*** RGP CActiveMasternode::Register 1 Start \n" );


    LogPrintf("*** RGP CActiveMasternode::Register strService %s \n",strService );
    LogPrintf("*** RGP CActiveMasternode::Register strKeyMasternode %s \n", strKeyMasternode );
    LogPrintf("*** RGP CActiveMasternode::Register txHash %s \n", txHash );
    LogPrintf("*** RGP CActiveMasternode::Register strRewardAddress %s \n", strRewardAddress );
    LogPrintf("*** RGP CActiveMasternode::Register strOutputIndex %s \n", strOutputIndex );
    LogPrintf("*** RGP CActiveMasternode::Register strRewardPercentage %s \n", strRewardPercentage );

    if(!darkSendSigner.SetKey(strKeyMasternode, errorMessage, keyMasternode, pubKeyMasternode))
    {
        //LogPrintf("*** RGP CActiveMasternode::Register Debug 1 \n" );
        // LogPrintf("CActiveMasternode::Register() - Error upon calling SetKey: %s\n", errorMessage.c_str());
        return false;
    }


    bool GetVINreturn = GetMasterNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress, txHash, strOutputIndex);
    if ( !GetVINreturn )
    {
    //if(!GetMasterNodeVin(vin, pubKeyCollateralAddress, keyCollateralAddress, txHash, strOutputIndex)) {
        errorMessage = "could not allocate vin";
        LogPrintf("CActiveMasternode::Register() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    //LogPrintf("*** RGP CActiveMasternode::Register Debug 3 \n" );

    CSocietyGcoinAddress address;
    if (strRewardAddress != "")
    {
        LogPrintf("*** RGP CActiveMasternode::Register Debug 4 \n" );

        if(!address.SetString(strRewardAddress))
        {
            //LogPrintf("ActiveMasternode::Register - Invalid Reward Address\n");
            return false;
        }
        rewardAddress.SetDestination(address.Get());

        LogPrintf("*** RGP CActiveMasternode::Register Debug 5 \n" );

        try {
            rewardPercentage = boost::lexical_cast<int>( strRewardPercentage );
        } catch( boost::bad_lexical_cast const& ) {
            LogPrintf("ActiveMasternode::Register - Invalid Reward Percentage (Couldn't cast)\n");
            return false;
        }

        LogPrintf("*** RGP CActiveMasternode::Register Debug 6 \n" );

        if(rewardPercentage < 0 || rewardPercentage > 100)
        {
           // LogPrintf("ActiveMasternode::Register - Reward Percentage Out Of Range\n");
            return false;
        }

        LogPrintf("*** RGP CActiveMasternode::Register Debug 7 \n" );

    }

    LogPrintf("*** RGP CActiveMasternode::Register Debug 8 Private key %s \n", strKeyMasternode );


    /* RGP, check the secret to see if it's been set or not */
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn != NULL)
    {
        if ( strlen ( &pmn->strKeyMasternode[0] ) == 0 )
        {
            LogPrintf("*** RGP updating masternode secret %s \n", strKeyMasternode );

            pmn->strKeyMasternode = strKeyMasternode;


        }

    }


    LogPrintf("*** RGP updated masternode secret privkey %s \n", strKeyMasternode );
    
    MilliSleep(5); /* RGP Optimise */
    
    return Register(vin, CService(strService, true), keyCollateralAddress, pubKeyCollateralAddress, keyMasternode, pubKeyMasternode, rewardAddress, rewardPercentage, errorMessage );

}

bool CActiveMasternode::Register(CTxIn vin, CService service, CKey keyCollateralAddress, CPubKey pubKeyCollateralAddress, CKey keyMasternode, CPubKey pubKeyMasternode, CScript rewardAddress, int rewardPercentage, std::string &retErrorMessage )
{
    std::string errorMessage;
    std::vector<unsigned char> vchMasterNodeSignature;
    std::string strMasterNodeSignMessage;
    int64_t masterNodeSignatureTime = GetAdjustedTime();

    std::string vchPubKey(pubKeyCollateralAddress.begin(), pubKeyCollateralAddress.end());
    std::string vchPubKey2(pubKeyMasternode.begin(), pubKeyMasternode.end());

    std::string strMessage = service.ToString() + boost::lexical_cast<std::string>(masterNodeSignatureTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(PROTOCOL_VERSION) + rewardAddress.ToString() + boost::lexical_cast<std::string>(rewardPercentage);

    LogPrintf("*** RGP CActiveMasternode::Register 2 start \n");

    LogPrintf("*** RGP data is vchPubKey %s vchPubKey2 %s strMessage \n", vchPubKey, vchPubKey2, strMessage  );

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchMasterNodeSignature, keyCollateralAddress)) {
		retErrorMessage = "sign message failed: " + errorMessage;
                // LogPrintf("CActiveMasternode::Register() - Error: %s\n", retErrorMessage.c_str());
		return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyCollateralAddress, vchMasterNodeSignature, strMessage, errorMessage)) {
		retErrorMessage = "Verify message failed: " + errorMessage;
                //LogPrintf("CActiveMasternode::Register() - Error: %s\n", retErrorMessage.c_str());
		return false;
	}


    LogPrintf("*** RGP CActiveMasternode::Register calling mnodeman.FIND \n");


    CMasternode* pmn = mnodeman.Find(vin);

    LogPrintf("*** RGP CActiveMasternode::Register calling mnodeman.FIND returned \n");

    if(pmn == NULL)
    {
        LogPrintf("*** RGP CActiveMasternode::Register Debug 1 \n");
        LogPrintf("CActiveMasternode::Register() - Adding to masternode list service: %s - vin: %s\n", service.ToString().c_str(), vin.ToString().c_str());

        LogPrintf("*** RGP CActiveMasternode::Register Debug 2 \n");

        CMasternode mn(service, vin, pubKeyCollateralAddress, vchMasterNodeSignature, masterNodeSignatureTime, pubKeyMasternode, PROTOCOL_VERSION, rewardAddress, rewardPercentage);

        LogPrintf("*** RGP CActiveMasternode::Register Debug 3 \n");

        mn.ChangeNodeStatus(true); /* RGP */
        mn.UpdateLastSeen(masterNodeSignatureTime);

        LogPrintf("*** RGP CActiveMasternode::Register Debug 4 \n");



        mnodeman.Add(mn);

        LogPrintf("*** RGP CActiveMasternode::Register Debug 5 \n");
    }

    LogPrintf("*** RGP CActiveMasternode::Register calling RelayMasternodeEntry \n");

    //send to all peers
    LogPrintf("CActiveMasternode::Register() - RelayElectionEntry vin = %s  \n", vin.ToString().c_str()  );
    mnodeman.RelayMasternodeEntry(vin, service, vchMasterNodeSignature, masterNodeSignatureTime, pubKeyCollateralAddress,
                                  pubKeyMasternode, -1, -1, masterNodeSignatureTime,
                                  PROTOCOL_VERSION, rewardAddress, rewardPercentage );
                                  
    MilliSleep(5); /* RGP Optimise */

    return true;
}

bool CActiveMasternode::GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey) 
{

    LogPrintf("RGP Debug GetMasterNodeVin start \n");

	return GetMasterNodeVin(vin, pubkey, secretKey, "", "");
}

bool CActiveMasternode::GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex) 
{
    CScript pubScript;

LogPrintf("RGP DEBUG Check GetMasterNodeVin Check 001 \n");

    // Find possible candidates
    vector<COutput> possibleCoins = SelectCoinsMasternode();

LogPrintf("RGP DEBUG Check GetMasterNodeVin Check 001.25 \n");

    COutput *selectedOutput;

LogPrintf("RGP DEBUG Check GetMasterNodeVin Check 001.5 \n");

    // Find the vin
	if(!strTxHash.empty())
    {
LogPrintf("RGP DEBUG Check GetMasterNodeVin Check 002 hash %s outputIndex %s \n", strTxHash, strOutputIndex );
		// Let's find it
		uint256 txHash(strTxHash);
        int outputIndex = boost::lexical_cast<int>(strOutputIndex);
		bool found = false;
		BOOST_FOREACH(COutput& out, possibleCoins) 
		{
			if(out.tx->GetHash() == txHash && out.i == outputIndex)
			{
				selectedOutput = &out;
				found = true;
				break;
			}
			MilliSleep(1); /* RGP Optimise */
		}
		if(!found) 
        {
            LogPrintf("CActiveMasternode::GetMasterNodeVin - Could not locate valid vin\n");
			return false;
		}
	} 
    else 
    {
LogPrintf("RGP DEBUG Check GetMasterNodeVin Check 003 \n");

		// No output specified,  Select the first one
		if(possibleCoins.size() > 0) 
        {
			selectedOutput = &possibleCoins[0];
		} 
        else 
        {
            LogPrintf("CActiveMasternode::GetMasterNodeVin - Could not locate specified vin from possible list\n");
/* hack this for now */
            return true;
			return false;
		}
    }

LogPrintf("RGP DEBUG Check GetMasterNodeVin Check 004 \n");

    MilliSleep(5); /* RGP Optimise */
	// At this point we have a selected output, retrieve the associated info
	return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}

bool CActiveMasternode::GetMasterNodeVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {
	return GetMasterNodeVinForPubKey(collateralAddress, vin, pubkey, secretKey, "", "");
}

bool CActiveMasternode::GetMasterNodeVinForPubKey(std::string collateralAddress, CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex) {
    CScript pubScript;

LogPrintf("RGP GetMasterNodeVinForPubKey collateralAddress %s  strTxHash %s strOutputIndex  \n", collateralAddress, strTxHash, strOutputIndex    );

    // Find possible candidates
    vector<COutput> possibleCoins = SelectCoinsMasternodeForPubKey(collateralAddress);
    COutput *selectedOutput;

    LogPrintf("\n *** RGP GetMasterNodeVinForPubKey collateralAddress %s \n", collateralAddress );

    // Find the vin
	if(!strTxHash.empty()) 
    {
LogPrintf("RGP DEBUG Check GetMasterNodeVin Check strTxHash is EMPTY!!! \n");
		// Let's find it
		uint256 txHash(strTxHash);
        int outputIndex = boost::lexical_cast<int>(strOutputIndex);
		bool found = false;
		BOOST_FOREACH(COutput& out, possibleCoins) 
		{
			if(out.tx->GetHash() == txHash && out.i == outputIndex)
			{
				selectedOutput = &out;
				found = true;
				break;
			}
			MilliSleep(1); /* RGP Optimise */
		}
		if(!found) 
        {
            LogPrintf("CActiveMasternode::GetMasterNodeVinForPubKey - Could not locate valid vin\n");
			return false;
		}
	} else {
		// No output specified,  Select the first one
		if(possibleCoins.size() > 0) {
			selectedOutput = &possibleCoins[0];
		} else 
        {
            LogPrintf("CActiveMasternode::GetMasterNodeVinForPubKey - Could not locate specified vin from possible list\n");
			return false;
		}
    }
    MilliSleep(5); /* RGP Optimise */
    
    // At this point we have a selected output, retrieve the associated info
    return GetVinFromOutput(*selectedOutput, vin, pubkey, secretKey);
}


// Extract masternode vin information from output
bool CActiveMasternode::GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey) {

    CScript pubScript;

    vin = CTxIn(out.tx->GetHash(),out.i);
    pubScript = out.tx->vout[out.i].scriptPubKey; // the inputs PubKey

    CTxDestination address1;
    ExtractDestination(pubScript, address1);
    CSocietyGcoinAddress address2(address1);

    CKeyID keyID;
    if (!address2.GetKeyID(keyID)) {
        LogPrintf("CActiveMasternode::GetMasterNodeVin - Address does not refer to a key\n");
        return false;
    }

    if (!pwalletMain->GetKey(keyID, secretKey)) {
        LogPrintf ("CActiveMasternode::GetMasterNodeVin - Private key for address is not known\n");
        return false;
    }

    pubkey = secretKey.GetPubKey();
    return true;
}

// get all possible outputs for running masternode
vector<COutput> CActiveMasternode::SelectCoinsMasternode()
{
    vector<COutput> vCoins;
    vector<COutput> filteredCoins;

LogPrintf("RGP Debug SelectCoinsMasternode Check 1 \n" );

    // Retrieve all possible outputs
    pwalletMain->AvailableCoinsMN(vCoins);

    // Filter
    BOOST_FOREACH(const COutput& out, vCoins)
    {

//LogPrintf("RGP DEBUG SelectCoinsMasternode %s \n ", out->ToString() );

        /* RGP, should return 150,000, defined in main.h */
        if(out.tx->vout[out.i].nValue == GetMNCollateral( pindexBest->nHeight ) *COIN ) 
        { //exactly

            

        	filteredCoins.push_back(out);
        }
        MilliSleep(1); /* RGP Optimise */
    }

    LogPrintf("RGP Debug SelectCoinsMasternode Check 2 \n" );

    return filteredCoins;
}

// get all possible outputs for running masternode for a specific pubkey
vector<COutput> CActiveMasternode::SelectCoinsMasternodeForPubKey(std::string collateralAddress)
{
    CSocietyGcoinAddress address(collateralAddress);
    CScript scriptPubKey;
    scriptPubKey.SetDestination(address.Get());
    vector<COutput> vCoins;
    vector<COutput> filteredCoins;

    // Retrieve all possible outputs
    pwalletMain->AvailableCoins(vCoins);

    // Filter
    BOOST_FOREACH(const COutput& out, vCoins)
    {
        if(out.tx->vout[out.i].scriptPubKey == scriptPubKey && out.tx->vout[out.i].nValue == GetMNCollateral(pindexBest->nHeight)*COIN) { //exactly
        	filteredCoins.push_back(out);
        }
        MilliSleep(1); /* RGP Optimise */
    }
    return filteredCoins;
}

// when starting a masternode, this can enable to run as a hot wallet with no funds
bool CActiveMasternode::EnableHotColdMasterNode(CTxIn& newVin, CService& newService)
{
    //LogPrintf("CActiveMasternode::EnableHotColdMasterNode() Start \n");

    if(!fMasterNode)
    {
        //LogPrintf("CActiveMasternode::EnableHotColdMasterNode() fMasterNode is false!!! \n");
        return false;
    }

    status = MASTERNODE_REMOTELY_ENABLED;
    notCapableReason = "";

    //The values below are needed for signing dseep messages going forward
    this->vin = newVin;
    this->service = newService;

    //LogPrintf("CActiveMasternode::EnableHotColdMasterNode() - Enabled! You may shut down the cold daemon.\n");

    return true;
}
