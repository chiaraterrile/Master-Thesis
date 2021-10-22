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




f = open('log.txt', 'r')
content = f.read()
print(content)
print ('#######################')
f.close()


# remove messages between BTs and Skills
x = content.replace("From     : /BatteryLevel/BT_rpc/client\nTo       : /BatteryLevel/BT_rpc/server\nCommand  : request_ack\nArguments:\n", " ")
x = x.replace("From      : /BatteryLevel/BT_rpc/server\nTo        : /BatteryLevel/BT_rpc/client\nProtocol  : Skill_request\nCommand   : request_ack\nArguments : \nReply     : 0", " ")
x = x.replace("From      : /BatteryLevel/BT_rpc/server\nTo        : /BatteryLevel/BT_rpc/client\nProtocol  : Skill_request\nCommand   : request_ack\nArguments : \nReply     : 2", " ")
x = x.replace("From      : /BatteryLevel/BT_rpc/server\nTo        : /BatteryLevel/BT_rpc/client\nProtocol  : Skill_request\nCommand   : request_ack\nArguments : \nReply     : 3", " ")
x = x.replace(" From     : /BatteryLevel/BT_rpc/server\n To       : /BatteryLevel/BT_rpc/client\nReply    : request_ack\nArguments: 3", " ")
x = x.replace("From      : /BatteryLevel/BT_rpc/client\nTo        : /BatteryLevel/BT_rpc/server\nProtocol  : Skill_request\nCommand   : request_ack\nArguments : \nReply     : ", " ")
x = x.replace("From      : /GoToDestination/BT_rpc/client\nTo        : /GoToDestination/BT_rpc/server\nProtocol  : Skill_request\nCommand   : request_ack\nArguments : \nReply     : ", " ")
x = x.replace("From      : /BatteryLevel/BT_rpc/client\nTo        : /BatteryLevel/BT_rpc/server\nProtocol  : Skill_request\nCommand   : send_start\nArguments : \nReply     : ", " ")
x = x.replace("From      : /GoToDestination/BT_rpc/client\nTo        : /GoToDestination/BT_rpc/server\nCommand   : Skill_request\nArguments : request_ack\nReply     : ", " ")
x = x.replace("From      : /GoToDestination/BT_rpc/server\nTo        : /GoToDestination/BT_rpc/client\nProtocol  : Skill_request\nCommand   : request_ack\nArguments : \nReply     : 0", " ")
x = x.replace("From      : /GoToDestination/BT_rpc/server\nTo        : /GoToDestination/BT_rpc/client\nProtocol  : Skill_request\nCommand   : send_start\nArguments : \nReply     : ", " ")
x = x.replace("From      : /GoToDestination/BT_rpc/server\nTo        : /GoToDestination/BT_rpc/client\nProtocol  : Skill_request\nCommand   : request_ack\nArguments : \nReply     : 1", " ")
x = x.replace("From      : /GoToDestination/BT_rpc/client\nTo        : /GoToDestination/BT_rpc/server\nProtocol  : Skill_request\nCommand   : send_stop\nArguments : \nReply     : ", " ")
x = x.replace("From      : /GoToDestination/BT_rpc/server\nTo        : /GoToDestination/BT_rpc/client\nProtocol  : Skill_request\nCommand   : send_stop\nArguments : \nReply     : ", " ")
x = x.replace("From      : /BatteryLevel/BT_rpc/server\nTo        : /BatteryLevel/BT_rpc/client\nProtocol  : Skill_request\nCommand   : send_start\nArguments : \nReply     : ", " ")
x = x.replace("From     : /tick\nTo       : *\nTick     : ", " ")
x = x.replace("From      : /GoToChargingStation/BT_rpc/server\nTo        : /GoToChargingStation/BT_rpc/client\nProtocol  : Skill_request\nCommand   : request_ack\nArguments : \nReply     : 0", " ")
x = x.replace("From      : /GoToChargingStation/BT_rpc/server\nTo        : /GoToChargingStation/BT_rpc/client\nProtocol  : Skill_request\nCommand   : request_ack\nArguments : \nReply     : 1", " ")
x = x.replace("From      : /GoToChargingStation/BT_rpc/client\nTo        : /GoToChargingStation/BT_rpc/server\nProtocol  : Skill_request\nCommand   : request_ack\nArguments : \nReply     :", " ")
x = x.replace("From     : /GoToChargingStation/BT_rpc/server\nTo       : /GoToChargingStation/BT_rpc/client\nReply    : request_ack\nArguments: 0", " ")
x = x.replace("From      : /GoToChargingStation/BT_rpc/server\nTo        : /GoToChargingStation/BT_rpc/client\nProtocol  : Skill_request\nCommand   : send_start\nArguments : \nReply     :", " ")
x = x.replace("From      : /GoToDestination/BT_rpc/client\nTo        : /GoToDestination/BT_rpc/server\nProtocol  : Skill_request\nCommand   : send_start\nArguments : \nReply     : ", " ")
x = x.replace("From      : /tick/monitor\nTo        : /monitor\nProtocol  : Tick\nCommand   : \nArguments : ", " ")


