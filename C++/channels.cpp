// Copyright (c) MAN Energy Solutions

#include "channels.h"
#include <vcl.h>
#pragma hdrstop
#pragma package(smart_init)
#include <ionet/ionet.h>
#include <cnet/endian.h>
#include <co/delays.h>
#include <krnl/os/krnl_curtask.h>
#include <win32util/queue.h>
#include <stdexcept>
#include "common/ionet_strings.h"
#include <cnet/ntnodeid.h>


template <Ionet_vers vers>
BesListView<vers>::BesListView(unsigned char node, SyncQueue<ListViewEvent>* queue) :
   ionet_ListView<vers>(node), node_(node), queue_(queue)
{
   resume();
}

template <Ionet_vers vers>
BesListView<vers>::~BesListView()
{
   terminate();
}

template <Ionet_vers vers>
void BesListView<vers>::OnConnect(ionet_StaticMapWrap<vers>& staticData)
{
   ListViewEvent e;
   ionet_StaticMapWrapContainer* data = new ionet_StaticMapWrapContainer;
   data->set(staticData);
   e.type = ListViewEvent::ON_CONNECT;
   e.data = (void*)data;
   queue_->insert(e);
}

template <Ionet_vers vers>
void BesListView<vers>::OnDisconnect()
{
   ListViewEvent e;
   e.type = ListViewEvent::ON_DISCONNECT;
   e.data = 0;
   queue_->insert(e);
}

template <Ionet_vers vers>
void BesListView<vers>::OnUpdateView(ionet_ListItemWrap<vers>& listData)
{
   ListViewEvent             e;
   ionet_ListItemWrap<vers>* data = new ionet_ListItemWrap<vers>(listData);
   e.type = ListViewEvent::ON_UPDATE_VIEW;
   e.data = (void*)data;
   queue_->insert(e);
}

template <Ionet_vers vers>
void BesListView<vers>::OnNoData()
{
   ListViewEvent e;
   e.type = ListViewEvent::ON_NO_DATA;
   e.data = 0;
   queue_->insert(e);
}

template class BesListView<MK2>;
template class BesListView<EGEN3>;

std::string GetChannelTypeStr(ionet_StaticDataInterfaceBase*  sd_base)
{
   std::string retstr("UNKNOWN");
   ChannelType chan_type = sd_base->channelType();
   switch (chan_type)
   {
      case CHT_ANALOG_IN:
      case CHT_ANALOG_IN_CIOM:
         retstr = "ANALOG_IN";
         break;
      case CHT_ANALOG_IN_AIB:
      case CHT_ANALOG_IN_AIB_CIOM:
         retstr = "DIGITAL_IN";
         break;
      case CHT_ANALOG_OUT:
         retstr  = "ANALOG_OUT";
         break;
      case CHT_DIGITAL_IN:
         retstr  = "DIGITAL_IN";
         break;
      case CHT_DIGITAL_OUT:
      case CHT_DIGITAL_OUT_CIOM:
      case CHT_DIGITAL_OUT_RELAY:
         retstr  = "DIGITAL_OUT";
         break;
   }
   return retstr;
}

std::string TIoListView::generateSha(const ChannelData& channelData) {
   std::string text = channelData.invalidValid + channelData.alarmState;
   mbedtls_sha1_context sha1_ctx;
   mbedtls_sha1_init(&sha1_ctx);
   mbedtls_sha1_starts(&sha1_ctx);
   mbedtls_sha1_update(&sha1_ctx, reinterpret_cast<const unsigned char*>(text.c_str()), text.length());
   unsigned char hash[20]; 
   mbedtls_sha1_finish(&sha1_ctx, hash);
   mbedtls_sha1_free(&sha1_ctx);
   return std::string((char*)hash);
}


void TIoListView::publishData(const ChannelData& chanData, std::string staticState) 
{
   char nodeName[50] = {'\0'};
   sprintf(nodeName, "%s", cnet_NetNodeId::getNodeIdText(node()).cStr());
   mqtt_public_interface pInterface("channels", std::string(chanData.channelName + "/info"), mqtt_json);
   if (staticState == "NONSTATIC")
   {
   pInterface.data.insert(mqtt_name, chanData.channelName);
   pInterface.data.insert(mqtt_alarm, chanData.alarmState);
   pInterface.data.insert(mqtt_valid, chanData.invalidValid);
   pInterface.data.insert(mqtt_node, nodeName);
   } else if (staticState == "STATIC")
   {
   pInterface.data.insert(mqtt_name, chanData.channelName);
   pInterface.data.insert(mqtt_type, chanData.channelType);
   pInterface.data.insert(mqtt_signalId, chanData.signalId);
   pInterface.data.insert(mqtt_desc, chanData.channelDesc);
   pInterface.data.insert(mqtt_node, nodeName);
   }
   pInterface.publish();
}

