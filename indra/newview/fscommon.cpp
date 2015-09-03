/**
 * @file fscommon.cpp
 * @brief Central class for common used functions in Firestorm
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Phoenix Firestorm Viewer Source Code
 * Copyright (c) 2012 Ansariel Hiller @ Second Life
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
 * http://www.firestormviewer.org
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "fscommon.h"
#include "fsradar.h"
#include "llagent.h"
#include "llavataractions.h"
#include "llavatarnamecache.h"
#include "llfloaterperms.h"
#include "llinventorymodel.h"
#include "lllogchat.h"
#include "llmutelist.h"
#include "llnotificationmanager.h"
#include "lltooldraganddrop.h"
#include "llviewerinventory.h"
#include "llviewernetwork.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "rlvactions.h"
#include "rlvhandler.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;
using namespace boost::gregorian;

static const std::string LL_LINDEN = "Linden";
static const std::string LL_MOLE = "Mole";
static const std::string LL_PRODUCTENGINE = "ProductEngine";
static const std::string LL_SCOUT = "Scout";
static const std::string LL_TESTER = "Tester";


extern S32 gMaxAgentGroups;

S32 FSCommon::sObjectAddMsg = 0;

void reportToNearbyChat(const std::string& message)
{
	LLChat chat;
	chat.mText = message;
	chat.mSourceType = CHAT_SOURCE_SYSTEM;
	LLNotificationsUI::LLNotificationManager::instance().onChat(chat, LLSD());
}

std::string applyAutoCloseOoc(const std::string& message)
{
	if (!gSavedSettings.getBOOL("AutoCloseOOC"))
	{
		return message;
	}

	std::string utf8_text(message);

	// Try to find any unclosed OOC chat (i.e. an opening
	// double parenthesis without a matching closing double
	// parenthesis.
	if (utf8_text.find("(( ") != -1 && utf8_text.find("))") == -1)
	{
		// add the missing closing double parenthesis.
		utf8_text += " ))";
	}
	else if (utf8_text.find("((") != -1 && utf8_text.find("))") == -1)
	{
		if (utf8_text.at(utf8_text.length() - 1) == ')')
		{
			// cosmetic: add a space first to avoid a closing triple parenthesis
			utf8_text += " ";
		}
		// add the missing closing double parenthesis.
		utf8_text += "))";
	}
	else if (utf8_text.find("[[ ") != -1 && utf8_text.find("]]") == -1)
	{
		// add the missing closing double parenthesis.
		utf8_text += " ]]";
	}
	else if (utf8_text.find("[[") != -1 && utf8_text.find("]]") == -1)
	{
		if (utf8_text.at(utf8_text.length() - 1) == ']')
		{
			// cosmetic: add a space first to avoid a closing triple parenthesis
			utf8_text += " ";
		}
			// add the missing closing double parenthesis.
		utf8_text += "]]";
	}

	return utf8_text;
}

std::string applyMuPose(const std::string& message)
{
	std::string utf8_text(message);

	// Convert MU*s style poses into IRC emotes here.
	if (gSavedSettings.getBOOL("AllowMUpose") && utf8_text.find(":") == 0 && utf8_text.length() > 3)
	{
		if (utf8_text.find(":'") == 0)
		{
			utf8_text.replace(0, 1, "/me");
 		}
		else if (!isdigit(utf8_text.at(1)) && !ispunct(utf8_text.at(1)) && !isspace(utf8_text.at(1)))	// Do not prevent smileys and such.
		{
			utf8_text.replace(0, 1, "/me ");
		}
	}

	return utf8_text;
}

std::string formatString(std::string text, const LLStringUtil::format_map_t& args)
{
	LLStringUtil::format(text, args);
	return text;
}

S32 FSCommon::secondsSinceEpochFromString(const std::string& format, const std::string& str)
{
	// LLDateUtil::secondsSinceEpochFromString does not handle time, only the date.
	// copied that function here and added the needed code to handle time fields.  -- TL
	time_input_facet *facet = new time_input_facet(format);
	std::stringstream ss;
	ss << str;
	ss.imbue(std::locale(ss.getloc(), facet));
	ptime time_t_date;
	ss >> time_t_date;
	ptime time_t_epoch(date(1970,1,1));
	time_duration diff = time_t_date - time_t_epoch;
	return diff.total_seconds();
}

void FSCommon::applyDefaultBuildPreferences(LLViewerObject* object)
{
	if (!object)
	{
		return;
	}
  
	LLTextureEntry texture_entry;
	texture_entry.setID(LLUUID(gSavedSettings.getString("FSDefaultObjectTexture")));
	texture_entry.setColor(gSavedSettings.getColor4("FSBuildPrefs_Color"));
	texture_entry.setAlpha((100.f - gSavedSettings.getF32("FSBuildPrefs_Alpha")) / 100.f);
	texture_entry.setGlow(gSavedSettings.getF32("FSBuildPrefs_Glow"));
	if(gSavedSettings.getBOOL("FSBuildPrefs_FullBright"))
	{
		texture_entry.setFullbright(TEM_FULLBRIGHT_MASK);
	}
	
	U8 shiny = 0; // Default none
	std::string shininess = gSavedSettings.getString("FSBuildPrefs_Shiny");
	if(shininess == "Low")
	{
		shiny = 1;
	}
	else if(shininess == "Medium")
	{
		shiny = 2;
	}
	else if(shininess == "High")
	{
		shiny = 3;
	}
	texture_entry.setShiny(shiny);
	
	for(U8 face = 0; face < object->getNumTEs(); face++)
	{
		object->setTE(face, texture_entry);
	}
	object->sendTEUpdate();
	
	if (gSavedPerAccountSettings.getBOOL("FSBuildPrefs_EmbedItem"))
	{
		LLUUID item_id(gSavedPerAccountSettings.getString("FSBuildPrefs_Item"));
		if (item_id.notNull())
		{
			LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(gInventory.getObject(item_id));
			if (item)
			{
				if (item->getType() == LLAssetType::AT_LSL_TEXT)
				{
					LLToolDragAndDrop::dropScript(object, item, TRUE,
									  LLToolDragAndDrop::SOURCE_AGENT,
									  gAgentID);
				}
				else
				{
					LLToolDragAndDrop::dropInventory(object, item,
									LLToolDragAndDrop::SOURCE_AGENT,
									gAgentID);
				}
			}
		}
	}

	U32 object_local_id = object->getLocalID();

#ifdef OPENSIM
	if (!LLGridManager::getInstance()->isInSecondLife() || !LLFloaterPermsDefault::getCapSent())
#else
	if (!LLFloaterPermsDefault::getCapSent())
#endif
	{
		gMessageSystem->newMessageFast(_PREHASH_ObjectPermissions);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_HeaderData);
		gMessageSystem->addBOOLFast(_PREHASH_Override, (BOOL)FALSE);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object_local_id);
		gMessageSystem->addU8Fast(_PREHASH_Field, PERM_NEXT_OWNER);
		gMessageSystem->addBOOLFast(_PREHASH_Set, gSavedSettings.getBOOL("ObjectsNextOwnerModify"));
		gMessageSystem->addU32Fast(_PREHASH_Mask, PERM_MODIFY);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object_local_id);
		gMessageSystem->addU8Fast(_PREHASH_Field, PERM_NEXT_OWNER);
		gMessageSystem->addBOOLFast(_PREHASH_Set, gSavedSettings.getBOOL("ObjectsNextOwnerCopy"));
		gMessageSystem->addU32Fast(_PREHASH_Mask, PERM_COPY);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object_local_id);
		gMessageSystem->addU8Fast(_PREHASH_Field, PERM_NEXT_OWNER);
		gMessageSystem->addBOOLFast(_PREHASH_Set, gSavedSettings.getBOOL("ObjectsNextOwnerTransfer"));
		gMessageSystem->addU32Fast(_PREHASH_Mask, PERM_TRANSFER);
		gMessageSystem->sendReliable(object->getRegion()->getHost());
	}

	gMessageSystem->newMessage(_PREHASH_ObjectFlagUpdate);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object_local_id);
	gMessageSystem->addBOOLFast(_PREHASH_UsePhysics, gSavedSettings.getBOOL("FSBuildPrefs_Physical"));
	gMessageSystem->addBOOL(_PREHASH_IsTemporary, gSavedSettings.getBOOL("FSBuildPrefs_Temporary"));
	gMessageSystem->addBOOL(_PREHASH_IsPhantom, gSavedSettings.getBOOL("FSBuildPrefs_Phantom"));
	gMessageSystem->addBOOL("CastsShadows", FALSE );
	gMessageSystem->sendReliable(object->getRegion()->getHost());
}

bool FSCommon::isLinden(const LLUUID& av_id)
{
	std::string first_name, last_name;
	LLAvatarName av_name;
	if (LLAvatarNameCache::get(av_id, &av_name))
	{
		std::istringstream full_name(av_name.getUserName());
		full_name >> first_name >> last_name;
	}
	else
	{
		gCacheName->getFirstLastName(av_id, first_name, last_name);
	}
#ifdef OPENSIM
	if (LLGridManager::getInstance()->isInOpenSim())
	{
		LLViewerRegion* region = gAgent.getRegion();
		if (!region) return false;
		bool is_god = false;
		// <FS:CR> They may not be "Lindens" per se, but opensim has gods.
		std::set<std::string> gods = region->getGods();
		if (!gods.empty())
		{
			is_god = (gods.find(first_name + " " + last_name) != gods.end()
					  || gods.find(last_name) != gods.end());
		}
		return is_god;
	}
#endif
	return (last_name == LL_LINDEN ||
			last_name == LL_MOLE ||
			last_name == LL_PRODUCTENGINE ||
			last_name == LL_SCOUT ||
			last_name == LL_TESTER);
}


bool FSCommon::checkIsActionEnabled(const LLUUID& av_id, EFSRegistrarFunctionActionType action)
{
	bool isSelf = (av_id == gAgentID);

	if (action == FS_RGSTR_ACT_ADD_FRIEND)
	{
		return (!isSelf && !LLAvatarActions::isFriend(av_id));
	}
	else if (action == FS_RGSTR_ACT_REMOVE_FRIEND)
	{
		return (!isSelf && LLAvatarActions::isFriend(av_id));
	}
	else if (action == FS_RGSTR_ACT_SEND_IM)
	{
		return (!isSelf && RlvActions::canStartIM(av_id));
	}
	else if (action == FS_RGSTR_ACT_VIEW_TRANSCRIPT)
	{
		return (!isSelf && LLLogChat::isTranscriptExist(av_id));
	}
	else if (action == FS_RGSTR_ACT_ZOOM_IN)
	{
		return (!isSelf && LLAvatarActions::canZoomIn(av_id));
	}
	else if (action == FS_RGSTR_ACT_OFFER_TELEPORT)
	{
		return (!isSelf && LLAvatarActions::canOfferTeleport(av_id));
	}
	else if (action == FS_RGSTR_ACT_REQUEST_TELEPORT)
	{
		return (!isSelf && LLAvatarActions::canRequestTeleport(av_id));
	}
	else if (action == FS_RGSTR_ACT_SHOW_PROFILE)
	{
		return (isSelf || !gRlvHandler.hasBehaviour(RLV_BHVR_SHOWNAMES));
	}
	else if (action == FS_RGSTR_ACT_TRACK_AVATAR)
	{
		return (!isSelf && FSRadar::getInstance()->getEntry(av_id) != NULL);
	}
	else if (action == FS_RGSTR_ACT_TELEPORT_TO)
	{
		return (!isSelf && FSRadar::getInstance()->getEntry(av_id) != NULL);
	}
	else if (action == FS_RGSTR_CHK_AVATAR_BLOCKED)
	{
		return (!isSelf && LLMuteList::getInstance()->isMuted(av_id));
	}
	else if (action == FS_RGSTR_CHK_IS_SELF)
	{
		return isSelf;
	}
	else if (action == FS_RGSTR_CHK_IS_NOT_SELF)
	{
		return !isSelf;
	}

	return false;
}

LLSD FSCommon::populateGroupCount()
{
	LLStringUtil::format_map_t args;
	S32 groupcount = gAgent.mGroups.size();
	args["[COUNT]"] = llformat("%d", groupcount);
	args["[REMAINING]"] = llformat("%d", gMaxAgentGroups - groupcount);
	LLUIString groupcountstring = LLTrans::getString((gMaxAgentGroups ? "groupcountstring" : "groupcountunlimitedstring"), args);
	return LLSD(groupcountstring);
}

std::string FSCommon::getAvatarNameByDisplaySettings(const LLAvatarName& av_name)
{
	std::string name;
	static LLCachedControl<bool> NameTagShowUsernames(gSavedSettings, "NameTagShowUsernames");
	static LLCachedControl<bool> UseDisplayNames(gSavedSettings, "UseDisplayNames");
	if ((NameTagShowUsernames) && (UseDisplayNames))
	{
		name = av_name.getCompleteName();
	}
	else if (UseDisplayNames)
	{
		name = av_name.getDisplayName();
	}
	else
	{
		name = av_name.getUserNameForDisplay();
	}
	return name;
}

bool FSCommon::isDefaultTexture(const LLUUID& asset_id)
{
	if (asset_id == LL_DEFAULT_WOOD_UUID ||
		asset_id == LL_DEFAULT_STONE_UUID ||
		asset_id == LL_DEFAULT_METAL_UUID ||
		asset_id == LL_DEFAULT_GLASS_UUID ||
		asset_id == LL_DEFAULT_FLESH_UUID ||
		asset_id == LL_DEFAULT_PLASTIC_UUID ||
		asset_id == LL_DEFAULT_RUBBER_UUID ||
		asset_id == LL_DEFAULT_LIGHT_UUID ||
		asset_id == LLUUID("5748decc-f629-461c-9a36-a35a221fe21f") ||	// UIImgWhiteUUID
		asset_id == LLUUID("8dcd4a48-2d37-4909-9f78-f7a9eb4ef903") ||	// UIImgTransparentUUID
		asset_id == LLUUID("f54a0c32-3cd1-d49a-5b4f-7b792bebc204") ||	// UIImgInvisibleUUID
		asset_id == LLUUID("6522e74d-1660-4e7f-b601-6f48c1659a77") ||	// UIImgDefaultEyesUUID
		asset_id == LLUUID("7ca39b4c-bd19-4699-aff7-f93fd03d3e7b") ||	// UIImgDefaultHairUUID
		asset_id == LLUUID("5748decc-f629-461c-9a36-a35a221fe21f")		// UIImgDefault for all clothing
	   )
	{
		return true;
	}
	return false;
}

bool FSCommon::isLegacySkin()
{
	std::string current_skin = gSavedSettings.getString("FSInternalSkinCurrent");
	return (current_skin == "Vintage" || current_skin == "Latency");
}