#rewrite messages of interest
# INPUT messages 
x = x.replace("From      : /GoToGoToClient/kitchen\nTo        : /GoToComponent\nProtocol  : GoTo\nCommand   : goTo\nArguments : kitchen\nReply     : ", "goTo_ki_input\n")
x = x.replace("From      : /BatteryReaderBatteryLevelClient\nTo        : /BatteryComponent\nProtocol  : BatteryReader\nCommand   : level\nArguments : \nReply     : ", "level_input\n")
x = x.replace("From      : /GoToGoToClient/kitchen\nTo        : /GoToComponent\nProtocol  : GoTo\nCommand   : getStatus\nArguments : kitchen\nReply     : ", "getStatus_ki_input\n")
x = x.replace("From      : /GoToGoToClient/kitchen\nTo        : /GoToComponent\nProtocol  : GoTo\nCommand   : halt\nArguments : kitchen\nReply     : ", "halt_ki_input\n")
x = x.replace("From      : /GoToGoToClient/charging_station\nTo        : /GoToComponent\nProtocol  : GoTo\nCommand   : goTo\nArguments : charging_station\nReply     :", "goTo_ch_input\n")
x = x.replace("From      : /GoToGoToClient/charging_station\nTo        : /GoToComponent\nProtocol  : GoTo\nCommand   : getStatus\nArguments : charging_station\nReply     :", "getStatus_ch_input\n")
x = x.replace("From      : /BatteryReaderBatteryNotChargingClient\nTo        : /BatteryComponent\nProtocol  : BatteryReader\nCommand   : charging_status\nArguments : \nReply     : ", "batteryStatus_input\n")
x = x.replace("From      : /GoToIsAtClient/kitchen\nTo        : /GoToComponent\nProtocol  : GoTo\nCommand   : isAtLocation\nArguments : kitchen\nReply     : ", "isAt_ki_input\n")
x = x.replace("From      : /GoToIsAtClient/charging_station\nTo        : /GoToComponent\nProtocol  : GoTo\nCommand   : isAtLocation\nArguments : charging_station\nReply     :", "isAt_ch_input\n")
x = x.replace("From     : /GoToGoToClient/charging_station\nTo       : /GoToComponent\nCommand  : halt\nArguments: charging_station", "halt_ch_input")

