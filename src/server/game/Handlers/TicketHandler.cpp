/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Common.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "TicketMgr.h"
#include "TicketPackets.h"
#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "DatabaseEnv.h"
#include "WordFilterMgr.h"
#include "MiscPackets.h"

void WorldSession::HandleComplaint(WorldPackets::Ticket::Complaint& packet)
{
    uint64 complaintId = sObjectMgr->GenerateReportComplaintID();
    if (sWordFilterMgr->AddComplaintForUser(packet.Offender.PlayerGuid, GetPlayer()->GetGUID(), complaintId, packet.Chat.MessageLog))
    {
        uint8 index = 0;
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_COMPLAINTS);
        stmt->setUInt64(index++, complaintId);
        stmt->setUInt64(index++, GetPlayer()->GetGUIDLow());
        stmt->setUInt32(index++, GetAccountId());
        stmt->setUInt32(index++, time(NULL));
        stmt->setUInt64(index++, packet.Offender.PlayerGuid.GetGUIDLow());
        stmt->setUInt8(index++, packet.ComplaintType);
        stmt->setUInt32(index++, packet.MailID);
        stmt->setUInt32(index++, packet.Offender.TimeSinceOffence);
        stmt->setString(index++, packet.Chat.MessageLog);
        CharacterDatabase.Execute(stmt);
    }

    WorldPackets::Ticket::ComplaintResult result;
    result.ComplaintType = packet.ComplaintType;
    result.Result = 0;
    SendPacket(result.Write());
}



void WorldSession::HandleGMTicketGetSystemStatus(WorldPackets::Ticket::GMTicketGetSystemStatus& /*packet*/)
{
    // Note: This only disables the ticket UI at client side and is not fully reliable
    WorldPackets::Ticket::GMTicketSystemStatus response;
    response.Status = sTicketMgr->GetStatus() ? GMTICKET_QUEUE_STATUS_ENABLED : GMTICKET_QUEUE_STATUS_DISABLED;
    SendPacket(response.Write());
}

void WorldSession::HandleGMTicketAcknowledgeSurvey(WorldPackets::Ticket::GMTicketAcknowledgeSurvey& /*packet*/)
{ }

void WorldSession::HandleGMTicketGetCaseStatus(WorldPackets::Ticket::GMTicketGetCaseStatus& /*packet*/)
{
    // TODO: Implement GmCase and handle this packet properly
    WorldPackets::Ticket::GMTicketCaseStatus status;
    SendPacket(status.Write());
}

void WorldSession::HandleSupportTicketSubmitComplaint(WorldPackets::Ticket::SupportTicketSubmitComplaint& /*packet*/)
{ }

void WorldSession::HandleSupportTicketSubmitSuggestion(WorldPackets::Ticket::SupportTicketSubmitSuggestion& /*packet*/)
{ }

void WorldSession::OnGMTicketGetTicketEvent()
{
    if (m_Player == nullptr)
        return;

    SendQueryTimeResponse();

    if (GmTicket* l_Ticket = sTicketMgr->GetTicketByPlayer(GetPlayer()->GetGUID()))
    {
        if (l_Ticket->IsCompleted())
            l_Ticket->SendResponse(this);
        else
            sTicketMgr->SendTicket(this, l_Ticket);
    }
    else
        sTicketMgr->SendTicket(this, NULL);
}


void WorldSession::SendTicketStatusUpdate(uint8 p_Response)
{
    if (!GetPlayer())
        return;

    WorldPackets::Misc::DisplayGameError display;

    switch (p_Response)
    {
    case GMTICKET_RESPONSE_ALREADY_EXIST:       ///< = 1
        display.Error = UIErrors::ERR_TICKET_ALREADY_EXISTS;
        break;
    case GMTICKET_RESPONSE_UPDATE_ERROR:        ///< = 5
        display.Error = UIErrors::ERR_TICKET_UPDATE_ERROR;
        break;
    case GMTICKET_RESPONSE_CREATE_ERROR:        ///< = 3
        display.Error = UIErrors::ERR_TICKET_CREATE_ERROR;
        break;

    case GMTICKET_RESPONSE_CREATE_SUCCESS:      ///< = 2
    case GMTICKET_RESPONSE_UPDATE_SUCCESS:      ///< = 4
        OnGMTicketGetTicketEvent();
        break;

    case GMTICKET_RESPONSE_TICKET_DELETED:      ///< = 9
        GetPlayer()->SendCustomMessage("NOVA_CLIENT_TICKET_DELETED");
        break;

    default:
        display.Error = UIErrors::ERR_TICKET_DB_ERROR;
        break;
    }

    if (p_Response != GMTICKET_RESPONSE_CREATE_SUCCESS &&
        p_Response != GMTICKET_RESPONSE_UPDATE_SUCCESS &&
        p_Response != GMTICKET_RESPONSE_TICKET_DELETED) {
    }
}
