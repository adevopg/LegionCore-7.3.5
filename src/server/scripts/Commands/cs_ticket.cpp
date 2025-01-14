/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

/* ScriptData
Name: ticket_commandscript
%Complete: 100
Comment: All ticket related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "AccountMgr.h"
#include "ObjectMgr.h"
#include "TicketMgr.h"
#include "GlobalFunctional.h"
#include "DatabaseEnv.h"

class ticket_commandscript : public CommandScript
{
public:
    ticket_commandscript() : CommandScript("ticket_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> ticketResponseCommandTable =
        {
            { "append",         SEC_MODERATOR,      true,  &HandleGMTicketResponseAppendCommand,    ""},
            { "appendln",       SEC_MODERATOR,      true,  &HandleGMTicketResponseAppendLnCommand,  ""}
        };
        static std::vector<ChatCommand> ticketCommandTable =
        {
            { "assign",         SEC_GAMEMASTER,     true,  &HandleGMTicketAssignToCommand,          ""},
            { "close",          SEC_MODERATOR,      true,  &HandleGMTicketCloseByIdCommand,         ""},
            { "closedlist",     SEC_MODERATOR,      true,  &HandleGMTicketListClosedCommand,        ""},
            { "comment",        SEC_MODERATOR,      true,  &HandleGMTicketCommentCommand,           ""},
            { "complete",       SEC_MODERATOR,      true,  &HandleGMTicketCompleteCommand,          ""},
            { "delete",         SEC_ADMINISTRATOR,  true,  &HandleGMTicketDeleteByIdCommand,        ""},
            { "escalate",       SEC_MODERATOR,      true,  &HandleGMTicketEscalateCommand,          ""},
            { "escalatedlist",  SEC_GAMEMASTER,     true,  &HandleGMTicketListEscalatedCommand,     ""},
            { "list",           SEC_MODERATOR,      true,  &HandleGMTicketListCommand,              ""},
            { "onlinelist",     SEC_MODERATOR,      true,  &HandleGMTicketListOnlineCommand,        ""},
            { "reset",          SEC_ADMINISTRATOR,  true,  &HandleGMTicketResetCommand,             ""},
            { "response",       SEC_MODERATOR,      true,  NULL,                                    "", ticketResponseCommandTable },
            { "togglesystem",   SEC_ADMINISTRATOR,  true,  &HandleToggleGMTicketSystem,             ""},
            { "unassign",       SEC_GAMEMASTER,     true,  &HandleGMTicketUnAssignCommand,          ""},
            { "viewid",         SEC_MODERATOR,      true,  &HandleGMTicketGetByIdCommand,           ""},
            { "viewname",       SEC_MODERATOR,      true,  &HandleGMTicketGetByNameCommand,         ""},
            { "nwccreate",      SEC_PLAYER,         true,  &Nova_Client_HandleGMTicketCreate,       ""},
            { "nwcupdate",      SEC_PLAYER,         true,  &Nova_Client_HandleGMTicketUpdate,                ""},
            { "nwcappend",      SEC_PLAYER,         true,  &Nova_Client_HandleGMTicketAppend,                ""},
            { "nwcend",         SEC_PLAYER,         true,  &Nova_Client_HandleGMTicketEnd,                   ""},
            { "nwcdelete",      SEC_PLAYER,         true,  &Nova_Client_HandleGMTicketDelete,                ""},
            { "nwcticketget",   SEC_PLAYER,         true,  &Nova_Client_HandleGMTicketGet,                   ""},
            { "nwcticketres",   SEC_PLAYER,         true,  &Nova_Client_HandleGMResponseResolve,             ""}
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "ticket",         SEC_PLAYER,      false, NULL,                                    "", ticketCommandTable }
        };
        return commandTable;
    }

    static bool Nova_Client_HandleGMTicketCreate(ChatHandler* p_Handler, char const* p_Args)
    {
        auto l_ReplaceAll = [](std::string& str, const std::string& from, const std::string& to) {
            if (from.empty())
                return;
            size_t start_pos = 0;
            while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
            }
            };

        // Don't accept tickets if the ticket queue is disabled. (Ticket UI is greyed out but not fully dependable)
        if (sTicketMgr->GetStatus() == GMTICKET_QUEUE_STATUS_DISABLED)
            return true;

        Player* l_Player = p_Handler->GetSession()->GetPlayer();

        if (!l_Player)
            return true;

        if (l_Player->getLevel() < sWorld->getIntConfig(CONFIG_TICKET_LEVEL_REQ))
        {
            p_Handler->GetSession()->SendNotification(p_Handler->GetSession()->GetTrinityString(LANG_TICKET_REQ), sWorld->getIntConfig(CONFIG_TICKET_LEVEL_REQ));
            return true;
        }

        std::string l_Message = p_Args;
        l_ReplaceAll(l_Message, "$$n", "\n");

        GMTicketResponse l_Response = GMTICKET_RESPONSE_CREATE_ERROR;
        // Player must not have opened ticket
        if (GmTicket* l_Ticket = sTicketMgr->GetTicketByPlayer(l_Player->GetGUID()))
        {
            if (l_Ticket->IsCompleted())
            {
                sTicketMgr->CloseTicket(l_Ticket->GetId(), l_Player->GetGUID());
                sTicketMgr->SendTicket(p_Handler->GetSession(), NULL);

                p_Handler->GetSession()->SendTicketStatusUpdate(GMTICKET_RESPONSE_TICKET_DELETED);

                GmTicket* l_NewTicket = new GmTicket(l_Player->GetName(), l_Player->GetGUID(), l_Player->GetMapId(), *l_Player, l_Message);
                sTicketMgr->AddTicket(l_NewTicket);
                sTicketMgr->UpdateLastChange();

                sWorld->SendGMText(LANG_COMMAND_TICKETNEW, l_Player->GetName(), l_NewTicket->GetId());

                //sTicketMgr->SendTicket(p_Handler->GetSession(), l_NewTicket);

                l_Response = GMTICKET_RESPONSE_CREATE_SUCCESS;
            }
            else
                l_Response = GMTICKET_RESPONSE_ALREADY_EXIST;
        }
        else
        {
            GmTicket* l_NewTicket = new GmTicket(l_Player->GetName(), l_Player->GetGUID(), l_Player->GetMapId(), *l_Player, l_Message);
            sTicketMgr->AddTicket(l_NewTicket);
            sTicketMgr->UpdateLastChange();

            sWorld->SendGMText(LANG_COMMAND_TICKETNEW, l_Player->GetName(), l_NewTicket->GetId());

            //sTicketMgr->SendTicket(p_Handler->GetSession(), l_NewTicket);

            l_Response = GMTICKET_RESPONSE_CREATE_SUCCESS;
        }

        p_Handler->GetSession()->SendTicketStatusUpdate(l_Response);
        return true;
    }

    static bool Nova_Client_HandleGMTicketUpdate(ChatHandler* p_Handler, char const* p_Args)
    {
        auto l_ReplaceAll = [](std::string& str, const std::string& from, const std::string& to) {
            if (from.empty())
                return;
            size_t start_pos = 0;
            while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
            }
            };

        // Don't accept tickets if the ticket queue is disabled. (Ticket UI is greyed out but not fully dependable)
        if (sTicketMgr->GetStatus() == GMTICKET_QUEUE_STATUS_DISABLED)
            return true;

        Player* l_Player = p_Handler->GetSession()->GetPlayer();

        if (!l_Player)
            return true;

        std::string l_Message = p_Args;
        l_ReplaceAll(l_Message, "$$n", "\n");

        GMTicketResponse l_Response = GMTICKET_RESPONSE_UPDATE_ERROR;
        if (GmTicket* l_Ticket = sTicketMgr->GetTicketByPlayer(l_Player->GetGUID()))
        {
            SQLTransaction l_Transaction = SQLTransaction(NULL);
            l_Ticket->SetMessage(l_Message);
            l_Ticket->SaveToDB(l_Transaction);

            sWorld->SendGMText(LANG_COMMAND_TICKETUPDATED, l_Player->GetName(), l_Ticket->GetId());

            sTicketMgr->SendTicket(p_Handler->GetSession(), l_Ticket);
            l_Response = GMTICKET_RESPONSE_UPDATE_SUCCESS;
        }

        p_Handler->GetSession()->SendTicketStatusUpdate(l_Response);

        return true;
    }


    static bool Nova_Client_HandleGMTicketAppend(ChatHandler* p_Handler, char const* p_Args)
    {
        auto l_ReplaceAll = [](std::string& str, const std::string& from, const std::string& to) {
            if (from.empty())
                return;
            size_t start_pos = 0;
            while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
            }
            };

        // Don't accept tickets if the ticket queue is disabled. (Ticket UI is greyed out but not fully dependable)
        if (sTicketMgr->GetStatus() == GMTICKET_QUEUE_STATUS_DISABLED)
            return true;

        Player* l_Player = p_Handler->GetSession()->GetPlayer();

        if (!l_Player)
            return true;

        std::string l_Message = p_Args;
        l_ReplaceAll(l_Message, "$$n", "\n");

        GMTicketResponse l_Response = GMTICKET_RESPONSE_UPDATE_ERROR;
        if (GmTicket* l_Ticket = sTicketMgr->GetTicketByPlayer(l_Player->GetGUID()))
        {
            SQLTransaction l_Transaction = SQLTransaction(NULL);
            l_Ticket->SetMessage(l_Ticket->GetMessage() + l_Message);
            l_Ticket->SaveToDB(l_Transaction);
        }

        p_Handler->GetSession()->SendTicketStatusUpdate(l_Response);

        return true;
    }


    static bool Nova_Client_HandleGMTicketEnd(ChatHandler* p_Handler, char const* /*p_Args*/)
    {
        // Don't accept tickets if the ticket queue is disabled. (Ticket UI is greyed out but not fully dependable)
        if (sTicketMgr->GetStatus() == GMTICKET_QUEUE_STATUS_DISABLED)
            return true;

        Player* l_Player = p_Handler->GetSession()->GetPlayer();

        if (!l_Player)
            return true;

        GMTicketResponse l_Response = GMTICKET_RESPONSE_UPDATE_ERROR;
        if (GmTicket* l_Ticket = sTicketMgr->GetTicketByPlayer(l_Player->GetGUID()))
        {
            sTicketMgr->SendTicket(p_Handler->GetSession(), l_Ticket);
        }

        return true;
    }

    static bool Nova_Client_HandleGMTicketDelete(ChatHandler* p_Handler, char const* /*p_Args*/)
    {
        Player* l_Player = p_Handler->GetSession()->GetPlayer();

        if (!l_Player)
            return true;

        if (GmTicket* l_Ticket = sTicketMgr->GetTicketByPlayer(l_Player->GetGUID()))
        {
            p_Handler->GetSession()->SendTicketStatusUpdate(GMTICKET_RESPONSE_TICKET_DELETED);

            sWorld->SendGMText(LANG_COMMAND_TICKETPLAYERABANDON, l_Player->GetName(), l_Ticket->GetId());

            sTicketMgr->CloseTicket(l_Ticket->GetId(), l_Player->GetGUID());
            sTicketMgr->SendTicket(p_Handler->GetSession(), NULL);
        }

        return true;
    }

    static bool Nova_Client_HandleGMTicketGet(ChatHandler* p_Handler, char const* /*p_Args*/)
    {
        Player* l_Player = p_Handler->GetSession()->GetPlayer();

        if (!l_Player)
            return true;

        p_Handler->GetSession()->OnGMTicketGetTicketEvent();

        return true;
    }

    static bool Nova_Client_HandleGMResponseResolve(ChatHandler* p_Handler, char const* /*p_Args*/)
    {
        Player* l_Player = p_Handler->GetSession()->GetPlayer();

        if (!l_Player)
            return true;

        if (GmTicket* l_Ticket = sTicketMgr->GetTicketByPlayer(l_Player->GetGUID()))
        {
            p_Handler->GetSession()->SendTicketStatusUpdate(GMTICKET_RESPONSE_TICKET_DELETED);

            sTicketMgr->CloseTicket(l_Ticket->GetId(), l_Player->GetGUID());
            sTicketMgr->SendTicket(p_Handler->GetSession(), NULL);
        }

        return true;
    }

    static bool HandleGMTicketAssignToCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* ticketIdStr = strtok((char*)args, " ");
        uint32 ticketId = atoi(ticketIdStr);

        char* targetStr = strtok(NULL, " ");
        if (!targetStr)
            return false;

        std::string target(targetStr);
        if (!normalizePlayerName(target))
            return false;

        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Get target information
        ObjectGuid targetGuid = ObjectMgr::GetPlayerGUIDByName(target.c_str());
        uint32 targetAccountId = ObjectMgr::GetPlayerAccountIdByGUID(targetGuid);
        uint32 targetGmLevel = AccountMgr::GetSecurity(targetAccountId, realm.Id.Realm);

        // Target must exist and have administrative rights
        if (!targetGuid || AccountMgr::IsPlayerAccount(targetGmLevel))
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETASSIGNERROR_A);
            return true;
        }

        // If already assigned, leave
        if (ticket->IsAssignedTo(targetGuid))
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETASSIGNERROR_B, ticket->GetId());
            return true;
        }

        // If assigned to different player other than current, leave
        //! Console can override though
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL;
        if (player && ticket->IsAssignedNotTo(player->GetGUID()))
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId(), target.c_str());
            return true;
        }

        // Assign ticket
        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetAssignedTo(targetGuid, AccountMgr::IsAdminAccount(targetGmLevel));
        ticket->SaveToDB(trans);
        sTicketMgr->UpdateLastChange();

        std::string msg = ticket->FormatMessageString(*handler, NULL, target.c_str(), NULL, NULL);
        handler->SendGlobalGMSysMessage(msg.c_str());
        return true;
    }

    static bool HandleGMTicketCloseByIdCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed() || ticket->IsCompleted())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Ticket should be assigned to the player who tries to close it.
        // Console can override though
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL;
        if (player && ticket->IsAssignedNotTo(player->GetGUID()))
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETCANNOTCLOSE, ticket->GetId());
            return true;
        }
        ObjectGuid closedByGuid;
        if (player)
            closedByGuid = player->GetGUID();
        else
            closedByGuid.SetRawValue(0, uint64(-1));

        sTicketMgr->CloseTicket(ticket->GetId(), closedByGuid);
        sTicketMgr->UpdateLastChange();

        std::string msg = ticket->FormatMessageString(*handler, player ? player->GetName() : "Console", NULL, NULL, NULL);
        handler->SendGlobalGMSysMessage(msg.c_str());

        // Inform player, who submitted this ticket, that it is closed
        if (Player* submitter = ticket->GetPlayer())
        {
            if (submitter->IsInWorld())
            {
                /*WorldPacket data(SMSG_GMTICKET_DELETETICKET, 4);
                WorldPacket data(SMSG_GM_TICKET_UPDATE, 4);
                data << uint8(GMTICKET_RESPONSE_TICKET_DELETED);
                submitter->SendDirectMessage(&data);*/
            }
        }
        return true;
    }

    static bool HandleGMTicketCommentCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        char* ticketIdStr = strtok((char*)args, " ");
        uint32 ticketId = atoi(ticketIdStr);

        char* comment = strtok(NULL, "\n");
        if (!comment)
            return false;

        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed())
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Cannot comment ticket assigned to someone else
        //! Console excluded
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL;
        if (player && ticket->IsAssignedNotTo(player->GetGUID()))
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());
            return true;
        }

        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetComment(comment);
        ticket->SaveToDB(trans);
        sTicketMgr->UpdateLastChange();

        std::string msg = ticket->FormatMessageString(*handler, NULL, ticket->GetAssignedToName().c_str(), NULL, NULL);
        msg += handler->PGetParseString(LANG_COMMAND_TICKETLISTADDCOMMENT, player ? player->GetName() : "Console", comment);
        handler->SendGlobalGMSysMessage(msg.c_str());

        return true;
    }

    static bool HandleGMTicketListClosedCommand(ChatHandler* handler, char const* /*args*/)
    {
        sTicketMgr->ShowClosedList(*handler);
        return true;
    }

    static bool HandleGMTicketCompleteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed() || ticket->IsCompleted())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        ticket->SetCompleted(true);

        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        ticket->SaveToDB(trans);
        CharacterDatabase.CommitTransaction(trans);

        //if (Player* player = ticket->GetPlayer())
            //if (player->IsInWorld())
                //ticket->SendResponse(player->GetSession());

        sTicketMgr->UpdateLastChange();
        return true;
    }

    static bool HandleGMTicketDeleteByIdCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket)
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        if (!ticket->IsClosed())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETCLOSEFIRST);
            return true;
        }

        std::string msg = ticket->FormatMessageString(*handler, NULL, NULL, NULL, handler->GetSession() ? handler->GetSession()->GetPlayer()->GetName() : "Console");
        handler->SendGlobalGMSysMessage(msg.c_str());

        sTicketMgr->RemoveTicket(ticket->GetId());
        sTicketMgr->UpdateLastChange();

        if (Player* player = ticket->GetPlayer())
        {
            if (player->IsInWorld())
            {
                // Force abandon ticket
                /*WorldPacket data(SMSG_GMTICKET_DELETETICKET, 4);
                WorldPacket data(SMSG_GM_TICKET_UPDATE, 4);
                data << uint8(GMTICKET_RESPONSE_TICKET_DELETED);
                player->SendDirectMessage(&data);*/
            }
        }

        return true;
    }

    static bool HandleGMTicketEscalateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed() || ticket->IsCompleted() || ticket->GetEscalatedStatus() != TICKET_UNASSIGNED)
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        ticket->SetEscalatedStatus(TICKET_IN_ESCALATION_QUEUE);

        //if (Player* player = ticket->GetPlayer())
            //if (player->IsInWorld())
                //sTicketMgr->SendTicket(player->GetSession(), ticket);

        sTicketMgr->UpdateLastChange();
        return true;
    }

    static bool HandleGMTicketListEscalatedCommand(ChatHandler* handler, char const* /*args*/)
    {
        sTicketMgr->ShowEscalatedList(*handler);
        return true;
    }

    static bool HandleGMTicketListCommand(ChatHandler* handler, char const* /*args*/)
    {
        sTicketMgr->ShowList(*handler, false);
        return true;
    }

    static bool HandleGMTicketListOnlineCommand(ChatHandler* handler, char const* /*args*/)
    {
        sTicketMgr->ShowList(*handler, true);
        return true;
    }

    static bool HandleGMTicketResetCommand(ChatHandler* handler, char const* /*args*/)
    {
        if (sTicketMgr->GetOpenTicketCount())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETPENDING);
            return true;
        }
        sTicketMgr->ResetTickets();
        handler->SendSysMessage(LANG_COMMAND_TICKETRESET);
        return true;
    }

    static bool HandleToggleGMTicketSystem(ChatHandler* handler, char const* /*args*/)
    {
        bool status = !sTicketMgr->GetStatus();
        sTicketMgr->SetStatus(status);
        handler->PSendSysMessage(status ? LANG_ALLOW_TICKETS : LANG_DISALLOW_TICKETS);
        return true;
    }

    static bool HandleGMTicketUnAssignCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }
        // Ticket must be assigned
        if (!ticket->IsAssigned())
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETNOTASSIGNED, ticket->GetId());
            return true;
        }

        // Get security level of player, whom this ticket is assigned to
        uint32 security = SEC_PLAYER;
        Player* assignedPlayer = ticket->GetAssignedPlayer();
        if (assignedPlayer && assignedPlayer->IsInWorld())
            security = assignedPlayer->GetSession()->GetSecurity();
        else
        {
            ObjectGuid guid = ticket->GetAssignedToGUID();
            uint32 accountId = ObjectMgr::GetPlayerAccountIdByGUID(guid);
            security = AccountMgr::GetSecurity(accountId, realm.Id.Realm);
        }

        // Check security
        //! If no m_session present it means we're issuing this command from the console
        uint32 mySecurity = handler->GetSession() ? handler->GetSession()->GetSecurity() : SEC_CONSOLE;
        if (security > mySecurity)
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETUNASSIGNSECURITY);
            return true;
        }

        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetUnassigned();
        ticket->SaveToDB(trans);
        sTicketMgr->UpdateLastChange();

        std::string msg = ticket->FormatMessageString(*handler, NULL, ticket->GetAssignedToName().c_str(),
            handler->GetSession() ? handler->GetSession()->GetPlayer()->GetName() : "Console", NULL);
        handler->SendGlobalGMSysMessage(msg.c_str());

        return true;
    }

    static bool HandleGMTicketGetByIdCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        uint32 ticketId = atoi(args);
        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed() || ticket->IsCompleted())
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetViewed();
        ticket->SaveToDB(trans);

        handler->SendSysMessage(ticket->FormatMessageString(*handler, true).c_str());
        return true;
    }

    static bool HandleGMTicketGetByNameCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        std::string name(args);
        if (!normalizePlayerName(name))
            return false;

        // Detect target's GUID
        ObjectGuid guid;
        if (Player* player = sObjectAccessor->FindPlayerByName(name))
            guid = player->GetGUID();
        else
            guid = ObjectMgr::GetPlayerGUIDByName(name);

        // Target must exist
        if (!guid)
        {
            handler->SendSysMessage(LANG_NO_PLAYERS_FOUND);
            return true;
        }

        // Ticket must exist
        GmTicket* ticket = sTicketMgr->GetTicketByPlayer(guid);
        if (!ticket)
        {
            handler->SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        SQLTransaction trans = SQLTransaction(NULL);
        ticket->SetViewed();
        ticket->SaveToDB(trans);

        handler->SendSysMessage(ticket->FormatMessageString(*handler, true).c_str());
        return true;
    }

    static bool _HandleGMTicketResponseAppendCommand(char const* args, bool newLine, ChatHandler* handler)
    {
        if (!*args)
            return false;

        char* ticketIdStr = strtok((char*)args, " ");
        uint32 ticketId = atoi(ticketIdStr);

        char* response = strtok(NULL, "\n");
        if (!response)
            return false;

        GmTicket* ticket = sTicketMgr->GetTicket(ticketId);
        if (!ticket || ticket->IsClosed())
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
            return true;
        }

        // Cannot add response to ticket, assigned to someone else
        //! Console excluded
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : NULL;
        if (player && ticket->IsAssignedNotTo(player->GetGUID()))
        {
            handler->PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->GetId());
            return true;
        }

        SQLTransaction trans = SQLTransaction(NULL);
        ticket->AppendResponse(response);
        if (newLine)
            ticket->AppendResponse("\n");
        ticket->SaveToDB(trans);

        return true;
    }

    static bool HandleGMTicketResponseAppendCommand(ChatHandler* handler, char const* args)
    {
        return _HandleGMTicketResponseAppendCommand(args, false, handler);
    }

    static bool HandleGMTicketResponseAppendLnCommand(ChatHandler* handler, char const* args)
    {
        return _HandleGMTicketResponseAppendCommand(args, true, handler);
    }
};

void AddSC_ticket_commandscript()
{
    new ticket_commandscript();
}