#OUTPUT messages
x = x.replace("From      : /GoToComponent\nTo        : /GoToGoToClient/kitchen\nProtocol  : GoTo\nCommand   : goTo\nArguments : \nReply     : kitchen", "output_goto_ki\n")
x = x.replace("From      : /GoToComponent\nTo        : /GoToGoToClient/kitchen\nProtocol  : GoTo\nCommand   : getStatus\nArguments : kitchen\nReply     : 1", "output_getStatus_ki_run\n")
x = x.replace("From      : /GoToComponent\nTo        : /GoToGoToClient/kitchen\nProtocol  : GoTo\nCommand   : getStatus\nArguments : kitchen\nReply     : 2", "output_getStatus_ki_suc\n")
x = x.replace("From      : /GoToComponent\nTo        : /GoToGoToClient/kitchen\nProtocol  : GoTo\nCommand   : getStatus\nArguments : kitchen\nReply     : 3", "output_getStatus_ki_ab\n")
x = x.replace("From      : /GoToComponent\nTo        : /GoToGoToClient/kitchen\nProtocol  : GoTo\nCommand   : halt\nArguments : kitchen\nReply     : ", "output_halt_ki")
x = x.replace("From      : /GoToComponent\nTo        : /GoToGoToClient/charging_station\nProtocol  : GoTo\nCommand   : goTo\nArguments : \nReply     : charging_station", "output_goto_ch\n")
x = x.replace("From      : /GoToComponent\nTo        : /GoToGoToClient/charging_station\nProtocol  : GoTo\nCommand   : getStatus\nArguments : charging_station\nReply     : 1", "output_getStatus_ch_run\n")
x = x.replace("From      : /GoToComponent\nTo        : /GoToGoToClient/charging_station\nProtocol  : GoTo\nCommand   : getStatus\nArguments : charging_station\nReply     : 2", "output_getStatus_ch_suc\n")
x = x.replace("From      : /GoToComponent\nTo        : /GoToGoToClient/charging_station\nProtocol  : GoTo\nCommand   : getStatus\nArguments : charging_station\nReply     : 3", "output_getStatus_ch_ab\n")
x = x.replace("From      : /BatteryComponent\nTo        : /BatteryReaderBatteryNotChargingClient\nProtocol  : BatteryReader\nCommand   : charging_status\nArguments : \nReply     : 0", "output_batteryStatus_false\n")
x = x.replace("From     : /BatteryComponent\nTo       : /BatteryReaderBatteryNotChargingClient\nReply    : charging_status\nArguments: 1", "output_batteryStatus_true")
x = x.replace("From      : /GoToComponent\nTo        : /GoToIsAtClient/kitchen\nProtocol  : GoTo\nCommand   : isAtLocation\nArguments : kitchen\nReply     : 1818845542", "output_isAt_ki_false\n")
x = x.replace("From      : /GoToComponent\nTo        : /GoToIsAtClient/kitchen\nProtocol  : GoTo\nCommand   : isAtLocation\nArguments : kitchen\nReply     : 27503", "output_isAt_ki_true\n")
x = x.replace("From      : /GoToComponent\nTo        : /GoToIsAtClient/charging_station\nProtocol  : GoTo\nCommand   : isAtLocation\nArguments : charging_station\nReply     : 1818845542", "output_isAt_ch_false\n")
x = x.replace("From      : /GoToComponent\nTo        : /GoToIsAtClient/charging_station\nProtocol  : GoTo\nCommand   : isAtLocation\nArguments : charging_station\nReply     : 27503", "output_isAt_ch_true\n")

#remove "DEBUG		Message received:" message
x = x.replace("DEBUG", " ")
x = x.replace("Message received:", " ")
x = x.replace("Command intercepted:", " ")
x = x.replace("Reply intercepted:", " ")
x = x.replace("Tick intercepted:", " ")


#managing battery message level
arr = np.zeros(len(content), dtype=object) 
counter_level = 0

for line in x.splitlines():

    match = re.search('Reply     : (\d+)', line)
    
    if match:
        print(match.group(1))
        
        arr[counter_level] = match.group(1)
        counter_level= counter_level+1
        

for i in range(0, len(arr)):
    if int(arr[i])>30 :
        arr[i]= 'high'
    elif 20<int(arr[i])<30 :
        arr[i]= 'medium'
    elif 1<int(arr[i])<20 :
        arr[i]= 'low'
    elif int(arr[i]) == 0 :
        arr[i]= 'zero'
print(arr)

for i in range(counter_level):
    x = x.replace("From      : /BatteryComponent\nTo        : /BatteryReaderBatteryLevelClient\nProtocol  : BatteryReader\nCommand   : level\nArguments :", 'output_level_'+ arr[i]+'\n',1)
    
x = x.replace("Reply     :","")

#remove timestamps
x = ''.join([i for i in x if not i.isdigit()])
x = x.replace(".","")
#remove empty lines
x = RemoveEmptyLines(x)

with open("Traces.txt", "w") as text_file:
    text_file.write(x)


