// Copyright (c) MAN Energy Solutions

#ifndef _CHANNELS_H
#define _CHANNELS_H
#include <datatree/unitscal.h>
#include <ionet/c/ionet_common.h>
#include <ionet/ionet.h>
#include <ionet/ionet_staticwrap.h>
#include <Classes.hpp>
#include <win32util/queue.h>
#include <mqtt_interface/mqtt_publish_interface.h>
#include <iostream>
#include <string>
#include <mbedtls/sha1.h>
#include <map>
#include <datatyps/senstyps.h>


struct ListViewEvent
{
   enum Type { ON_CONNECT, ON_DISCONNECT, ON_UPDATE_VIEW, ON_NO_DATA };
   Type  type;
   void* data;
};

template <Ionet_vers vers>
class BesListView : public ionet_ListView<vers>
{
public:
   BesListView(unsigned char node, SyncQueue<ListViewEvent>* queue);
   ~BesListView();
public:
   void OnConnect(ionet_StaticMapWrap<vers>& staticData);
   void OnDisconnect();
   void OnUpdateView(ionet_ListItemWrap<vers>& listData);
   void OnNoData();
private:
   unsigned char             node_;
   SyncQueue<ListViewEvent>* queue_;
};

struct ChannelData {
   std::string channelName;
   std::string channelType;
   std::string channelDesc;
   std::string signalId;
   std::string invalidValid;
   std::string alarmState;
};

class TIoListView : public TThread
{
   typedef struct tagTHREADNAME_INFO
   {
      DWORD  dwType;
      LPCSTR szName;
      DWORD  dwThreadID; 
      DWORD  dwFlags; 
   } THREADNAME_INFO;
   
private:
   void       SetName();
   AnsiString GenerateNodeString(unsigned char node, unsigned char mode);
   template <Ionet_vers vers>
   void OnConnectFormHandling(ionet_StaticMapWrapContainer& data);
   template <Ionet_vers vers>
   void OnUpdateViewFormHandling(ionet_ListItemWrap<vers>& data);
protected:
   void __fastcall Execute();
public:
   __fastcall TIoListView(unsigned char node);
   unsigned char                 node() { return node_; }
   ionet_StaticMapWrapContainer& staticData() { return staticData_; }
public:
   void __fastcall OnConnect();
   void __fastcall OnDisconnect();
   void __fastcall OnUpdateView();
   void __fastcall OnNoData();
   std::map<std::string, std::string> hashMap;
   std::string generateSha(const ChannelData& channelData);
   void publishData(const ChannelData& chanData, std::string staticState);
   void processData(const ChannelData& chanData, std::string staticState);
private:
   void __fastcall QueueCleanup();
   unsigned char                node_;
   SyncQueue<ListViewEvent>     queue_;
   ListViewEvent                evt_;
   ionet_StaticMapWrapContainer staticData_;
   unsigned char                ionetVersion_;
};

template<Ionet_vers vers>
void TIoListView::OnUpdateViewFormHandling(ionet_ListItemWrap<vers>& data)
{
   std::string staticState = "NONSTATIC"; 
   for( ionet_ListItemWrap<vers>::ListItemInterface_iterator it = data.begin(); it != data.end(); ++it) 
   {
      ChannelData chanData;
      Ionet_data<vers>::ChannelKey     c = it->first;
      ionet_ListItemInterface<vers>& ld    = it->second;
      ionet_StaticMapWrap<vers>*  s;
      staticData_.get(&s);
      char channelName[50] = {'\0'};
      sprintf(channelName, "%s", AnsiString(Ionet_string<vers>::key_string(c).c_str()));
      chanData.channelName = channelName;
      char invalidValid[50] = {'\0'};
      sprintf(invalidValid, "%s", ld.invalid() ? "INVALID" : "VALID");
      chanData.invalidValid = invalidValid;
      char alarmState[50] = {'\0'};
      sprintf(alarmState, "%s", ld.alarm() ? "ALARM" : "NORMAL");
      chanData.alarmState = alarmState;
      processData(chanData, staticState);
   }
}

template <Ionet_vers vers>
void TIoListView::OnConnectFormHandling(ionet_StaticMapWrapContainer& data)
{
  ionet_StaticMapWrap<vers>*              sd;
  std::string staticState = "STATIC"; 
  data.get(&sd);
  if(sd->size() == 0)
  {
    OnDisconnect();
  }
  else 
  {
     for(ionet_StaticMapWrap<vers>::StaticDataInterface_iterator it = sd->begin(); it != sd->end(); ++it) 
     {
         Ionet_data<vers>::ChannelKey        c = it->first;
         ionet_StaticDataInterface<vers>&    d = it->second;
         ChannelData chanData;
         char channelName[50] = {'\0'};
         sprintf(channelName, "%s", AnsiString(Ionet_string<vers>::key_string(c).c_str()));
         chanData.channelName = channelName;
         chanData.channelType = channelTypeShortStrings[d.channelType()];
         chanData.signalId = d.signalId();
         chanData.channelDesc = d.signalDescription();
         processData(chanData, staticState);
     }
  }
}

#endif  // _CHANNELS_H