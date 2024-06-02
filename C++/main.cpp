// Copyright (c) MAN Energy Solutions

#include <vcl.h>
#include <ionet/ionet_info.h>
#include <ionet/w/ionet_pc_simul.h>
#include <ionet/w/ionet_pc_stand_alone_sim.h>
#include <nctrl/nctrl_modedef.h>
#include <rnctrl/rnctrl_client.h>
#include "channels.h"
#include <netman/netman.h>
#include <netman/netman_obs.h>
#include <parnet/parnet_mon_proxy.h>
#include <cnet/ntnodeid.h>
#include <mqtt_interface/mqtt_publish_interface.h>
#include <mqtt_interface/mqtt_write_interface.h>
#include <win32util/queue.h>
#include <windows.h>
#include <ionet/w/ionet_paramnet_wrap.h>
#include <map>

struct NodeMonEvent
{
   bool          connect;
   unsigned char node;
   unsigned char mode;
};

class BesNodeMon : public netman_NetworkObserver, public mqtt_write_interface
{
public:
   BesNodeMon(SyncQueue<NodeMonEvent>* queue);
   ~BesNodeMon();
public:
   virtual void onConnected(const unsigned char remoteNodeId, const unsigned char nodeMode);
   virtual void onDisconnected(const unsigned char remoteNodeId);
   virtual void onCombinedNssvUpdate(const unsigned char remoteNodeId, const co_BitSet256& combinedNssv);
   void         GenerateNodeModeString(unsigned char node, unsigned char mode, bool connect);
   virtual void write(Write* payload);
   void         publishIoNodeChannels(unsigned char node);
private:
   SyncQueue<NodeMonEvent>* queue_;
   SyncQueue<ListViewEvent> listQueue_;
   TIoListView*             view;
};

BesNodeMon::BesNodeMon(SyncQueue<NodeMonEvent>* queue) : queue_(queue), mqtt_write_interface("iotest")
{
   netman_NetworkManager::registerObserver(this);
};

BesNodeMon::~BesNodeMon()
{
   netman_NetworkManager::unregisterObserver(this);
};

void BesNodeMon::onConnected(const unsigned char remoteNodeId, const unsigned char nodeMode)
{
   NodeMonEvent e;
   e.connect = true;
   e.node    = remoteNodeId;
   e.mode    = nodeMode;
   queue_->insert(e);
};

void BesNodeMon::onDisconnected(const unsigned char remoteNodeId)
{
   NodeMonEvent e;
   e.connect = false;
   e.node    = remoteNodeId;
   queue_->insert(e);
};

void BesNodeMon::onCombinedNssvUpdate(const unsigned char remoteNodeId, const co_BitSet256& combinedNssv){};

void BesNodeMon::GenerateNodeModeString(unsigned char node, unsigned char mode, bool connect)
{
   char nodeName_[10] = {'\0'};
   sprintf(nodeName_, "%s", cnet_NetNodeId::getNodeIdText(node).cStr());
   char nodeInteger[10] = {'\0'};
   sprintf(nodeInteger, "%u", node);
   std::string nodeName(nodeName_);
   mqtt_public_interface pInterface("node", std::string(nodeName + "/info"), mqtt_json);
   if (nodeName != "UNDEF")
   {
      pInterface.data.insert(mqtt_node, nodeName);
      pInterface.data.insert(mqtt_mode, getNodeModeDescriptor((const nctrl_NodeMode)mode));
      pInterface.data.insert(mqtt_iostate, connect);
      pInterface.publish();
   } else
   {
      std::cout << "\n----- Node : " << nodeInteger << " / " << nodeName << " NOT published as node is UNDEFINED -----" << std::endl;
   }
}


void BesNodeMon::write(Write* payload)
{
   rnctrl_Client::init();
   while (payload)
   {
      DataChain* dataChain = payload->getData();
      std::string   payloadStr   = payload->getNode();
      const char*   payloadNode_ = payloadStr.c_str();
      unsigned char node_        = cnet_NetNodeId::resolveNodeId(co_Text(payloadNode_));
      char nodeInteger[10] = {'\0'};
      sprintf(nodeInteger, "%u", node_);
      while (dataChain)
      {
         long value;
         dataChain->val(value);
         std::string attribute = payload->getAttribute();
         if(attribute == std::string("nodeInfo"))
         {
            publishIoNodeChannels(node_);
         }
         else if(attribute == std::string("setMode"))
         {
            if(value == nctrl_NodeMode::NCTRL_NODE_MODE_NORMAL) {
               bool invalidMode = false;
               rnctrl_Client::modeChange(node_, NCTRL_NODE_MODE_NORMAL, &invalidMode);
            } else {
               bool invalidMode = false;
               rnctrl_Client::modeChange(node_, NCTRL_NODE_MODE_CONNECTION_TEST, &invalidMode);
            }
         }
         else if (attribute == std::string("inValidate"))
         {
            unsigned char node  = node_;
            ionet_ParamSession* ses = 0;
            ses = new ionet_ParamSession(node);
            ses->start();
            AnsiString componentId = value;
            AnsiString id = componentId + "03";
            if(ses->setParameter(id.c_str(), true) == true)
            { }
            ses->activate();
            delete ses;
         }
         else if (attribute == std::string("validate"))
         {
            unsigned char node  = node_;
            ionet_ParamSession* ses = 0;
            ses = new ionet_ParamSession(node);
            ses->start();
            AnsiString componentId = value;
            AnsiString id = componentId + "03";
            if(ses->setParameter(id.c_str(), false) == true)
            { }
            ses->activate();
            delete ses;
         }
         else if (attribute == std::string("inValidateAll"))
         {
            ionet_ParamSession* ses = 0;
            ses = new ionet_ParamSession(node_);
            ses->start();
            AnsiString id = "411103";
            ses->setParameter(id.c_str(), true);
            if(ses) {
               ses->activate();
               delete ses;
            };
         }
         else if (attribute == std::string("validateAll"))
         {
            ionet_ParamSession* ses = 0;
            ses = new ionet_ParamSession(node_);
            ses->start();
            AnsiString id = "411103";
            ses->setParameter(id.c_str(), false);
            if(ses) {
               ses->activate();
               delete ses;
            };
         }
         dataChain = dataChain->parrent();
      }
      payload = payload->parrent();
   }
}

void BesNodeMon::publishIoNodeChannels(unsigned char node_)
{
   view = new TIoListView(node_);
}

int main()
{
   unsigned char ionetVersion;
   ionet_Info::ionet_Version(0, ionetVersion);
   if (3 == ionetVersion)
   { }
   else if (6 == ionetVersion)
   { }
   else
   { }
   SyncQueue<NodeMonEvent> queue_;
   BesNodeMon*             mon = new BesNodeMon(&queue_);
   while (mon)
   {
      NodeMonEvent evt_;
      if (queue_.remove(evt_, 500) == W32UTIL_WAIT_OK)
      {
         if (evt_.connect)
         {
            std::cout << "\n ----- Connect event ----- \n" << std::endl;
            mon->GenerateNodeModeString(evt_.node, evt_.mode, evt_.connect);
         }
         else
         {
            std::cout << "\n ----- Disconnect event ----- \n" << std::endl;
            mon->GenerateNodeModeString(evt_.node, evt_.mode, evt_.connect);
         }
      }
   }
   return 0;
};