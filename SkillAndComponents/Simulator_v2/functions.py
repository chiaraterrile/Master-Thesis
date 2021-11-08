import pandas as pd
import numpy as np
import io
import re
from collections import OrderedDict


def RemoveEmptyLines (str):
    lines = str.split("\n")
    non_empty_lines = [line for line in lines if line.strip() != ""]

    string_without_empty_lines = ""
    for line in non_empty_lines:
        string_without_empty_lines += line + "\n"

    return string_without_empty_lines



def TracesCreator() :
    f = open('log.txt', 'r')
    content = f.read()
    #print(content)
    #print ('#######################')
    f.close()


    # remove messages between BTs and Skills
    x = content.replace("From     : /BatteryLevel/BT_rpc/client\nTo       : /BatteryLevel/BT_rpc/server\nCommand  : request_ack\nArguments:\n", " ")
    x = x.replace("From     : /BatteryLevel/BT_rpc/server\nTo       : /BatteryLevel/BT_rpc/client\nReply    : request_ack\nArguments: 0", " ")
    x = x.replace("From     : /BatteryLevel/BT_rpc/server\nTo       : /BatteryLevel/BT_rpc/client\nReply    : request_ack\nArguments: 2", " ")
    x = x.replace(" From     : /BatteryLevel/BT_rpc/server\n To       : /BatteryLevel/BT_rpc/client\nReply    : request_ack\nArguments: 3", " ")
    x = x.replace("From     : /BatteryLevel/BT_rpc/client\nTo       : /BatteryLevel/BT_rpc/server\nCommand  : request_ack\nArguments:", " ")
    x = x.replace("From     : /GoToDestination/BT_rpc/client\nTo       : /GoToDestination/BT_rpc/server\nCommand  : request_ack\nArguments: ", " ")
    x = x.replace("From     : /BatteryLevel/BT_rpc/client\nTo       : /BatteryLevel/BT_rpc/server\nCommand  : send_start\nArguments: ", " ")
    x = x.replace("From     : /GoToDestination/BT_rpc/client\nTo       : /GoToDestination/BT_rpc/server\nCommand  : request_ack\nArguments: ", " ")
    x = x.replace("From     : /GoToDestination/BT_rpc/server\nTo       : /GoToDestination/BT_rpc/client\nReply    : send_start\nArguments: ", " ")
    x = x.replace("From     : /GoToDestination/BT_rpc/server\nTo       : /GoToDestination/BT_rpc/client\nReply    : request_ack\nArguments: 1", " ")
    x = x.replace("From     : /GoToDestination/BT_rpc/client\nTo       : /GoToDestination/BT_rpc/server\nCommand  : send_stop\nArguments: ", " ")
    x = x.replace("From     : /BatteryLevel/BT_rpc/server\nTo       : /BatteryLevel/BT_rpc/client\nReply    : send_start\nArguments: ", " ")
    x = x.replace("From     : /tick\nTo       : *\nTick     : ", " ")
    x = x.replace("From     : /GoToChargingStation/BT_rpc/server\nTo       : /GoToChargingStation/BT_rpc/client\nReply    : request_ack\nArguments: 1", " ")
    x = x.replace("From     : /GoToChargingStation/BT_rpc/client\nTo       : /GoToChargingStation/BT_rpc/server\nCommand  : request_ack\nArguments: ", " ")
    x = x.replace("From     : /GoToChargingStation/BT_rpc/server\nTo       : /GoToChargingStation/BT_rpc/client\nReply    : request_ack\nArguments: 0", " ")
    x = x.replace("From     : /GoToChargingStation/BT_rpc/server\nTo       : /GoToChargingStation/BT_rpc/client\nReply    : send_start\nArguments: ", " ")

    #rewrite messages of interest
    # INPUT messages 
    x = x.replace("From     : /GoToGoToClient/kitchen\nTo       : /GoToComponent\nCommand  : goTo\nArguments: kitchen", "goTo_ki_input\n")
    x = x.replace("From     : /BatteryReaderBatteryLevelClient\nTo       : /BatteryComponent\nCommand  : level\nArguments:", "level_input\n")
    x = x.replace("From     : /GoToGoToClient/kitchen\nTo       : /GoToComponent\nCommand  : getStatus\nArguments: kitchen", "getStatus_ki_input\n")
    x = x.replace("From     : /GoToGoToClient/kitchen\nTo       : /GoToComponent\nCommand  : halt\nArguments: kitchen", "halt_ki_input\n")
    x = x.replace("From     : /GoToGoToClient/charging_station\nTo       : /GoToComponent\nCommand  : goTo\nArguments: charging_station", "goTo_ch_input\n")
    x = x.replace("From     : /GoToGoToClient/charging_station\nTo       : /GoToComponent\nCommand  : getStatus\nArguments: charging_station", "getStatus_ch_input\n")
    x = x.replace("From     : /BatteryReaderBatteryNotChargingClient\nTo       : /BatteryComponent\nCommand  : charging_status\nArguments:", "batteryStatus_input\n")
    x = x.replace("From     : /GoToIsAtClient/kitchen\nTo       : /GoToComponent\nCommand  : isAtLocation\nArguments: kitchen", "isAt_ki_input\n")
    x = x.replace("From     : /GoToIsAtClient/charging_station\nTo       : /GoToComponent\nCommand  : isAtLocation\nArguments: charging_station", "isAt_ch_input\n")
    x = x.replace("From     : /GoToGoToClient/charging_station\nTo       : /GoToComponent\nCommand  : halt\nArguments: charging_station", "halt_ch_input\n")

    #OUTPUT messages
    x = x.replace("From     : /GoToComponent\nTo       : /GoToGoToClient/kitchen\nReply    : goTo\nArguments: kitchen", "output_goto_ki\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToGoToClient/kitchen\nReply    : getStatus\nArguments: kitchen 1", "output_getStatus_ki_run\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToGoToClient/kitchen\nReply    : getStatus\nArguments: kitchen 2", "output_getStatus_ki_suc\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToGoToClient/kitchen\nReply    : getStatus\nArguments: kitchen 3", "output_getStatus_ki_ab\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToGoToClient/kitchen\nReply    : halt\nArguments: kitchen", "output_halt_ki\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToGoToClient/charging_station\nReply    : goTo\nArguments: charging_station", "output_goto_ch\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToGoToClient/charging_station\nReply    : getStatus\nArguments: charging_station 1", "output_getStatus_ch_run\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToGoToClient/charging_station\nReply    : getStatus\nArguments: charging_station 2", "output_getStatus_ch_suc\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToGoToClient/charging_station\nReply    : getStatus\nArguments: charging_station 3", "output_getStatus_ch_ab\n")
    x = x.replace("From     : /BatteryComponent\nTo       : /BatteryReaderBatteryNotChargingClient\nReply    : charging_status\nArguments: 0", "output_batteryStatus_false\n")
    x = x.replace("From     : /BatteryComponent\nTo       : /BatteryReaderBatteryNotChargingClient\nReply    : charging_status\nArguments: 1", "output_batteryStatus_true\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToIsAtClient/kitchen\nReply    : isAtLocation\nArguments: kitchen 0", "output_isAt_ki_false\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToIsAtClient/kitchen\nReply    : isAtLocation\nArguments: kitchen 1", "output_isAt_ki_true\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToIsAtClient/charging_station\nReply    : isAtLocation\nArguments: charging_station 0", "output_isAt_ch_false\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToIsAtClient/charging_station\nReply    : isAtLocation\nArguments: charging_station 1", "output_isAt_ch_true\n")
    x = x.replace("From     : /GoToComponent\nTo       : /GoToGoToClient/charging_station\nReply    : halt\nArguments: charging_station", "output_halt_ch\n")

    #remove "DEBUG		Message received:" message
    x = x.replace("DEBUG", " ")
    x = x.replace("Message received:", " ")

    #managing battery message level
    arr = np.zeros(len(content), dtype=object) 
    counter_level = 0

    for line in x.splitlines():

        match = re.search('Arguments: (\d+)', line)
        
        if match:
            #print(match.group(1))
            
            arr[counter_level] = match.group(1)
            counter_level= counter_level+1
            

    for i in range(0, len(arr)):
        if int(arr[i])>500 :
            arr[i]= 'high'
        elif 450<int(arr[i])<500 :
            arr[i]= 'medium_six'
        elif 400<int(arr[i])<450 :
            arr[i]= 'medium_five'
        elif 350<int(arr[i])<400 :
            arr[i]= 'medium_four'
        elif 300<int(arr[i])<350 :
            arr[i]= 'medium_three'
        elif 250<int(arr[i])<300 :
            arr[i]= 'medium_two'
        elif 200<int(arr[i])<250 :
            arr[i]= 'medium_one'
        elif 1<int(arr[i])<200 :
            arr[i]= 'low'
        #elif int(arr[i]) == 0 :
        #   arr[i]= 'zero'
    #print(arr)

    for i in range(counter_level):
        x = x.replace("From     : /BatteryComponent\nTo       : /BatteryReaderBatteryLevelClient\nReply    : level", 'output_level_'+ arr[i]+'\n',1)
        
    x = x.replace("Arguments:","")

    #remove timestamps
    x = ''.join([i for i in x if not i.isdigit()])
    x = x.replace(".","")
    #remove empty lines
    x = RemoveEmptyLines(x)

    with open("Traces.txt", "w") as text_file:
        text_file.write(x)
    


