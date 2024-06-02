// Copyright (c) MAN Energy Solutions

import { FNHandler, ServerObject } from '@server/utils/friendly_name';
import { Channels, Node} from "@server/database/entity/data_models"
import { dbWrapper } from '@server/database/controls/database_api';
import { mqttInterface } from "@server/utils/connect_mqtt";


FNHandler.createSO("NodeList", getNodes, setNodeMode, []);
FNHandler.createSO("ChannelList", getChannels, null, []);

async function getNodes(ref: any, FNS: Array<string>)
{
    return new Promise((resolve) => {
        dbWrapper.subRepository(Node, ref)
        .createQueryBuilder("nodes")
        .select(["nodes.node as id", "nodes.node as node", "nodes.mode as mode", "nodes.state as state"])
        .where("state = :state", { state: 1 })
        .getRawMany().then(result => {
            if(result)
            {
                resolve(result);
            }
            else {
                resolve({});
            }
    })});
}

function setNodeMode(ref:any, node_info: any)
{
    const mqttCmd = "#" + node_info.node + "/action/"+node_info.action+"/"+node_info.value;
    mqttInterface.publish("iotest", "write", mqttCmd);
    (FNHandler.dnByName("NodeList") as ServerObject)?.multicast();
}

async function getChannels(ref: any, FNS: Array<string>)
{
    return new Promise((resolve) => {
        dbWrapper.subRepository(Channels, ref)
        .createQueryBuilder("channels")
        .select(["channels.name as id", "channels.name as name", 
        "channels.alarm as alarm", "channels.valid as valid", "channels.nodeNode as node",
        "channels.type as type", "channels.signalId as signalId", "channels.description as description"])
        .getRawMany().then(result => {   
            if(result)
            {
                resolve(result);
            }
            else {
                resolve({});
            }
    })});
}