void TIoListView::processData(const ChannelData& chanData, std::string staticState) 
{
   const std::string& channelName = chanData.channelName;
   std::map<std::string, std::string>::iterator it = hashMap.find(channelName);
   if (it != hashMap.end()) {
      std::string currentSha = it->second;
      std::string newSha = generateSha(chanData);
      if (currentSha != newSha) {
         publishData(chanData, staticState);
         hashMap[channelName] = newSha;
      }
   } else {
      publishData(chanData, staticState);
      std::string newSha = generateSha(chanData);
      hashMap[channelName] = newSha;
   }
}

__fastcall TIoListView::TIoListView(unsigned char node) : TThread(false), node_(node), ionetVersion_(0) 
{ }

void TIoListView::SetName()
{
   THREADNAME_INFO info;
   info.dwType     = 0x1000;
   info.szName     = "VCL list view";
   info.dwThreadID = -1;
   info.dwFlags    = 0;
   __try
   {
      RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (DWORD*)&info);
   } __except (EXCEPTION_CONTINUE_EXECUTION)
   {
   }
}

void __fastcall TIoListView::OnConnect()
{
   ionet_StaticMapWrapContainer& data = *(ionet_StaticMapWrapContainer*)evt_.data;
   staticData_ = data;
   OnConnectFormHandling<EGEN3>(data);
}

void __fastcall TIoListView::OnDisconnect()
{ }

void __fastcall TIoListView::OnUpdateView()
{  
   if (MK2 == ionetVersion_)  
   {
      ionet_ListItemWrap<MK2>& data = *(ionet_ListItemWrap<MK2>*)evt_.data;
      OnUpdateViewFormHandling<MK2>(data);
   }
   else
   {
      ionet_ListItemWrap<EGEN3>& data = *(ionet_ListItemWrap<EGEN3>*)evt_.data;
      OnUpdateViewFormHandling<EGEN3>(data);
   }
}

void __fastcall TIoListView::OnNoData()
{ }

void __fastcall TIoListView::QueueCleanup()
{
   while (queue_.remove(evt_, 0) == W32UTIL_WAIT_OK)
      delete evt_.data;
}

void __fastcall TIoListView::Execute()
{
  ionet_AbstractBase*   view=0;
  bool                  version_ok = false;
  try {
     SetName();
     OnDisconnect();
     while(!Terminated)
     {
       if (version_ok)
       {
         if(queue_.remove(evt_, 500) == W32UTIL_WAIT_OK)
         {
           switch(evt_.type)
           {
           case ListViewEvent::ON_CONNECT:
              OnConnect();
              delete reinterpret_cast<ionet_StaticMapWrapContainer*>(evt_.data);
              break;
           case ListViewEvent::ON_DISCONNECT:
              OnDisconnect();
              QueueCleanup();
              break;
           case ListViewEvent::ON_UPDATE_VIEW:
              if( node_ == node_)
              {
                  OnUpdateView();
              }
              if (ionetVersion_ == MK2)
                 delete reinterpret_cast<ionet_ListItemWrap<MK2>* >(evt_.data);
              else
                 delete reinterpret_cast<ionet_ListItemWrap<EGEN3>* >(evt_.data);
              break;
           case ListViewEvent::ON_NO_DATA:
              OnNoData();
              break;
           }
         }
       }
       else
       {
         krnl_CurrentTask::delay(200);
         version_ok = ionet_Info::ionet_Version(0,ionetVersion_);
         if (version_ok)
         {
            if (ionetVersion_ == MK2)
              view = new BesListView<MK2>(node_,&queue_);
            else
              view = new BesListView<EGEN3>(node_,&queue_);
         }
       }
     }
     delete view;
     QueueCleanup();
  }
  catch(std::exception& e) {
     const char* s = e.what();
     (void)s;
     exit(1);
  }
  catch(...) {
     exit(1);
  }
}