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
#include "TicketMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "TicketPackets.h"
#include "Chat.h"
#include "World.h"
#include "MiscPackets.h"

inline float GetAge(uint64 t) { return float(time(nullptr) - t) / DAY; }

///////////////////////////////////////////////////////////////////////////////////////////////////
// GM ticket
GmTicket::GmTicket() { }

GmTicket::GmTicket(std::string p_PlayerName, ObjectGuid p_PlayerGuid, uint32 p_MapID, WorldLocation p_Position, std::string p_Content) : _createTime(time(NULL)), _lastModifiedTime(time(NULL)), _closedBy(ObjectGuid::Empty), _assignedTo(ObjectGuid::Empty), _completed(false),
_escalatedStatus(TICKET_UNASSIGNED), _viewed(false), m_NeedResponse(false), m_HaveTicket(false)
{
    _id = sTicketMgr->GenerateTicketId();
    _playerName = p_PlayerName;
    _playerGuid = p_PlayerGuid;
    _mapId = p_MapID;
    _pos = p_Position.m_positionX;
    _pos = p_Position.m_positionY;
    _pos = p_Position.m_positionZ;
    m_HaveTicket = false;    ///< unk ?
    m_NeedResponse = true;     ///< Lua constant

    _message = p_Content;
}


GmTicket::~GmTicket() { }

bool GmTicket::LoadFromDB(Field* fields)
{
    //     0       1     2      3          4        5      6     7     8           9            10         11         12        13        14        15
    // ticketId, guid, name, message, createTime, mapId, posX, posY, posZ, lastModifiedTime, closedBy, assignedTo, comment, completed, escalated, viewed
    uint8 index = 0;
    _id                 = fields[  index].GetUInt32();
    _playerGuid         = ObjectGuid::Create<HighGuid::Player>(fields[++index].GetUInt64());
    _playerName         = fields[++index].GetString();
    _message            = fields[++index].GetString();
    _createTime         = fields[++index].GetUInt32();
    _mapId              = fields[++index].GetUInt16();
    _pos                = Position(fields[++index].GetFloat(), fields[++index].GetFloat(), fields[++index].GetFloat());
    _lastModifiedTime   = fields[++index].GetUInt32();
    int64 closedBy = fields[++index].GetInt64();
    if (closedBy < 0)
        _closedBy.SetRawValue(0, uint64(closedBy));
    else if (!closedBy)
        _closedBy = ObjectGuid::Empty;
    else
        _closedBy = ObjectGuid::Create<HighGuid::Player>(uint64(closedBy));

    _assignedTo         = ObjectGuid::Create<HighGuid::Player>(fields[++index].GetUInt64());
    _comment            = fields[++index].GetString();
    _completed          = fields[++index].GetBool();
    _escalatedStatus    = GMTicketEscalationStatus(fields[++index].GetUInt8());
    _viewed             = fields[++index].GetBool();
    return true;
}

void GmTicket::SaveToDB(SQLTransaction& trans) const
{
    //     0       1     2      3          4        5      6     7     8           9            10         11         12        13        14        15
    // ticketId, guid, name, message, createTime, mapId, posX, posY, posZ, lastModifiedTime, closedBy, assignedTo, comment, completed, escalated, viewed
    uint8 index = 0;
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_GM_TICKET);
    stmt->setUInt32(  index, _id);
    stmt->setUInt64(++index, _playerGuid.GetCounter());
    stmt->setString(++index, _playerName);
    stmt->setString(++index, _message);
    stmt->setUInt32(++index, uint32(_createTime));
    stmt->setUInt16(++index, _mapId);
    stmt->setFloat (++index, _pos.GetPositionX());
    stmt->setFloat (++index, _pos.GetPositionY());
    stmt->setFloat (++index, _pos.GetPositionZ());
    stmt->setUInt32(++index, uint32(_lastModifiedTime));
    stmt->setInt64 (++index, int64(_closedBy.GetCounter()));
    stmt->setUInt64(++index, _assignedTo.GetCounter());
    stmt->setString(++index, _comment);
    stmt->setBool  (++index, _completed);
    stmt->setUInt8 (++index, uint8(_escalatedStatus));
    stmt->setBool  (++index, _viewed);

    CharacterDatabase.ExecuteOrAppend(trans, stmt);
}

void GmTicket::DeleteFromDB()
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GM_TICKET);
    stmt->setUInt32(0, _id);
    CharacterDatabase.Execute(stmt);
}

