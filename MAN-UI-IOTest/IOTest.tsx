// Copyright (c) MAN Energy Solutions

import { Button } from "man-ui-core/dist/src/layouts/button";
import { Primary, Row, Col } from "man-ui-core/dist/src/layouts/containers";
import { formatMessage } from "devextreme/localization";
import { parseCSV } from '../csvParser';
import { loadMessages, locale } from 'devextreme/localization';
import { ISourceContext, SourceContext } from "man-ui-core/dist/src/context/source_context";
import { useState, useCallback, useEffect, useContext, useRef } from 'react';
import { Column } from 'devextreme-react/data-grid';
import { DataGrid } from "man-ui-core/dist/src/layouts/data_grid";
import { Cell } from "man-ui-core/dist/src/ui/render"
import { Dialog } from "man-ui-core/dist/src/layouts/dialog";
import { Toast } from 'devextreme-react/toast';
import { ensureError } from '../../errorHandling'


export const IOTest = () => {
   locale("en");
   loadMessages(parseCSV())

   const channelGrid = useRef(null);
   const grid = useRef(null);
   const [selectedNode, setSelectedNode] = useState<string | null>(undefined);
   const [selectedChannel, setSelectedChannel] = useState<string>();
   const sources: ISourceContext = useContext(SourceContext);
   const nodes = sources.fetch({id: "NodeList"});
   const [nodesList, setNodeList] = useState([{"id": undefined, "mode": undefined, "node": undefined}]);  
   const nodeModes = sources.fetch({ id: "NodeList", options: { select: ["node", "mode"], filter: ["node", "=", selectedNode] } });
   const ChannelList = sources.fetch({id: "ChannelList", options: { filter: ["node", "=", selectedNode] }});
   const channelState = sources.fetch({ id: "ChannelList", options: { select: ["signalId", "valid"], filter: ["signalId", "=", selectedChannel]}});
   
   const [toastConfig, setToastConfig] = useState({
      isVisible: false,
      type: 'info',
      message: '',
   } as {
      isVisible: boolean,
      type: 'error' | 'info' | 'success' | 'warning',
      message: string,
   });

   useEffect(() => {
      nodes.on(("changed"), () => {
         const nodeInfo = nodes.items();
         if (nodeInfo.length > 0){
            setNodeList(nodeInfo);
            grid.current.instance.repaint();
         }
      })
  }, []); 

   const onHiding = useCallback(() => {
      setToastConfig({
         ...toastConfig,
         isVisible: false,
      });
   }, [toastConfig, setToastConfig]);

   function nodeNormalMode() {
      try {
         const mode = nodeModes.items()[0].mode;
         if (mode != "Normal") {
            nodes.store().update(1, {node: selectedNode, value: 5, action: "setMode"});
            setToastConfig({
               ...toastConfig,
               isVisible: true,
               type:'success',
               message:'Switched node to normal mode.',
            });
         } else {
            setToastConfig({
               ...toastConfig,
               isVisible: true,
               type:'error',
               message:"The selected node is already in 'Normal' mode.",
            });
         }
      } catch (err) {
         setToastConfig({
            ...toastConfig,
            isVisible: true,
            type:'warning',
            message:'Select the node you wish to change mode on.',
         });
         const error = ensureError(err) 
         console.log(error.message);
      }
   }

   function nodeTestMode() {
      try {
         const mode = nodeModes.items()[0].mode;
         if (mode != "Connection Test") {
            nodes.store().update(1, {node: selectedNode, value: 6, action: "setMode"});
            setToastConfig({
               ...toastConfig,
               isVisible: true,
               type:'success',
               message:'Switched node to test mode.',
            });
         } else {
            setToastConfig({
               ...toastConfig,
               isVisible: true,
               type:'error',
               message:"The selected node is already in 'Test' mode.",
            });
         }
      } catch (err) {
         setToastConfig({
            ...toastConfig,
            isVisible: true,
            type:'warning',
            message:'Select the node you wish to change mode on.',
         });
         const error = ensureError(err) 
         console.log(error.message);
      }
   }  

   function nodeToggleValidation() {
      try 
      {
         const state = channelState.items()[0];
         if (state.valid == "VALID") 
         {
            nodes.store().update(1, {node: selectedNode, value: selectedChannel, action: "inValidate"});
         } 
         else 
         {
            nodes.store().update(1, {node: selectedNode, value: selectedChannel, action: "validate"});
         } 
      } catch (err) {
         setToastConfig({
            ...toastConfig,
            isVisible: true,
            type:'warning',
            message:'Select a channel in second datagrid first.',
         });
         const error = ensureError(err) 
         console.log(error.message);
      }
   }

   function nodeValidateAll() {
      // This is the button to validate all nodes at once.
      console.log("All nodes validated")
   }

   function nodeInvalidateAll() {
      // This is the button to invalidate all nodes at once.
      console.log("All nodes invalidated")
   }

   function nodeInvalidatedList() {
      // This is the button to get a list of all invalidated nodes.
      console.log("showing list of invalidated nodes")
   }

   function onNodeSelected(selectedNode) {
      if(selectedNode)
      {
         nodes.store().update(1, {node: selectedNode.node, value: 0, action: "nodeInfo"});
         setSelectedNode(selectedNode.node);
      }
      else 
      {
         setSelectedNode(undefined)
      }
   }
   
   function onChannelSelected(selectedChannel) {
      try {
         setSelectedChannel(selectedChannel.signalId);
      } catch (err) {
         const error = ensureError(err) 
         console.log(error.message);
      }
   }

   return (
      <>
         <Primary>
            <Col>
               <Row>
                  <DataGrid
                     dataSource={nodesList}
                     test_id="nodeTable"
                     onSelectionChanged={(e) => onNodeSelected(e.selectedRowsData[0])}
                     selection={{ mode: 'single' }}
                     keyExpr={"node"}
                     repaintChangesOnly={false}
                     ref={grid}
                     >
                     <Column dataField="node" alignment="center" />
                     <Column dataField="mode" alignment="center" />
                  </DataGrid>
                  {selectedNode && (
                     <DataGrid
                        dataSource={ChannelList}
                        test_id="channelTable"
                        ref={channelGrid}
                        onSelectionChanged={(e) => onChannelSelected(e.selectedRowsData[0])}
                        selection={{ mode: 'single' }}
                     >
                        <Column dataField="name" alignment="center" />
                        <Column dataField="type" alignment="center" />
                        <Column dataField="signalId" alignment="center" />
                        <Column dataField="description" alignment="center" />
                        <Column dataField="valid" alignment="center" />
                        <Column dataField="alarm" alignment="center" />
                        <Column dataField="value" alignment="center" />
                        <Column cellRender={(data) => {
                           const chanDataSource = []
                           for (let key in data.data) {
                              let value = data.data[key];
                              chanDataSource.push({ property: key, value: value })
                           }
                           return <Cell data={{ state: "success" }} trigger={
                              <Dialog title={"Channel " + ChannelList[data.rowIndex]?.name + " on node " + selectedNode}>
                                 <DataGrid dataSource={chanDataSource} test_id="popupTable" paging={{ pageSize: 25 }} />
                              </Dialog>
                           } />;
                        }} />
                     </DataGrid>
                  )}
               </Row>
            </Col>
            <Row>
               {/* Buttons */}
               <Button caption={formatMessage("normal_mode")} text={formatMessage("normal_mode")} test_id="normal_mode" onClick={nodeNormalMode}/>
               <Button caption={formatMessage("test_mode")} text={formatMessage("test_mode")} test_id="test_mode" onClick={nodeTestMode}/>
               <Button caption={formatMessage("toggle_validated")} text={formatMessage("toggle_validated")} test_id="toggle_validated" onClick={nodeToggleValidation}/>
               <Button caption={formatMessage("validate_all")} text={formatMessage("validate_all")} test_id="validate_all" onClick={nodeValidateAll} />
               <Button caption={formatMessage("invalidate_all")} text={formatMessage("invalidate_all")} test_id="invalidate_all" onClick={nodeInvalidateAll} />
               <Button caption={formatMessage("show_invalidated_list")} text={formatMessage("show_invalidated_list")} test_id="show_invalidated_list" onClick={nodeInvalidatedList} />
               <Toast
                  visible={toastConfig.isVisible}
                  message={toastConfig.message}
                  type={toastConfig.type}
                  onHiding={onHiding}
                  displayTime={2000}
               />
            </Row>
         </Primary>
      </>
   )
}