#include "Events/PartyEvents.hpp"
#include "API/CNWSPlayer.hpp"
#include "API/CNWSMessage.hpp"
#include "API/Functions.hpp"
#include "API/Constants.hpp"
#include "Events.hpp"
#include "Utils.hpp"
#include <cstring>

namespace Events {

using namespace NWNXLib;
using namespace NWNXLib::API;
using namespace NWNXLib::Platform;

static NWNXLib::Hooking::FunctionHook* m_HandlePlayerToServerPartyHook = nullptr;

PartyEvents::PartyEvents(ViewPtr<Services::HooksProxy> hooker)
{
    hooker->RequestExclusiveHook<Functions::CNWSMessage__HandlePlayerToServerParty, int32_t,
        CNWSMessage*, CNWSPlayer*, uint8_t>(&HandlePartyMessageHook);
    m_HandlePlayerToServerPartyHook = hooker->FindHookByAddress(API::Functions::CNWSMessage__HandlePlayerToServerParty);
}
template <typename T>
static T PeekMessage(CNWSMessage *pMessage, int32_t offset)
{
    static_assert(std::is_pod<T>::value);
    T value;
    uint8_t *ptr = pMessage->m_pnReadBuffer + pMessage->m_nReadBufferPtr + offset;
    std::memcpy(&value, ptr, sizeof(T));
    return value;
}

int32_t PartyEvents::HandlePartyMessageHook(CNWSMessage *thisPtr, CNWSPlayer *pPlayer, uint8_t nMinor)
{
    int32_t retVal;
    
    std::string event = "NWNX_ON_PARTY_";
    Types::ObjectID oidPlayer = pPlayer ? pPlayer->m_oidNWSObject : Constants::OBJECT_INVALID;
    std::string sOidOther = Utils::ObjectIDToString(PeekMessage<Types::ObjectID>(thisPtr, 0) & 0x7FFFFFFF);

    std::string argname;
    switch (nMinor)
    {
        case 0x06:
            event += "LEAVE";
            argname = "LEAVING";
            break;
        case 0x07:
            event += "KICK";
            argname = "KICKED";
            break;
        case 0x08:
            event += "TRANSFER_LEADERSHIP";
            argname = "NEW_LEADER";
            break;
        case 0x09:
            event += "INVITE";
            argname = "INVITED";
            break;
        case 0x0a:
            event += "IGNORE_INVITATION";
            argname = "INVITED_BY";
            break;
        case 0x0b:
            event += "ACCEPT_INVITATION";
            argname = "INVITED_BY";
            break;
        case 0x0c:
            event += "REJECT_INVITATION";
            argname = "INVITED_BY";
            break;
        case 0x0d:
            event += "KICK_HENCHMAN";
            argname = "KICKED";
            break;

        default:
            break;
    }

    Events::PushEventData(argname, sOidOther);

    if (Events::SignalEvent(event + "_BEFORE", oidPlayer))
    {
        retVal = m_HandlePlayerToServerPartyHook->CallOriginal<int32_t>(thisPtr, pPlayer, nMinor);
    }
    else
    {
        retVal = false;
    }

    Events::PushEventData(argname, sOidOther);
    Events::SignalEvent(event + "_AFTER", oidPlayer);

    return retVal;
}

}