std::string GmTicket::FormatMessageString(ChatHandler& handler, bool detailed) const
{
    time_t curTime = time(nullptr);

    std::stringstream ss;
    std::string nameLink = handler.playerLink(_playerName);

    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTGUID, _id);
    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTNAME, nameLink.c_str());
    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTAGECREATE, (secsToTimeString(curTime - _createTime, true, false)).c_str());
    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTAGE, (secsToTimeString(curTime - _lastModifiedTime, true, false)).c_str());

    std::string name;
    if (ObjectMgr::GetPlayerNameByGUID(_assignedTo, name))
        ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, name.c_str());

    if (detailed)
    {
        ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTMESSAGE, _message.c_str());
        if (!_comment.empty())
            ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTCOMMENT, _comment.c_str());
    }
    return ss.str();
}

std::string GmTicket::FormatMessageString(ChatHandler& handler, const char* szClosedName, const char* szAssignedToName, const char* szUnassignedName, const char* szDeletedName) const
{
    std::stringstream ss;
    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTGUID, _id);
    ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTNAME, _playerName.c_str());
    if (szClosedName)
        ss << handler.PGetParseString(LANG_COMMAND_TICKETCLOSED, szClosedName);
    if (szAssignedToName)
        ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, szAssignedToName);
    if (szUnassignedName)
        ss << handler.PGetParseString(LANG_COMMAND_TICKETLISTUNASSIGNED, szUnassignedName);
    if (szDeletedName)
        ss << handler.PGetParseString(LANG_COMMAND_TICKETDELETED, szDeletedName);
    return ss.str();
}

void GmTicket::SetUnassigned()
{
    _assignedTo.Clear();
    switch (_escalatedStatus)
    {
        case TICKET_ASSIGNED: _escalatedStatus = TICKET_UNASSIGNED; break;
        case TICKET_ESCALATED_ASSIGNED: _escalatedStatus = TICKET_IN_ESCALATION_QUEUE; break;
        case TICKET_UNASSIGNED:
        case TICKET_IN_ESCALATION_QUEUE:
        default:
            break;
    }
}

void GmTicket::TeleportTo(Player* player) const
{
    player->TeleportTo(_mapId, &_pos, 0.0f, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Ticket manager
TicketMgr::TicketMgr() : _status(true), _lastTicketId(0), _lastSurveyId(0), _openTicketCount(0), _lastChange(time(nullptr)) { }

TicketMgr::~TicketMgr()
{
    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        delete itr->second;
}

void TicketMgr::Initialize() { SetStatus(sWorld->getBoolConfig(CONFIG_ALLOW_TICKETS)); }

void TicketMgr::ResetTickets()
{
    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        if (itr->second->IsClosed())
            sTicketMgr->RemoveTicket(itr->second->GetId());

    _lastTicketId = 0;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ALL_GM_TICKETS);

    CharacterDatabase.Execute(stmt);
}

void TicketMgr::LoadTickets()
{
    uint32 oldMSTime = getMSTime();

    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        delete itr->second;
    _ticketList.clear();

    _lastTicketId = 0;
    _openTicketCount = 0;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GM_TICKETS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        TC_LOG_INFO(LOG_FILTER_SERVER_LOADING, ">> Loaded 0 GM tickets. DB table `gm_tickets` is empty!");

        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        GmTicket* ticket = new GmTicket();
        if (!ticket->LoadFromDB(fields))
        {
            delete ticket;
            continue;
        }
        if (!ticket->IsClosed())
            ++_openTicketCount;

        // Update max ticket id if necessary
        uint32 id = ticket->GetId();
        if (_lastTicketId < id)
            _lastTicketId = id;

        _ticketList[id] = ticket;
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO(LOG_FILTER_SERVER_LOADING, ">> Loaded %u GM tickets in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void TicketMgr::LoadSurveys()
{
    // we don't actually load anything into memory here as there's no reason to
    _lastSurveyId = 0;

    uint32 oldMSTime = getMSTime();
    if (QueryResult result = CharacterDatabase.Query("SELECT MAX(surveyId) FROM gm_surveys"))
        _lastSurveyId = (*result)[0].GetUInt32();

    TC_LOG_INFO(LOG_FILTER_SERVER_LOADING, ">> Loaded GM Survey count from database in %u ms", GetMSTimeDiffToNow(oldMSTime));

}

void TicketMgr::AddTicket(GmTicket* ticket)
{
    _ticketList[ticket->GetId()] = ticket;
    if (!ticket->IsClosed())
        ++_openTicketCount;

    SQLTransaction trans = SQLTransaction(nullptr);
    ticket->SaveToDB(trans);
}

void TicketMgr::CloseTicket(uint32 ticketId, ObjectGuid source)
{
    if (GmTicket* ticket = GetTicket(ticketId))
    {
        SQLTransaction trans = SQLTransaction(nullptr);
        ticket->SetClosedBy(source);
        if (!source.IsEmpty())
            --_openTicketCount;
        ticket->SaveToDB(trans);
    }
}

void TicketMgr::RemoveTicket(uint32 ticketId)
{
    if (GmTicket* ticket = GetTicket(ticketId))
    {
        ticket->DeleteFromDB();
        _ticketList.erase(ticketId);
        delete ticket;
    }
}

void TicketMgr::ShowList(ChatHandler& handler, bool onlineOnly) const
{
    handler.SendSysMessage(onlineOnly ? LANG_COMMAND_TICKETSHOWONLINELIST : LANG_COMMAND_TICKETSHOWLIST);
    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        if (!itr->second->IsClosed() && !itr->second->IsCompleted())
            if (!onlineOnly || itr->second->GetPlayer())
                handler.SendSysMessage(itr->second->FormatMessageString(handler).c_str());
}

void TicketMgr::ShowClosedList(ChatHandler& handler) const
{
    handler.SendSysMessage(LANG_COMMAND_TICKETSHOWCLOSEDLIST);
    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        if (itr->second->IsClosed())
            handler.SendSysMessage(itr->second->FormatMessageString(handler).c_str());
}

void TicketMgr::ShowEscalatedList(ChatHandler& handler) const
{
    handler.SendSysMessage(LANG_COMMAND_TICKETSHOWESCALATEDLIST);
    for (GmTicketList::const_iterator itr = _ticketList.begin(); itr != _ticketList.end(); ++itr)
        if (!itr->second->IsClosed() && itr->second->GetEscalatedStatus() == TICKET_IN_ESCALATION_QUEUE)
            handler.SendSysMessage(itr->second->FormatMessageString(handler).c_str());
}

void GmTicket::SendResponse(WorldSession* p_Session) const
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

    if (!p_Session->GetPlayer())
        return;


    std::string l_Message = _message;
    std::string l_Response = _response;

    l_ReplaceAll(l_Message, "|", "/");
    l_ReplaceAll(l_Message, "\n", "$$n");
    l_ReplaceAll(l_Response, "|", "/");
    l_ReplaceAll(l_Response, "\n", "$$n");

    auto l_StringSplit = [](std::string const& p_Str, char p_Delimeter) -> std::vector<std::string>
        {
            std::vector<std::string> l_Result;

            std::stringstream l_StringStream(p_Str);
            std::string l_Item;

            while (std::getline(l_StringStream, l_Item, p_Delimeter))
                l_Result.push_back(l_Item);

            return l_Result;
        };

    auto l_Tokenize = [&l_StringSplit](std::string const& l_Message) -> std::vector<std::string>
        {
            const int l_MaxLineLenght = 180; ///< Magic value

            std::vector<std::string> l_Lines;
            std::vector<std::string> l_Words = l_StringSplit(l_Message, ' ');
            std::string l_Buffer = "";

            for (std::string const& l_Word : l_Words)
            {
                if ((l_Buffer.length() + 1 + l_Word.length()) <= l_MaxLineLenght)
                    l_Buffer += (!l_Buffer.empty() ? " " : "") + l_Word;
                else
                {
                    l_Lines.push_back(l_Buffer);
                    l_Buffer = l_Word;
                }
            }

            if (!l_Buffer.empty())
                l_Lines.push_back(l_Buffer);

            return l_Lines;
        };

    auto l_MessageTokenized = l_Tokenize(l_Message);
    auto l_ResponseTokenized = l_Tokenize(l_Response);


    p_Session->GetPlayer()->SendCustomMessage("NOVA_CLIENT_TICKET_GM_REPONSE_RECEIVED_BEG");

    for (auto l_Current : l_MessageTokenized)
    {
        std::vector<std::string> l_OutData;
        l_OutData.push_back(l_Current);

        p_Session->GetPlayer()->SendCustomMessage("NOVA_CLIENT_TICKET_GM_REPONSE_RECEIVED_UPD_A", l_OutData);
    }

    for (auto l_Current : l_ResponseTokenized)
    {
        std::vector<std::string> l_OutData;
        l_OutData.push_back(l_Current);

        p_Session->GetPlayer()->SendCustomMessage("NOVA_CLIENT_TICKET_GM_REPONSE_RECEIVED_UPD_B", l_OutData);
    }

    p_Session->GetPlayer()->SendCustomMessage("NOVA_CLIENT_TICKET_GM_REPONSE_RECEIVED_END");
}


void GmTicket::WriteData(std::vector<std::string>& p_Data, std::string & p_Message) const
{
    /// HelpFrame.lua
    /// local category, ticketDescription, ticketOpenTime, oldestTicketTime, updateTime, assignedToGM, openedByGM, waitTimeOverrideMessage, waitTimeOverrideMinutes = ...;

    auto l_ReplaceAll = [](std::string& str, const std::string& from, const std::string& to) {
        if (from.empty())
            return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    };

    std::string l_Message = GetMessage();
    l_ReplaceAll(l_Message, "|", "/");
    l_ReplaceAll(l_Message, "\n", "$$n");
    p_Message = l_Message;

    p_Data.push_back(std::to_string(uint16(std::min(_escalatedStatus, TICKET_IN_ESCALATION_QUEUE))));
    p_Data.push_back(std::to_string(GetAge(_lastModifiedTime)));
    
    if (GmTicket* ticket = sTicketMgr->GetOldestOpenTicket())
        p_Data.push_back(std::to_string(uint32(GetAge(ticket->GetLastModifiedTime()))));
    else
        p_Data.push_back(std::to_string(uint32(float(0))));

    p_Data.push_back(std::to_string(GetAge(sTicketMgr->GetLastChange())));
    p_Data.push_back(std::to_string(uint16(m_HaveTicket)));
    p_Data.push_back(std::to_string(uint16(_viewed ? GMTICKET_OPENEDBYGM_STATUS_OPENED : GMTICKET_OPENEDBYGM_STATUS_NOT_OPENED)));
    p_Data.push_back(std::to_string(uint32(0)));
}

void TicketMgr::SendTicket(WorldSession* p_Session, GmTicket* p_Ticket) const
{
     WorldPackets::Misc::DisplayGameError ticket;
    uint32 l_Status = GMTICKET_STATUS_DEFAULT;
    if (p_Ticket)
        l_Status = GMTICKET_STATUS_HASTEXT;

    if (l_Status == GMTICKET_STATUS_HASTEXT)
    {
        auto l_StringSplit = [](std::string const& p_Str, char p_Delimeter) -> std::vector<std::string>
            {
                std::vector<std::string> l_Result;

                std::stringstream l_StringStream(p_Str);
                std::string l_Item;

                while (std::getline(l_StringStream, l_Item, p_Delimeter))
                    l_Result.push_back(l_Item);

                return l_Result;
            };

        std::vector<std::string> l_Data;
        std::string l_Message = "";
        p_Ticket->WriteData(l_Data, l_Message);

        p_Session->GetPlayer()->SendCustomMessage("NOVA_CLIENT_TICKET_UPDATE_BEG");

        const int l_MaxLineLenght = 180; ///< Magic value

        std::vector<std::string> l_Words = l_StringSplit(l_Message, ' ');
        std::string l_Buffer = "";

        for (std::string const& l_Word : l_Words)
        {
            if ((l_Buffer.length() + 1 + l_Word.length()) <= l_MaxLineLenght)
                l_Buffer += (!l_Buffer.empty() ? " " : "") + l_Word;
            else
            {
                std::vector<std::string> l_OutData;
                l_OutData.push_back(l_Buffer);

                p_Session->GetPlayer()->SendCustomMessage("NOVA_CLIENT_TICKET_UPDATE_UPD", l_OutData);

                l_Buffer = l_Word;
            }
        }

        if (!l_Buffer.empty())
        {
            std::vector<std::string> l_OutData;
            l_OutData.push_back(l_Buffer);

            p_Session->GetPlayer()->SendCustomMessage("NOVA_CLIENT_TICKET_UPDATE_UPD", l_OutData);
        }

        p_Session->GetPlayer()->SendCustomMessage("NOVA_CLIENT_TICKET_UPDATE_END", l_Data);
    }
    else
    {
        if (!l_Status)
        {
            ticket.Error = UIErrors::ERR_TICKET_DB_ERROR;
        }

        if (p_Session->GetPlayer())
            p_Session->GetPlayer()->SendCustomMessage("NOVA_CLIENT_TICKET_DELETED");
    }
}
